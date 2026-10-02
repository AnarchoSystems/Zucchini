#pragma once

#include <bitset>
#include <cstddef>
#include <utility>

namespace nZucchini {
template <typename Enum, std::size_t N> class TagSet {
public:
  explicit TagSet(std::bitset<N> bits) : bits(std::move(bits)) {}

  bool contains(Enum value) const {
    const auto index = static_cast<std::size_t>(value);
    return index < N && bits.test(index);
  }

private:
  std::bitset<N> bits;
};
} // namespace nZucchini