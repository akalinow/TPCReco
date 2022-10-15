#ifndef TPCRECO_UTILITIES_TTREEOPS_H_
#define TPCRECO_UTILITIES_TTREEOPS_H_
#include <TTree.h>
#include <TTreeIndex.h>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/combine.hpp>

namespace tpcreco {
namespace utilities {

auto getTreeIndexRange(TTree *tree) {
  auto *treeIndex = static_cast<TTreeIndex *>(tree->GetTreeIndex());
  auto *indices = treeIndex->GetIndex();
  auto *values = treeIndex->GetIndexValuesMinor();
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

template<int N>
auto element_view() {
  return boost::adaptors::transformed(
      [](const auto &entry) { return boost::get<N>(entry); });
}

} // namespace

template <class ZipRange> auto filterDuplicates(ZipRange &&range) {
  return range | boost::adaptors::reversed | unique_view() |
         element_view<0>();
}

auto filterDuplicates(TTree *tree) {
  return filterDuplicates(getTreeIndexRange(tree));
}

} // namespace utilities

} // namespace tpcreco
#endif // TPCRECO_UTILITIES_TTREEOPS_H_