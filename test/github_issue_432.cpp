// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/int128/issues/432
//
// The float conversion operators had two defects:
//
// 1) The high word was scaled by UINT64_MAX (2^64 - 1) instead of 2^64. float and
//    double round 2^64 - 1 up to 2^64, which hid the error, but any type with 64 or
//    more significand bits (x87 80-bit and IEEE quad long double) holds 2^64 - 1
//    exactly, so every conversion with a non-zero high word was off by the value of
//    the high word. For example uint128{1, 0} (2^64) converted to 2^64 - 1.
//
// 2) int128 fed its raw two's complement words into high * scale + low. That
//    identity is exact in integer arithmetic but not in floating point: a small
//    negative value stores low close to 2^64, which rounds to exactly 2^64 whenever
//    the type has fewer than 64 significand bits, and adding the scaled high word
//    then cancels catastrophically. int128{-1} converted to 0.0 instead of -1.0.
//
// The operators now use the builtin 128-bit conversion where one exists, and the
// portable fallback scales by an exact 2^64 and converts negative values through
// the unsigned magnitude. Both paths are covered below.

// The pragmas must cover lightweight_test.hpp so that BOOST_TEST_EQ can be
// used on floating point values, which is the entire point of this test
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wfloat-equal"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#include <boost/int128.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

using boost::int128::int128;
using boost::int128::uint128;

// Exactly representable values must convert exactly, so a defect either produces
// the exact expected value or it is a bug. No tolerances are used anywhere here.

// Defect 2 regression: small negative values used to cancel to 0.0
template <typename T>
void test_small_negative_values()
{
    for (int v {-1}; v >= -1024; --v)
    {
        const int128 value {v};
        BOOST_TEST_EQ(static_cast<T>(value), static_cast<T>(v));

        // The portable fallback must also be correct on platforms
        // where the operator uses the builtin conversion instead
        BOOST_TEST_EQ(boost::int128::detail::signed_words_to_float<T>(value.signed_high(), value.low), static_cast<T>(v));
    }
}

// Every power of two is exactly representable in every binary floating point type
template <typename T>
void test_signed_powers_of_two()
{
    for (int k {0}; k < 127; ++k)
    {
        const int128 value {-(int128{1} << k)};
        const T expected {-std::ldexp(static_cast<T>(1), k)};

        BOOST_TEST_EQ(static_cast<T>(value), expected);
        BOOST_TEST_EQ(boost::int128::detail::signed_words_to_float<T>(value.signed_high(), value.low), expected);
    }

    // INT128_MIN itself: the magnitude 2^127 does not fit in int128,
    // so this exercises the negation edge case in the fallback
    const auto min_value {(std::numeric_limits<int128>::min)()};
    const T expected_min {-std::ldexp(static_cast<T>(1), 127)};

    BOOST_TEST_EQ(static_cast<T>(min_value), expected_min);
    BOOST_TEST_EQ(boost::int128::detail::signed_words_to_float<T>(min_value.signed_high(), min_value.low), expected_min);

    // This fails with 32-bit ASAN but passes on the same compiler without ASAN
    // libs/int128/test/github_issue_432.cpp(86): test 'static_cast<T>(min_value + 1) == expected_min' ('-1.70141e+38' == '-1.70141e+38') failed in function 'void test_signed_powers_of_two() [with T = float]'
    // libs/int128/test/github_issue_432.cpp(86): test 'static_cast<T>(min_value + 1) == expected_min' ('-1.70141e+38' == '-1.70141e+38') failed in function 'void test_signed_powers_of_two() [with T = double]'
    // 2 errors detected.
    #if !defined(ASAN) && !defined(__SANITIZE_ADDRESS__)
    // INT128_MIN + 1 has magnitude 2^127 - 1 which correctly rounds
    // to 2^127 in every format with fewer than 127 significand bits
    BOOST_TEST_EQ(static_cast<T>(min_value + 1), expected_min);
    #endif
}

// Defect 1 regression: with the 2^64 - 1 scale factor every value with a non-zero
// high word was off by the high word wherever long double holds 2^64 - 1 exactly
template <typename T>
void test_offset_exactness()
{
    const T two_64 {std::ldexp(static_cast<T>(1), 64)};

    for (std::uint64_t k {1}; k <= 64; ++k)
    {
        BOOST_TEST_EQ(static_cast<T>(uint128{k, 0}), static_cast<T>(k) * two_64);
        BOOST_TEST_EQ(boost::int128::detail::unsigned_words_to_float<T>(k, 0), static_cast<T>(k) * two_64);

        const int128 negative {-int128{static_cast<std::int64_t>(k), 0}};
        BOOST_TEST_EQ(static_cast<T>(negative), -(static_cast<T>(k) * two_64));
    }

    BOOST_TEST_EQ(static_cast<T>(uint128{UINT64_C(1) << 63, 0}), std::ldexp(static_cast<T>(1), 127));

    // -(2^64 - 1): exact wherever the significand holds 64 bits, and the computed
    // expected value rounds identically to the conversion everywhere else
    const int128 value {-1, 1};
    BOOST_TEST_EQ(static_cast<T>(value), -(two_64 - static_cast<T>(1)));
}

#if defined(BOOST_INT128_HAS_INT128)

using builtin_i128 = boost::int128::detail::builtin_i128;
using builtin_u128 = boost::int128::detail::builtin_u128;

static constexpr std::size_t N {1024};

// The portable fallback rounds the 128-bit value once, to nearest with ties to even, so it
// agrees with the builtin conversion exactly. IBM double-double is the exception: it is not a
// canonical format and "correctly rounded" is not uniquely defined for it, so the two round
// ties differently, by at most half an ulp of its 106 bit significand. std::nextafter is not
// meaningful for that format either, hence the relative comparison
template <typename T>
bool matches_builtin(const T computed, const T reference)
{
    if (computed == reference)
    {
        return true;
    }

    if (std::numeric_limits<T>::digits == 106)
    {
        const T diff {computed > reference ? computed - reference : reference - computed};
        const T scale {reference > T{0} ? reference : -reference};

        return diff <= scale * static_cast<T>(1e-30L);
    }

    return false;
}

template <typename T>
void test_vs_builtin()
{
    std::mt19937_64 rng {42};

    for (std::size_t i {}; i < N; ++i)
    {
        const auto hi {rng()};
        const auto lo {rng()};

        const uint128 u {hi, lo};
        const auto builtin_u {(static_cast<builtin_u128>(hi) << 64) | static_cast<builtin_u128>(lo)};

        const int128 s {static_cast<std::int64_t>(hi), lo};
        const auto builtin_s {static_cast<builtin_i128>(builtin_u)};

        // The operators use the builtin conversion on this platform, so these are exact
        BOOST_TEST_EQ(static_cast<T>(u), static_cast<T>(builtin_u));
        BOOST_TEST_EQ(static_cast<T>(s), static_cast<T>(builtin_s));

        // The fallback rounds the 128-bit value once, so it agrees with the builtin exactly.
        // The old implementation was off by the whole high word or returned 0.0 for
        // negative values, which this catches with a huge margin
        BOOST_TEST(matches_builtin(boost::int128::detail::unsigned_words_to_float<T>(hi, lo),
                                   static_cast<T>(builtin_u)));
        BOOST_TEST(matches_builtin(boost::int128::detail::signed_words_to_float<T>(s.signed_high(), s.low),
                                   static_cast<T>(builtin_s)));

        // The fallback must never lose the sign the way the cancellation defect did
        if (builtin_s != 0)
        {
            BOOST_TEST_EQ(boost::int128::detail::signed_words_to_float<T>(s.signed_high(), s.low) < 0, builtin_s < 0);
        }
    }
}

#endif // BOOST_INT128_HAS_INT128

// Both conversion paths are constexpr, so the regressions are also pinned at compile time
static_assert(static_cast<float>(int128{-1}) == -1.0f, "int128{-1} must convert to -1.0f");
static_assert(static_cast<double>(int128{-1}) == -1.0, "int128{-1} must convert to -1.0");
static_assert(static_cast<double>(int128{-1024}) == -1024.0, "small negatives must not cancel to 0");
static_assert(static_cast<double>(uint128{1, 0}) == 18446744073709551616.0, "uint128 2^64 must convert to 2^64");

static_assert(boost::int128::detail::signed_words_to_float<double>(-1, UINT64_MAX) == -1.0,
              "fallback conversion of int128{-1} must yield -1.0");
static_assert(boost::int128::detail::unsigned_words_to_float<double>(1, 0) == 18446744073709551616.0,
              "fallback conversion of 2^64 must yield 2^64");

#if !defined(BOOST_INT128_HAS_GPU_SUPPORT)

static_assert(static_cast<long double>(int128{-1}) == -1.0L, "int128{-1} must convert to -1.0L");
static_assert(static_cast<long double>(uint128{1, 0}) == 18446744073709551616.0L,
              "uint128 2^64 must convert to 2^64 exactly, not 2^64 - 1");
static_assert(static_cast<long double>(int128{-1, 1}) == -(18446744073709551616.0L - 1.0L),
              "int128 -(2^64 - 1) must match the correctly rounded value");

#endif

int main()
{
    test_small_negative_values<float>();
    test_small_negative_values<double>();
    test_small_negative_values<long double>();

    test_signed_powers_of_two<float>();
    test_signed_powers_of_two<double>();
    test_signed_powers_of_two<long double>();

    test_offset_exactness<float>();
    test_offset_exactness<double>();
    test_offset_exactness<long double>();

    #if defined(BOOST_INT128_HAS_INT128)

    test_vs_builtin<float>();
    test_vs_builtin<double>();
    test_vs_builtin<long double>();

    #endif

    return boost::report_errors();
}

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif
