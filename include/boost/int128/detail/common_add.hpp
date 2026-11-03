// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT128_DETAIL_COMMON_ADD_HPP
#define BOOST_INT128_DETAIL_COMMON_ADD_HPP

#include <boost/int128/detail/config.hpp>

#ifndef BOOST_INT128_BUILD_MODULE

#include <cstdint>

#endif

// The builtin 128-bit types and the overflow builtins keep loops over the two word types scalar
// on x86-64, because GCC and Clang only vectorize a carry that is spelled as a plain comparison.
// GCC before 14 does not turn that comparison back into adc/sbb inside loops, so it keeps the
// old paths. On ARM64 NEON holds only one 128-bit value per register and vectorizing loses
#if defined(__x86_64__) && !defined(_WIN32) && (defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 14))
#  define BOOST_INT128_HAS_VECTORIZABLE_ADD
#endif

namespace boost {
namespace int128 {
namespace detail {

// Carry out of the low word of a two word addition, where sum = a + b.
// GCC only emits adc for the comparison against the sum, while Clang scalarizes that same
// comparison when it vectorizes with AVX2 but handles the comparison of the inputs well
BOOST_INT128_HOST_DEVICE BOOST_INT128_FORCE_INLINE constexpr std::uint64_t add_carry_word(const std::uint64_t a, const std::uint64_t b, const std::uint64_t sum) noexcept
{
    #ifdef __clang__

    static_cast<void>(sum);
    return static_cast<std::uint64_t>(~a < b);

    #else

    static_cast<void>(b);
    return static_cast<std::uint64_t>(sum < a);

    #endif
}

// High word of the two's complement negation of (high, low); the low word is always 0 - low.
// Both spellings are branch free, and each is the faster one for its compiler and target
BOOST_INT128_HOST_DEVICE BOOST_INT128_FORCE_INLINE constexpr std::uint64_t negate_high_word(const std::uint64_t high, const std::uint64_t low) noexcept
{
    #if defined(__aarch64__) && defined(__clang__)

    return UINT64_C(0) - high - static_cast<std::uint64_t>(low != UINT64_C(0));

    #else

    return ~high + static_cast<std::uint64_t>(low == UINT64_C(0));

    #endif
}

} // namespace detail
} // namespace int128
} // namespace boost

#endif // BOOST_INT128_DETAIL_COMMON_ADD_HPP
