#ifndef TPCRECO_ANALYSIS_DIFF_ANALYSIS_H_
#define TPCRECO_ANALYSIS_DIFF_ANALYSIS_H_
#include "EventInfo.h"
#include "TTreeOps.h"
#include "Track3D.h"
#include <TFile.h>
#include <TTree.h>
#include <functional>
#include <iostream>
#include <memory>
namespace tpcreco {
namespace analysis {
namespace diff {
namespace checks {
void checkSegments(int entryIndex, eventraw::EventInfo *, Track3D *inputTrack,
                   eventraw::EventInfo *, Track3D *referenceTrack,
                   const std::string &treeInfo) {
  auto inputSegments = inputTrack->getSegments().size();
  auto referenceSegments = referenceTrack->getSegments().size();
  if (inputSegments != referenceSegments) {
    std::cout << "s > entry index " << entryIndex << "\tsegments "
              << inputSegments << " vs " << referenceSegments << treeInfo
              << '\n';
  }
}

void checkType(int entryIndex, eventraw::EventInfo *inputInfo, Track3D *,
               eventraw::EventInfo *referenceInfo, Track3D *,
               const std::string &treeInfo) {
  auto inputType = inputInfo->GetEventType().to_ulong();
  auto referenceType = referenceInfo->GetEventType().to_ulong();
  if (inputType != referenceType) {
    std::cout << "t > entry index " << entryIndex << "\ttype " << inputType
              << " vs " << referenceType << treeInfo << '\n';
  }
}

} // namespace checks

class DetailSink {
public:
  DetailSink(TTree *inputTree, TTree *referenceTree,
             const std::string &recoEventBranch,
             const std::string &eventInfoBranch)
      : inputTree(inputTree), referenceTree(referenceTree) {
    inputTree->SetBranchAddress(recoEventBranch.c_str(), &inputTrack);
    referenceTree->SetBranchAddress(recoEventBranch.c_str(), &referenceTrack);
    inputTree->SetBranchAddress(eventInfoBranch.c_str(), &inputInfo);
    referenceTree->SetBranchAddress(eventInfoBranch.c_str(), &referenceInfo);
    treeInfo = std::string() + "\t(" + inputTree->GetDirectory()->GetName() +
               " vs " + referenceTree->GetDirectory()->GetName() + " / " +
               referenceTree->GetName() + ')';
  }
  template <class T> void operator()(const T &lhs, const T &rhs) {
    auto entryIndex = boost::get<1>(lhs);
    loadEntry(inputTree, lhs);
    loadEntry(referenceTree, rhs);
    for (auto &check : checks) {
      check(entryIndex, inputInfo, inputTrack, referenceInfo, referenceTrack,
            treeInfo);
    }
  }
  void resetTreeInfo() { treeInfo.clear(); }
  void addCheck(std::function<void(int, eventraw::EventInfo *, Track3D *,
                                   eventraw::EventInfo *, Track3D *,
                                   std::string)> &&check) {
    checks.push_back(check);
  }

private:
  template <class T> void loadEntry(TTree *tree, const T &indices) {
    auto entry = boost::get<0>(indices);
    tree->GetEntry(entry);
  }
  TTree *inputTree;
  TTree *referenceTree;
  Track3D *inputTrack = new Track3D;
  Track3D *referenceTrack = new Track3D;
  eventraw::EventInfo *inputInfo = new eventraw::EventInfo;
  eventraw::EventInfo *referenceInfo = new eventraw::EventInfo;
  std::string treeInfo;
  std::vector<
      std::function<void(int, eventraw::EventInfo *, Track3D *,
                         eventraw::EventInfo *, Track3D *, std::string)>>
      checks;
};

class ExtraSink {
public:
  ExtraSink(TTree *tree) {
    treeInfo = std::string() + "\t(" + tree->GetDirectory()->GetName() + " / " +
               tree->GetName() + ")";
  }
  template <int N, class T>
  void operator()(const tpcreco::utilities::Dispatched<N, T> &operand) {
    if (disabled) {
      return;
    }
    auto translation = translateDispatched(operand);
    std::cout << translation.first << " > entry index "
              << boost::get<1>(operand.data) << "\t" << translation.second
              << treeInfo << '\n';
  }
  void resetTreeInfo() { treeInfo.clear(); }
  void disable(bool disable) { disabled = disable; }

private:
  template <class T>
  std::pair<char, std::string>
  translateDispatched(const tpcreco::utilities::Dispatched<0, T> &) {
    return std::make_pair('+', "extra");
  }
  template <class T>
  std::pair<char, std::string>
  translateDispatched(const tpcreco::utilities::Dispatched<1, T> &) {
    return std::make_pair('-', "missing");
  }
  std::string treeInfo;
  bool disabled = false;
};

class Analysis {
public:
  Analysis(std::string inputName, std::string referenceName)
      : inputFile(openFile(inputName)), referenceFile(openFile(referenceName)) {
    std::string treeName = "TPCRecoData";
    inputTree = openTree(inputFile, treeName.c_str());
    referenceTree = openTree(referenceFile, treeName.c_str());
    extras = std::make_unique<ExtraSink>(inputTree);
    details = std::make_unique<DetailSink>(inputTree, referenceTree,
                                           "RecoEvent", "EventInfo");
  }

  void run() {
    auto inputRange = tpcreco::utilities::getTreeIndexRange(inputTree);
    auto referenceRange = tpcreco::utilities::getTreeIndexRange(referenceTree);
    tpcreco::utilities::diffDispatcher(
        inputRange, referenceRange,
        [](const auto &lhs, const auto &rhs) {
          return boost::get<1>(lhs) < boost::get<1>(rhs);
        },
        [](const auto &lhs, const auto &rhs) {
          return boost::get<1>(lhs) == boost::get<1>(rhs);
        },
        *details, *extras);
  }

  ExtraSink *getExtraSink() { return extras.get(); }
  DetailSink *getDetailSink() { return details.get(); }

private:
  TFile *openFile(std::string name) {
    auto *file = TFile::Open(name.c_str(), "READ");
    if (!file) {
      throw std::logic_error("Can't open file " + name);
    }
    return file;
  }
  TTree *openTree(TFile *file, std::string name) {
    auto *tree = static_cast<TTree *>(file->Get(name.c_str()));
    if (!tree) {
      throw std::logic_error("No valid TTree " + name + " in file " +
                             file->GetName() + "\n");
    }
    std::string majorIndex = "EventInfo.runId";
    std::string minorIndex = "EventInfo.eventId";
    auto ret = tree->BuildIndex(majorIndex.c_str(), minorIndex.c_str());
    if (ret != tree->GetEntries()) {
      throw std::logic_error("Can't build index: \"" + majorIndex + "\",\"" +
                             minorIndex + "\" on tree " + tree->GetName() +
                             " in file " + tree->GetDirectory()->GetName() +
                             "\n");
    }
    return tree;
  }
  TFile *inputFile;
  TFile *referenceFile;
  TTree *inputTree;
  TTree *referenceTree;
  std::unique_ptr<ExtraSink> extras;
  std::unique_ptr<DetailSink> details;
};

} // namespace diff
} // namespace analysis
} // namespace tpcreco

#endif // TPCRECO_ANALYSIS_DIFF_ANALYSIS_H_