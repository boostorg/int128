// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// C++23 <stdfloat> extended floating point types (std::float16_t, float32_t, float64_t,
// float128_t, bfloat16_t) mixed with uint128/int128: construction, conversion, mixed
// arithmetic, comparisons, compound assignment, and the deleted operators.
//
// This file compiles to a no-op before C++23, or on a standard library that ships
// <stdfloat> but not a given extended type. Never gate any of this on
// __cpp_lib_stdfloat: libstdc++ (as of gcc-16) does not define that macro even though
// <stdfloat> and the types both exist. Use the compiler-predefined __STDCPP_..._T__
// macros instead, one per type.

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
#include <cstring>
#include <limits>
#include <type_traits>

#ifdef __has_include
#  if (__cplusplus > 202002L || (defined(_MSVC_LANG) && _MSVC_LANG > 202002L)) && __has_include(<stdfloat>)
#    include <stdfloat>
#  endif
#endif

// See test_float_conversion.cpp (the same rationale applies verbatim): a 32-bit x87
// build can keep an intermediate in an 80-bit register, and -ffast-math lets the
// optimizer assume neither infinity nor NaN occurs.
#ifndef BOOST_INT128_TEST_HONORS_INF_NAN
#  if defined(__FAST_MATH__)
#    define BOOST_INT128_TEST_HONORS_INF_NAN 0
#  elif defined(__FINITE_MATH_ONLY__) && (__FINITE_MATH_ONLY__ == 1)
#    define BOOST_INT128_TEST_HONORS_INF_NAN 0
#  elif defined(__INTEL_LLVM_COMPILER) && defined(__NO_MATH_ERRNO__)
#    define BOOST_INT128_TEST_HONORS_INF_NAN 0
#  else
#    define BOOST_INT128_TEST_HONORS_INF_NAN 1
#  endif
#endif

using boost::int128::uint128;
using boost::int128::int128;

namespace
{

template <typename T>
T opaque(T v) noexcept
{
    volatile T stored {v};
    return stored;
}

template <typename T>
bool same_bits(T lhs, T rhs) noexcept
{
    const T rounded_lhs {opaque(lhs)};
    const T rounded_rhs {opaque(rhs)};

    unsigned char lhs_bytes[sizeof(T)] {};
    unsigned char rhs_bytes[sizeof(T)] {};
    std::memcpy(lhs_bytes, &rounded_lhs, sizeof(T));
    std::memcpy(rhs_bytes, &rounded_rhs, sizeof(T));

    return std::memcmp(lhs_bytes, rhs_bytes, sizeof(lhs_bytes)) == 0;
}

// An independent reference for (high, low) -> T, for any T whose significand is at
// most 64 bits (every currently defined <stdfloat> type except std::float128_t).
// Keeps the top digits(T) bits of the 128-bit magnitude, folds everything below into
// a guard and a sticky bit, rounds to nearest with ties to even, and scales the exact
// kept integer by an exact power of two -- built from nothing the library itself uses.
// The final T() narrows a double that is already exactly the correctly rounded value
// (or exactly the overflow threshold), so that narrowing is not a second rounding.
template <typename T>
T reference_to_float(const std::uint64_t high, const std::uint64_t low)
{
    static_assert(std::numeric_limits<T>::digits <= 64, "use the exact-value-only path for float128_t");

    int high_bit {-1};
    if (high != 0U)
    {
        for (int b {63}; b >= 0; --b)
        {
            if ((high >> b) & 1U) { high_bit = 64 + b; break; }
        }
    }
    else if (low != 0U)
    {
        for (int b {63}; b >= 0; --b)
        {
            if ((low >> b) & 1U) { high_bit = b; break; }
        }
    }
    else
    {
        return T{0};
    }

    constexpr int digits {std::numeric_limits<T>::digits};

    if (high_bit < digits)
    {
        return static_cast<T>(low);
    }

    if (high_bit + 1 > std::numeric_limits<T>::max_exponent)
    {
        return std::numeric_limits<T>::infinity();
    }

    const int drop {high_bit - digits + 1};

    const auto bit_at = [&](int pos) -> bool
    {
        return pos < 64 ? ((low >> pos) & 1U) != 0U : ((high >> (pos - 64)) & 1U) != 0U;
    };

    std::uint64_t kept {0};
    for (int b {0}; b < digits; ++b)
    {
        if (bit_at(drop + b))
        {
            kept |= (UINT64_C(1) << b);
        }
    }

    const bool guard {bit_at(drop - 1)};
    bool sticky {false};
    for (int b {0}; b < drop - 1; ++b)
    {
        if (bit_at(b)) { sticky = true; break; }
    }

    if (guard && (sticky || ((kept & 1U) != 0U)))
    {
        ++kept;
    }

    return static_cast<T>(std::ldexp(static_cast<double>(kept), drop));
}

template <typename T>
void check_to_float(const uint128 v, const char* what)
{
    const T got {opaque(static_cast<T>(v))};
    const T expected {opaque(reference_to_float<T>(v.high, v.low))};

    if (!same_bits(got, expected))
    {
        std::fprintf(stderr, "mismatch in %s: got=%.20Lf expected=%.20Lf\n",
                      what, static_cast<long double>(got), static_cast<long double>(expected));
    }

    BOOST_TEST(same_bits(got, expected));
}

template <typename T>
void check_signed_to_float(const int128 v, const char* what)
{
    const T got {opaque(static_cast<T>(v))};

    const bool negative {v.signed_high() < 0};
    const uint128 mag {negative ? static_cast<uint128>(-v) : static_cast<uint128>(v)};
    const T mag_ref {reference_to_float<T>(mag.high, mag.low)};
    const T expected {opaque(negative ? -mag_ref : mag_ref)};

    if (!same_bits(got, expected))
    {
        std::fprintf(stderr, "mismatch in %s: got=%.20Lf expected=%.20Lf\n",
                      what, static_cast<long double>(got), static_cast<long double>(expected));
    }

    BOOST_TEST(same_bits(got, expected));
}

// SFINAE probe: true if `Lib op= T` is well formed
template <typename Lib, typename T, typename = void>
struct has_mod_assign : std::false_type {};
template <typename Lib, typename T>
struct has_mod_assign<Lib, T, decltype((void)(std::declval<Lib&>() %= std::declval<T>()))> : std::true_type {};

template <typename Lib, typename T, typename = void>
struct has_and_assign : std::false_type {};
template <typename Lib, typename T>
struct has_and_assign<Lib, T, decltype((void)(std::declval<Lib&>() &= std::declval<T>()))> : std::true_type {};

template <typename Lib, typename T, typename = void>
struct has_shl_assign : std::false_type {};
template <typename Lib, typename T>
struct has_shl_assign<Lib, T, decltype((void)(std::declval<Lib&>() <<= std::declval<T>()))> : std::true_type {};

template <typename T>
void test_deleted_ops()
{
    static_assert(!has_mod_assign<uint128, T>::value, "uint128 %= T must stay deleted");
    static_assert(!has_and_assign<uint128, T>::value, "uint128 &= T must stay deleted");
    static_assert(!has_shl_assign<uint128, T>::value, "uint128 <<= T must stay deleted");
    static_assert(!has_mod_assign<int128, T>::value, "int128 %= T must stay deleted");
    static_assert(!has_and_assign<int128, T>::value, "int128 &= T must stay deleted");
    static_assert(!has_shl_assign<int128, T>::value, "int128 <<= T must stay deleted");
}

template <typename T>
void test_construction(const char* name)
{
    // Exact small integer
    BOOST_TEST_EQ(uint128{T{7}}, uint128{7});
    BOOST_TEST_EQ(int128{T{-7}}, int128{-7});

    // Truncation toward zero of a non-integral value
    BOOST_TEST_EQ(uint128{static_cast<T>(12.75)}, uint128{12});
    BOOST_TEST_EQ(int128{static_cast<T>(-12.75)}, int128{-12});

    // Negative into unsigned gives zero
    BOOST_TEST_EQ(uint128{T{-5}}, uint128{0});

    // NaN into either gives zero (gated: -ffast-math can fold this away)
    #if BOOST_INT128_TEST_HONORS_INF_NAN
    const T nan_val {std::numeric_limits<T>::quiet_NaN()};
    BOOST_TEST_EQ(uint128{nan_val}, uint128{0});
    BOOST_TEST_EQ(int128{nan_val}, int128{0});
    #endif

    // Saturation: +infinity into unsigned saturates to max, into signed to INT128_MAX;
    // -infinity into signed saturates to INT128_MIN
    #if BOOST_INT128_TEST_HONORS_INF_NAN
    const T inf_val {std::numeric_limits<T>::infinity()};
    BOOST_TEST_EQ(uint128{inf_val}, (std::numeric_limits<uint128>::max)());
    BOOST_TEST_EQ(int128{inf_val}, (std::numeric_limits<int128>::max)());
    BOOST_TEST_EQ(int128{-inf_val}, (std::numeric_limits<int128>::min)());
    #endif

    // The largest finite value of T constructs without UB, whatever its magnitude
    // relative to 128 bits. float16_t's max (65504) fits inside uint128 exactly, so
    // that round trip must be exact; every wider type's max vastly exceeds 2^128, so
    // construction from it must saturate to the max representable uint128/int128
    // instead (it is not a truncation once the magnitude no longer fits at all)
    const T max_finite {(std::numeric_limits<T>::max)()};
    if (std::numeric_limits<T>::max_exponent <= 128)
    {
        const auto as_u128 {uint128{max_finite}};
        const auto back {static_cast<T>(as_u128)};
        BOOST_TEST(same_bits(opaque(back), opaque(max_finite)));
    }
    else
    {
        BOOST_TEST_EQ(uint128{max_finite}, (std::numeric_limits<uint128>::max)());
        BOOST_TEST_EQ(int128{max_finite}, (std::numeric_limits<int128>::max)());
    }

    (void)name;
}

template <typename T>
void test_conversion_narrow(const char* name)
{
    check_to_float<T>(uint128{0, 0}, name);
    check_to_float<T>(uint128{0, 1}, name);
    check_to_float<T>(uint128{0, 100}, name);
    check_to_float<T>(uint128{0, 65504}, name); // float16_t's max finite value
    check_to_float<T>(uint128{0, UINT64_C(1000000)}, name); // beyond float16_t's range
    check_to_float<T>(uint128{0, UINT64_MAX}, name);
    check_to_float<T>(uint128{1, 0}, name);
    check_to_float<T>(uint128{UINT64_MAX, UINT64_MAX}, name);

    // A halfway case for a type with `digits` bits: 2^digits + 2^(digits-1) has its
    // guard bit set and nothing below it, so ties-to-even decides the direction
    constexpr int digits {std::numeric_limits<T>::digits};
    const std::uint64_t halfway_case {(UINT64_C(1) << digits) | (UINT64_C(1) << (digits - 1))};
    check_to_float<T>(uint128{0, halfway_case}, name);

    check_signed_to_float<T>(int128{-1}, name);
    check_signed_to_float<T>(int128{static_cast<std::int64_t>(-65504)}, name);
    check_signed_to_float<T>((std::numeric_limits<int128>::min)(), name);
}

template <typename T>
void test_mixed_ops(const char* name)
{
    const uint128 u {UINT64_C(0), UINT64_C(5)};
    const int128 i {-5};
    const T half {T{1} / T{2}};

    {
        const auto r {u + half};
        static_assert(std::is_same<decltype(r), const T>::value, "result type must be T");
        BOOST_TEST(same_bits(opaque(r), opaque(T{5} + half)));
    }
    {
        const auto r {half + u};
        static_assert(std::is_same<decltype(r), const T>::value, "result type must be T");
        BOOST_TEST(same_bits(opaque(r), opaque(half + T{5})));
    }
    {
        const auto r {i + half};
        static_assert(std::is_same<decltype(r), const T>::value, "result type must be T");
        BOOST_TEST(same_bits(opaque(r), opaque(T{-5} + half)));
    }

    BOOST_TEST(u < T{10});
    BOOST_TEST(!(T{10} < u));
    BOOST_TEST(i < T{0});

    const auto ord {u <=> T{5}};
    static_assert(std::is_same<decltype(ord), const std::partial_ordering>::value, "must be partial_ordering");
    BOOST_TEST(ord == std::partial_ordering::equivalent);

    // Compound assignment on an integer type stays that integer type: the operation
    // runs in T, and the result truncates back toward zero, exactly like the builtin
    // uint128 += double would
    {
        uint128 x {UINT64_C(0), UINT64_C(5)};
        x += half; // 5.5 truncates to 5
        BOOST_TEST_EQ(x, uint128{5});
    }
    {
        int128 x {-5};
        x -= half; // -5.5 truncates to -5
        BOOST_TEST_EQ(x, int128{-5});
    }

    test_deleted_ops<T>();
    (void)name;
}

void test_float128_exact()
{
    #ifdef __STDCPP_FLOAT128_T__

    using T = std::float128_t;

    BOOST_TEST_EQ(uint128{T{7}}, uint128{7});
    BOOST_TEST_EQ(int128{T{-7}}, int128{-7});
    BOOST_TEST_EQ(uint128{T{-5}}, uint128{0});

    // 113 digits covers a 128-bit magnitude exactly only when no more than 113 of its
    // bits are significant; the low 16 bits are zeroed here so this one is, and the
    // conversion is then exact (never rounded), so a plain equality check is valid
    const uint128 big {UINT64_C(0x0102030405060708), UINT64_C(0x1112131415160000)};
    const T as_f128 {static_cast<T>(big)};
    const auto back {static_cast<uint128>(as_f128)};
    BOOST_TEST_EQ(back, big);

    constexpr auto mn {(std::numeric_limits<int128>::min)()};
    const T mn_f128 {static_cast<T>(mn)};
    BOOST_TEST_EQ(static_cast<int128>(mn_f128), mn);

    const auto r {uint128{5} + T{0.5}};
    static_assert(std::is_same<decltype(r), const T>::value, "result type must be float128_t");
    BOOST_TEST(r == T{5.5});

    test_deleted_ops<T>();

    #endif // __STDCPP_FLOAT128_T__
}

// Existing code must not gain new ambiguity now that the extended types satisfy
// detail::is_floating_point_v: plain double/float usage, integer conversions, and
// std::sqrt/std::abs-style calls on the builtin types are unaffected.
void test_no_new_ambiguity()
{
    const uint128 x {UINT64_C(0), UINT64_C(4)};
    BOOST_TEST(x == 4.0);
    BOOST_TEST(!(x == 1.5));
    const auto as_int {static_cast<int>(x)};
    BOOST_TEST_EQ(as_int, 4);
    const auto as_double {static_cast<double>(x)};
    BOOST_TEST_EQ(as_double, 4.0);
    using std::sqrt;
    BOOST_TEST_EQ(sqrt(as_double), 2.0);
}

} // namespace

int main()
{
    test_no_new_ambiguity();

    #ifdef __STDCPP_FLOAT16_T__
    test_construction<std::float16_t>("float16_t");
    test_conversion_narrow<std::float16_t>("float16_t");
    test_mixed_ops<std::float16_t>("float16_t");
    #endif

    #ifdef __STDCPP_BFLOAT16_T__
    test_construction<std::bfloat16_t>("bfloat16_t");
    test_conversion_narrow<std::bfloat16_t>("bfloat16_t");
    test_mixed_ops<std::bfloat16_t>("bfloat16_t");
    #endif

    #ifdef __STDCPP_FLOAT32_T__
    test_construction<std::float32_t>("float32_t");
    test_conversion_narrow<std::float32_t>("float32_t");
    test_mixed_ops<std::float32_t>("float32_t");
    #endif

    #ifdef __STDCPP_FLOAT64_T__
    test_construction<std::float64_t>("float64_t");
    test_conversion_narrow<std::float64_t>("float64_t");
    test_mixed_ops<std::float64_t>("float64_t");
    #endif

    test_float128_exact();

    return boost::report_errors();
}

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif
