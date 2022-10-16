#ifndef TPCRECO_UTILITIES_TTREEOPS_H_
#define TPCRECO_UTILITIES_TTREEOPS_H_
#include <TFile.h>
#include <TTree.h>
#include <TTreeIndex.h>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/combine.hpp>
#include <stdexcept>
namespace tpcreco {
namespace utilities {

auto getTreeIndexRange(TTree *tree) {
  auto *treeIndex = static_cast<TTreeIndex *>(tree->GetTreeIndex());
  if (!treeIndex) {
    throw std::logic_error("Invalid TTreeIndex");
  }
  auto *indices = treeIndex->GetIndex();
  if (!treeIndex) {
    throw std::logic_error("Invalid TTreeIndex indices");
  }
  auto *values = treeIndex->GetIndexValuesMinor();
  if (!treeIndex) {
    throw std::logic_error("Invalid TTreeIndex values");
  }
  auto indicesRange = boost::make_iterator_range(
      indices, indices + tree->GetEntries()); // poor man's std/gsl/ranges::span
  auto valuesRange = boost::make_iterator_range(
      values, values + tree->GetEntries()); // poor man's std/gsl/ranges::span
  return boost::combine(indicesRange, valuesRange); // or views::zip
}

namespace {
auto unique_view() {
  return boost::adaptors::adjacent_filtered(
      [](const auto &lhs, const auto &rhs) {
        return boost::get<1>(lhs) != boost::get<1>(rhs);
      });
}

template <int N> auto element_view() {
  return boost::adaptors::transformed(
      [](const auto &entry) { return boost::get<N>(entry); });
}

} // namespace

template <class ZipRange> auto filterDuplicates(ZipRange &&range) {
  return range | boost::adaptors::reversed | unique_view() | element_view<0>();
}

auto filterDuplicates(TTree *tree) {
  return filterDuplicates(getTreeIndexRange(tree));
}

TTree *cloneUnique(TTree *tree, TFile *file) {
  if (file) {
    file->cd();
  }
  auto clonedTree = tree->CloneTree(0);
  for (auto entry : filterDuplicates(tree)) {
    tree->GetEntry(entry);
    clonedTree->Fill();
  }
  return clonedTree;
}

} // namespace utilities

} // namespace tpcreco
#endif // TPCRECO_UTILITIES_TTREEOPS_H_