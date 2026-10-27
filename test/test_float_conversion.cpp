// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Conversions between the 128-bit types and the built-in floating point types, in both
// directions.
//
// The portable words-to-float path is exercised directly rather than through the conversion
// operators, because on a platform that has a built-in 128-bit integer the operators take the
// compiler's own conversion instead. Testing it directly is what keeps the two paths in
// agreement, so that a value converts to the same bit pattern everywhere.

// Exact floating point comparison is the whole point of this file. The pragmas must cover
// lightweight_test.hpp so that its macros can be used on floating point values
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wfloat-equal"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4127)
#  pragma warning(disable : 4056)
#  pragma warning(disable : 4307)
#endif

#include <boost/int128.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstdint>
#include <cstring>
#include <limits>
#include <random>

using boost::int128::int128;
using boost::int128::uint128;
using boost::int128::detail::signed_words_to_float;
using boost::int128::detail::unsigned_words_to_float;

namespace {

// Compares value bytes, so that a NaN matches a NaN and -0.0 does not match 0.0. x87 long
// double carries padding bytes in the object, which must not take part in the comparison
template <typename Float>
bool same_bits(const Float lhs, const Float rhs) noexcept
{
    constexpr std::size_t value_bytes {std::numeric_limits<Float>::digits == 64 ? 10U : sizeof(Float)};

    unsigned char l[sizeof(Float)] {};
    unsigned char r[sizeof(Float)] {};
    std::memcpy(l, &lhs, sizeof(Float));
    std::memcpy(r, &rhs, sizeof(Float));

    return std::memcmp(l, r, value_bytes) == 0;
}

template <typename Float>
bool is_positive_infinity(const Float v) noexcept
{
    return v > (std::numeric_limits<Float>::max)();
}

// IBM double-double, the default long double on ppc64le, is the one format here that a bitwise
// comparison cannot be applied to. It is not a canonical format, its arithmetic is not
// correctly rounded, and "correctly rounded" is not uniquely defined for it, so the portable
// path and the compiler's own conversion break ties differently. Measured on ppc64le the two
// differ on 0.19% of values and never by more than half an ulp of the format, 2^-106.
// Nothing depends on this in practice: ppc64le always has a built-in 128-bit integer, so the
// conversion operators there use the compiler's conversion and never the portable path.
// Its 106 bit significand identifies it; every other format here has 24, 53, 64, or 113 bits
template <typename Float>
struct bitwise_reproducible
{
    static constexpr bool value {std::numeric_limits<Float>::digits != 106};
};

// Agreement far inside the last bits of the format. A double rounding, a truncated operand, or
// a mishandled sticky bit would all be off by very much more than this
// As above this is only needed for ppc64le ibm128

// LCOV_EXCL_START
template <typename Float>
bool nearly_same(const Float lhs, const Float rhs) noexcept
{
    if (lhs != lhs || rhs != rhs)
    {
        return (lhs != lhs) && (rhs != rhs);
    }

    if (lhs == rhs)
    {
        return true;
    }

    const Float diff {lhs > rhs ? lhs - rhs : rhs - lhs};
    const Float scale {lhs > Float{0} ? lhs : -lhs};

    return diff <= scale * static_cast<Float>(1e-30L);
}
// LCOV_EXCL_STOP

template <typename Float>
bool same_result(const Float lhs, const Float rhs) noexcept
{
    if (bitwise_reproducible<Float>::value)
    {
        return same_bits(lhs, rhs);
    }

    return nearly_same(lhs, rhs);
}

// =========================================================================
// Known answers for the rounding of the portable path, on every platform
// =========================================================================

// Composing the value as high * 2^64 + low rounds the high word, the low word, and the sum,
// which lands one ulp away from the correctly rounded result for roughly one value in 150.
// These are the two cases that pin the rounding down: an exact tie, which must round to even,
// and a tie plus one, which must round up
void test_double_rounding()
{
    constexpr double two_64 {18446744073709551616.0};
    constexpr double two_53 {9007199254740992.0};
    constexpr double two_117 {two_53 * two_64};   // exact
    constexpr double two_65 {2.0 * two_64};       // exact

    // high = 2^53 + 1 needs 54 bits, one more than a double significand holds
    constexpr std::uint64_t high {(UINT64_C(1) << 53) + UINT64_C(1)};

    // 2^117 + 2^64 is exactly halfway between 2^117 and 2^117 + 2^65, and 2^117 has the even
    // significand, so it wins the tie
    BOOST_TEST(same_bits(unsigned_words_to_float<double>(high, UINT64_C(0)), two_117));

    // One more than the halfway point must round away from it. Composing the words instead
    // produced 2^117 here, which was the defect
    BOOST_TEST(same_bits(unsigned_words_to_float<double>(high, UINT64_C(1)), two_117 + two_65));

    // The largest low word cannot push it any further than one step
    BOOST_TEST(same_bits(unsigned_words_to_float<double>(high, UINT64_MAX), two_117 + two_65));

    // The same shape for the signed path, where the magnitude is rounded and then negated
    BOOST_TEST(same_bits(signed_words_to_float<double>(static_cast<std::int64_t>(high), UINT64_C(0)), two_117));
    BOOST_TEST(same_bits(signed_words_to_float<double>(static_cast<std::int64_t>(high), UINT64_C(1)),
                         two_117 + two_65));

    // Negative values must round the magnitude, not the two's complement words
    BOOST_TEST(same_bits(signed_words_to_float<double>(-1, UINT64_MAX), -1.0));
    BOOST_TEST(same_bits(signed_words_to_float<double>(-1, UINT64_C(0)), -two_64));

    // A float significand is 24 bits, so any value above 2^24 rounds
    constexpr float two_64_f {18446744073709551616.0F};
    BOOST_TEST(same_bits(unsigned_words_to_float<float>(UINT64_C(1), UINT64_C(0)), two_64_f));
    BOOST_TEST(same_bits(unsigned_words_to_float<float>(UINT64_C(0), UINT64_C(1)), 1.0F));

    // Values that fit the significand exactly must be exact
    BOOST_TEST(same_bits(unsigned_words_to_float<double>(UINT64_C(0), UINT64_C(1)), 1.0));
    BOOST_TEST(same_bits(unsigned_words_to_float<double>(UINT64_C(0), UINT64_MAX),
                         two_64));   // 2^64 - 1 rounds up to 2^64
    BOOST_TEST(same_bits(unsigned_words_to_float<long double>(UINT64_C(1), UINT64_C(0)),
                         18446744073709551616.0L));
}

// =========================================================================
// Overflow of the 128-bit to floating point direction, on every platform
// =========================================================================

// 2^128 is above the largest finite float, so the top of the range has to round to infinity
// where the correctly rounded result would exceed FLT_MAX, and stay finite everywhere below
void test_float_overflow()
{
    constexpr float flt_max {(std::numeric_limits<float>::max)()};

    // (2^64 - 2^40) * 2^64 is exactly FLT_MAX, which is 2^128 - 2^104
    constexpr std::uint64_t flt_max_high {UINT64_C(0xFFFFFF0000000000)};
    BOOST_TEST(same_bits(unsigned_words_to_float<float>(flt_max_high, UINT64_C(0)), flt_max));
    BOOST_TEST(!is_positive_infinity(unsigned_words_to_float<float>(flt_max_high, UINT64_C(0))));

    // (2^64 - 2^39) * 2^64 is exactly 2^128 - 2^103, the halfway point between FLT_MAX and
    // 2^128. FLT_MAX has the odd significand, so the tie goes up and overflows to infinity
    constexpr std::uint64_t tie_high {UINT64_C(0xFFFFFF8000000000)};
    BOOST_TEST(is_positive_infinity(unsigned_words_to_float<float>(tie_high, UINT64_C(0))));

    // One below that halfway point must stay finite
    BOOST_TEST(same_bits(unsigned_words_to_float<float>(tie_high - UINT64_C(1), UINT64_MAX), flt_max));

    // UINT128_MAX is above the halfway point, so it overflows as well
    BOOST_TEST(is_positive_infinity(unsigned_words_to_float<float>(UINT64_MAX, UINT64_MAX)));

    // The signed extremes are below 2^127 and stay finite, with the sign carried through
    constexpr float two_127_f {170141183460469231731687303715884105728.0F};
    BOOST_TEST(same_bits(signed_words_to_float<float>(static_cast<std::int64_t>(UINT64_C(0x8000000000000000)),
                                                      UINT64_C(0)),
                         -two_127_f));
    BOOST_TEST(!is_positive_infinity(signed_words_to_float<float>(INT64_MAX, UINT64_MAX)));

    // double and long double have the range to hold every 128-bit value, so nothing overflows
    BOOST_TEST(!is_positive_infinity(unsigned_words_to_float<double>(UINT64_MAX, UINT64_MAX)));
    BOOST_TEST(!is_positive_infinity(unsigned_words_to_float<long double>(UINT64_MAX, UINT64_MAX)));
}

// =========================================================================
// Overflow of the floating point to 128-bit direction, on every platform
// =========================================================================

// The built-in cast is undefined outside the range of the target type, and what it produces
// varies by architecture. The library is total instead: it saturates, on every platform
template <typename Float>
void test_from_float_saturation()
{
    const Float nan {std::numeric_limits<Float>::quiet_NaN()};
    const Float inf {std::numeric_limits<Float>::infinity()};

    const uint128 u_max {~static_cast<std::uint64_t>(0), ~static_cast<std::uint64_t>(0)};
    const int128 i_max {INT64_MAX, UINT64_MAX};
    const int128 i_min {static_cast<std::int64_t>(UINT64_C(0x8000000000000000)), UINT64_C(0)};

    // NaN yields zero for both types
    BOOST_TEST_EQ(uint128{nan}, uint128{0});
    BOOST_TEST_EQ(int128{nan}, int128{0});

    // Negative values cannot be represented by uint128 and yield zero
    BOOST_TEST_EQ(uint128{static_cast<Float>(-1)}, uint128{0});
    BOOST_TEST_EQ(uint128{-inf}, uint128{0});
    BOOST_TEST_EQ(uint128{static_cast<Float>(-0.5)}, uint128{0});

    // Above the range, both saturate
    BOOST_TEST_EQ(uint128{inf}, u_max);
    BOOST_TEST_EQ(int128{inf}, i_max);
    BOOST_TEST_EQ(int128{-inf}, i_min);

    // 2^128 is the first value that does not fit a uint128, and 2^127 the first for an int128
    const Float two_64 {static_cast<Float>(18446744073709551616.0L)};
    const Float two_127 {two_64 * static_cast<Float>(9223372036854775808.0L)};

    BOOST_TEST_EQ(int128{two_127}, i_max);
    BOOST_TEST_EQ(int128{-two_127}, i_min);       // -2^127 is exactly INT128_MIN

    // For float, 2^128 is already infinity, so only the wider types can name it
    if (std::numeric_limits<Float>::max_exponent > 128)
    {
        BOOST_TEST_EQ(uint128{two_127 * static_cast<Float>(2)}, u_max);
    }

    // Just inside the range must not saturate: the largest value the type can name below the
    // boundary has to convert to itself
    const Float below_128 {two_127 * static_cast<Float>(2) == std::numeric_limits<Float>::infinity()
                               ? (std::numeric_limits<Float>::max)()
                               : two_127};
    BOOST_TEST(uint128{below_128} != u_max || below_128 == (std::numeric_limits<Float>::max)());

    // Truncation toward zero, in both directions
    BOOST_TEST_EQ(uint128{static_cast<Float>(3.75)}, uint128{3});
    BOOST_TEST_EQ(int128{static_cast<Float>(-3.75)}, int128{-3});
    BOOST_TEST_EQ(int128{static_cast<Float>(0.9)}, int128{0});
    BOOST_TEST_EQ(int128{static_cast<Float>(-0.9)}, int128{0});

    // A word at or above 2^63 is the case a compiler reaches through its unsigned conversion
    // path, so it needs a known answer of its own. Every bit of the significand is set here,
    // capped at the 64 bits a word holds, which every format names exactly
    constexpr int word_bits {std::numeric_limits<Float>::digits < 64 ? std::numeric_limits<Float>::digits : 64};

    Float dense {static_cast<Float>(9223372036854775808.0L)};     // 2^63
    Float step {dense};
    std::uint64_t expected {UINT64_C(1) << 63};

    for (int i {1}; i < word_bits; ++i)
    {
        step /= static_cast<Float>(2);
        dense += step;
        expected |= UINT64_C(1) << (63 - i);
    }

    BOOST_TEST_EQ(uint128{dense}, uint128{expected});
    BOOST_TEST_EQ(int128{dense}, int128{expected});
    BOOST_TEST_EQ(int128{-dense}, -int128{expected});
}

// =========================================================================
// constexpr
// =========================================================================

constexpr double c_two_64 {18446744073709551616.0};
constexpr double c_two_53 {9007199254740992.0};

static_assert(unsigned_words_to_float<double>((UINT64_C(1) << 53) + UINT64_C(1), UINT64_C(0))
                  == c_two_53 * c_two_64,
              "the tie must round to even at compile time as well");

static_assert(unsigned_words_to_float<double>((UINT64_C(1) << 53) + UINT64_C(1), UINT64_C(1))
                  == c_two_53 * c_two_64 + 2.0 * c_two_64,
              "one past the tie must round up at compile time as well");

static_assert(unsigned_words_to_float<double>(UINT64_C(1), UINT64_C(0)) == c_two_64,
              "2^64 must be exact");

static_assert(signed_words_to_float<double>(-1, UINT64_MAX) == -1.0, "int128{-1} must be -1.0");

// The overflow of a 128-bit value to an infinite float is only checked at runtime, in
// test_float_overflow. GCC rejects a floating point operation that overflows to infinity as
// non-constant whatever form it takes, so no formula can assert it at compile time

} // namespace

// =========================================================================
// Parity with the built-in conversion
// =========================================================================

#ifdef BOOST_INT128_HAS_INT128

namespace {

using boost::int128::detail::builtin_i128;
using boost::int128::detail::builtin_u128;

std::mt19937_64 rng {42};

// The portable path must agree with the compiler's own conversion bit for bit, and so must the
// conversion operator, which is what makes a value convert identically on every platform
template <typename Float>
void check_value(const builtin_u128 raw)
{
    const auto high = static_cast<std::uint64_t>(raw >> 64);
    const auto low = static_cast<std::uint64_t>(raw);

    const Float oracle {static_cast<Float>(raw)};

    BOOST_TEST(same_result(unsigned_words_to_float<Float>(high, low), oracle));
    BOOST_TEST(same_result(static_cast<Float>(uint128{raw}), oracle));

    const auto signed_raw = static_cast<builtin_i128>(raw);
    const int128 lib {signed_raw};
    const Float signed_oracle {static_cast<Float>(signed_raw)};

    BOOST_TEST(same_result(signed_words_to_float<Float>(lib.signed_high(), lib.low), signed_oracle));
    BOOST_TEST(same_result(static_cast<Float>(lib), signed_oracle));
}

// Every magnitude class, with the discarded bits placed exactly on, just below, and just above
// the rounding boundary, which is where a tie or sticky bit mistake shows up
template <typename Float>
void test_rounding_boundaries()
{
    constexpr int digits {std::numeric_limits<Float>::digits};

    for (int shift {0}; shift < 64; ++shift)
    {
        const int exponent {128 - digits - shift};
        if (exponent <= 0)
        {
            continue;
        }

        const builtin_u128 unit {static_cast<builtin_u128>(1) << exponent};
        const builtin_u128 half {static_cast<builtin_u128>(1) << (exponent - 1)};

        // Held as a builtin_u128 because a long double significand can be wider than 64 bits:
        // 113 for IEEE binary128 and 106 for IBM double-double
        const builtin_u128 top_bit {static_cast<builtin_u128>(1) << (digits - 1)};

        const builtin_u128 significands[] {
            top_bit,
            top_bit + 1,
            (top_bit << 1) - 2,
            (top_bit << 1) - 1
        };

        for (const builtin_u128 significand : significands)
        {
            const builtin_u128 base {significand << exponent};

            check_value<Float>(base);
            check_value<Float>(base + 1);
            check_value<Float>(base + half - 1);
            check_value<Float>(base + half);          // the tie
            check_value<Float>(base + half + 1);
            check_value<Float>(base + unit - 1);
        }
    }
}

template <typename Float>
void test_random_parity()
{
    constexpr std::size_t N {4096U};

    for (std::size_t i {0}; i < N; ++i)
    {
        builtin_u128 raw {(static_cast<builtin_u128>(rng()) << 64) | static_cast<builtin_u128>(rng())};
        raw >>= (rng() % 128U);
        check_value<Float>(raw);
    }
}

// The other direction, restricted to the range where the built-in cast is defined
template <typename Float>
void test_from_float_parity()
{
    constexpr std::size_t N {4096U};
    const Float two_128 {static_cast<Float>(340282366920938463463374607431768211456.0L)};

    for (std::size_t i {0}; i < N; ++i)
    {
        builtin_u128 raw {(static_cast<builtin_u128>(rng()) << 64) | static_cast<builtin_u128>(rng())};
        raw >>= (rng() % 128U);

        const Float f {static_cast<Float>(raw)};
        if (!(f >= Float{0}) || f >= two_128)
        {
            continue;   // the cast back would be undefined for the built-in
        }

        BOOST_TEST_EQ(uint128{f}, uint128{static_cast<builtin_u128>(f)});

        // And the signed direction inside its own range
        const Float g {f / static_cast<Float>(4)};
        BOOST_TEST_EQ(int128{g}, int128{static_cast<builtin_i128>(g)});
        BOOST_TEST_EQ(int128{-g}, int128{static_cast<builtin_i128>(-g)});
    }
}

} // namespace

#endif // BOOST_INT128_HAS_INT128

int main()
{
    test_double_rounding();
    test_float_overflow();

    test_from_float_saturation<float>();
    test_from_float_saturation<double>();
    test_from_float_saturation<long double>();

    #ifdef BOOST_INT128_HAS_INT128

    test_rounding_boundaries<float>();
    test_rounding_boundaries<double>();
    test_rounding_boundaries<long double>();

    test_random_parity<float>();
    test_random_parity<double>();
    test_random_parity<long double>();

    test_from_float_parity<float>();
    test_from_float_parity<double>();
    test_from_float_parity<long double>();

    #endif // BOOST_INT128_HAS_INT128

    return boost::report_errors();
}

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif
