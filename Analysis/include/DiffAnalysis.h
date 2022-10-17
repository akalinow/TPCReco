#ifndef TPCRECO_ANALYSIS_DIFF_ANALYSIS_H_
#define TPCRECO_ANALYSIS_DIFF_ANALYSIS_H_
#include "EventInfo.h"
#include "TTreeOps.h"
#include "Track3D.h"
#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <memory>

class DetailSink {
public:
  DetailSink(TTree *inputTree, TTree *referenceTree)
      : inputTree(inputTree), referenceTree(referenceTree) {
    inputTree->SetBranchAddress("RecoEvent", &inputTrack);
    referenceTree->SetBranchAddress("RecoEvent", &referenceTrack);
    inputTree->SetBranchAddress("EventInfo", &inputInfo);
    referenceTree->SetBranchAddress("EventInfo", &referenceInfo);
  }
  template <class T> void operator()(const T &lhs, const T &rhs) {
    auto eventIndex = boost::get<1>(lhs);
    loadEntry(inputTree, lhs);
    loadEntry(referenceTree, rhs);
    auto inputSegments = inputTrack->getSegments().size();
    auto referenceSegments = referenceTrack->getSegments().size();
    if (inputSegments != referenceSegments) {
      std::cout << "s  > entry index " << eventIndex << " segments "
                << inputSegments << " vs " << referenceSegments << "\t("
                << inputTree->GetDirectory()->GetName() << " vs "
                << referenceTree->GetDirectory()->GetName() << " / "
                << referenceTree->GetName() << ")\n";
    }
    auto inputType = inputInfo->GetEventType().to_ulong();
    auto referenceType = referenceInfo->GetEventType().to_ulong();
    if (inputType != referenceType) {
      std::cout << "t > entry index " << eventIndex << " type " << inputType
                << " vs " << referenceType << "\t("
                << inputTree->GetDirectory()->GetName() << " vs "
                << referenceTree->GetDirectory()->GetName() << " / "
                << referenceTree->GetName() << ")\n";
    }
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
};

class ExtraSink {
public:
  ExtraSink(TTree *tree) : inputTree(tree) {}
  template <int N, class T>
  void operator()(const tpcreco::utilities::Dispatched<N, T> &operand) {
    auto translation = translateDispatched(operand);
    std::cout << translation.first << " > entry index "
              << boost::get<1>(operand.data) << "\t" << translation.second
              << "\t(" << inputTree->GetDirectory()->GetName() << " / "
              << inputTree->GetName() << ")\n";
  }

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
  TTree *inputTree;
};

class DiffAnalysis {
public:
  DiffAnalysis(std::string inputName, std::string referenceName)
      : inputFile(openFile(inputName)), referenceFile(openFile(referenceName)) {
    inputTree = openTree(inputFile, "TPCRecoData");
    referenceTree = openTree(referenceFile, "TPCRecoData");
    extras = std::make_unique<ExtraSink>(inputTree);
    details = std::make_unique<DetailSink>(inputTree, referenceTree);
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
      throw std::logic_error("No valid TTree " + name + " in input file " +
                             file->GetName() + "\n");
    }
    tree->BuildIndex("EventInfo.runId", "EventInfo.eventId");
    return tree;
  }
  TFile *inputFile;
  TFile *referenceFile;
  TTree *inputTree;
  TTree *referenceTree;
  std::unique_ptr<ExtraSink> extras;
  std::unique_ptr<DetailSink> details;
};

#endif // TPCRECO_ANALYSIS_DIFF_ANALYSIS_H_