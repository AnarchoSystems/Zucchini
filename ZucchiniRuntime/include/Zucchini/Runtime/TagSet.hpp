#pragma once

#include <bitset>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace nZucchini {
// Enum values are dense zero-based bit indices; out-of-range values are absent.
template <typename Enum, std::size_t N> class TagSet {
  static_assert(std::is_enum_v<Enum>, "TagSet requires an enum type");

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