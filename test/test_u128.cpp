// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int128.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/mp11.hpp>
#include <cstring>
#include <cstdint>
#include <random>
#include <limits>
#include <cmath>
#include <sstream>
#include <iostream>
#include <iomanip>

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wold-style-cast"
#  pragma clang diagnostic ignored "-Wundef"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wsign-conversion"
#  pragma clang diagnostic ignored "-Wfloat-equal"
#  pragma clang diagnostic ignored "-Wsign-compare"
#  pragma clang diagnostic ignored "-Woverflow"

#  if (__clang_major__ >= 10 && !defined(__APPLE__)) || __clang_major__ >= 13
#    pragma clang diagnostic ignored "-Wdeprecated-copy"
#  endif

#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wundef"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wsign-compare"
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#  pragma GCC diagnostic ignored "-Woverflow"

#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4389)
#  pragma warning(disable : 4127)
#  pragma warning(disable : 4305)
#  pragma warning(disable : 4309)
#endif

template <typename IntType>
IntType get_max()
{
    return std::numeric_limits<IntType>::max();
}

template <typename IntType>
IntType get_min()
{
    return std::numeric_limits<IntType>::min();
}

template <typename T>
T get_root_max()
{
    return static_cast<T>(std::sqrt(std::numeric_limits<T>::max()));
}

template <typename T>
T get_root_min()
{
    // numeric_limits<T>::min() is negative for signed T, so sqrt() would be NaN
    // and the cast UB; use the negative of the positive root instead.
    BOOST_INT128_IF_CONSTEXPR (std::is_signed<T>::value)
    {
        return static_cast<T>(-get_root_max<T>());
    }
    else
    {
        return static_cast<T>(0);
    }
}

#include <boost/random/uniform_int_distribution.hpp>

// Used defined seed for repeatability
static std::mt19937_64 rng(42);

constexpr std::size_t N = 1024;

#if defined(BOOST_INT128_HAS_INT128) || defined(BOOST_INT128_HAS_MSVC_INT128)

using boost::int128::detail::builtin_u128;
using boost::int128::detail::builtin_i128;

// We reduce the max end of the 128 bit types as they can cause a stack overflow in boost.random

template <>
builtin_u128 get_max<builtin_u128>()
{
    return static_cast<builtin_u128>(UINT64_MAX) << 64 | UINT64_MAX / 32;
}

template <>
builtin_u128 get_min<builtin_u128>()
{
    return 0;
}

template <>
builtin_i128 get_max<builtin_i128>()
{
    return static_cast<builtin_i128>((static_cast<builtin_u128>(1) << 127) - 1) / 32;
}

template <>
builtin_i128 get_min<builtin_i128>()
{
    return -get_max<builtin_i128>() - 1;
}

template <>
builtin_u128 get_root_max<builtin_u128>()
{
    return (UINT64_MAX >> 2);
}

template <>
builtin_u128 get_root_min<builtin_u128>()
{
    return 0;
}

template <>
builtin_i128 get_root_max<builtin_i128>()
{
    return INT64_MAX;
}

template <>
builtin_i128 get_root_min<builtin_i128>()
{
    return INT64_MIN;
}

template <typename IntType>
void test_arithmetic_constructor()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        builtin_u128 emulated_bits;
        std::memcpy(&emulated_bits, &emulated_value, sizeof(builtin_u128));

        BOOST_TEST(emulated_bits == builtin_value);
    }
}

template <typename IntType>
void test_assignment_operators()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        builtin_u128 builtin_value {};
        builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {};
        emulated_value = value;

        builtin_u128 emulated_bits;
        std::memcpy(&emulated_bits, &emulated_value, sizeof(builtin_u128));

        BOOST_TEST(emulated_bits == builtin_value);
    }
}

template <typename IntType>
void test_integer_conversion_operators()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        builtin_u128 builtin_value;
        builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {};
        emulated_value = value;

        const auto builtin_value_return = static_cast<IntType>(builtin_value);
        const auto emulated_value_return = static_cast<IntType>(emulated_value);

        BOOST_TEST(builtin_value_return == emulated_value_return);

        // Hits the implicit bool conversion
        if (builtin_value)
        {
            BOOST_TEST(emulated_value);
        }
    }
}

template <typename FloatType>
void test_float_conversion_operators()
{
    // Float128 won't have numerics limit defined all the time,
    // Plus this affords some additional tolerance
    constexpr FloatType error_tol {std::is_same<FloatType, float>::value ?
        static_cast<FloatType>(std::numeric_limits<float>::epsilon()) :
        static_cast<FloatType>(std::numeric_limits<double>::epsilon())};

    for (std::size_t i {}; i < N; ++i)
    {
        BOOST_INT128_IF_CONSTEXPR (!std::is_same<FloatType, float>::value)
        {
            boost::random::uniform_int_distribution<std::uint32_t> dist(std::numeric_limits<std::uint32_t>::min(),
                                                                        std::numeric_limits<std::uint32_t>::max());

            const auto value {dist(rng)}; // LCOV_EXCL_LINE
            builtin_u128 builtin_value;
            builtin_value = static_cast<builtin_u128>(value) << 64 | static_cast<builtin_u128>(value);
            boost::int128::uint128 emulated_value {value, value};

            // Converts the value and then normalizes the range
            const auto builtin_value_return = static_cast<FloatType>(builtin_value) / static_cast<FloatType>(1e27L);
            const auto emulated_value_return = static_cast<FloatType>(emulated_value) / static_cast<FloatType>(1e27L);
            FloatType distance = builtin_value_return - emulated_value_return;

            distance = distance < 0 ? -distance : distance; // LCOV_EXCL_LINE

            BOOST_TEST(distance < error_tol); // LCOV_EXCL_LINE
        }
        else
        {
            boost::random::uniform_int_distribution<std::uint64_t> dist(std::numeric_limits<std::uint64_t>::min(),
                                                                        std::numeric_limits<std::uint64_t>::max());

            const auto value {dist(rng)}; // LCOV_EXCL_LINE
            builtin_u128 builtin_value;
            builtin_value = value;
            boost::int128::uint128 emulated_value {};
            emulated_value = value;

            const auto builtin_value_return = static_cast<FloatType>(builtin_value);
            const auto emulated_value_return = static_cast<FloatType>(emulated_value);
            FloatType distance = builtin_value_return - emulated_value_return;

            distance = distance < 0 ? -distance : distance; // LCOV_EXCL_LINE

            BOOST_TEST(distance < error_tol); // LCOV_EXCL_LINE
        }
    }
}

template <typename IntType>
void test_unary_plus()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        builtin_value = +builtin_value;
        boost::int128::uint128 emulated_value {value};
        emulated_value = +emulated_value;

        builtin_u128 emulated_bits;
        std::memcpy(&emulated_bits, &emulated_value, sizeof(builtin_u128));

        BOOST_TEST(emulated_bits == builtin_value);
    }
}

template <typename IntType>
void test_unary_minus()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        builtin_value = -builtin_value;
        boost::int128::uint128 emulated_value {value};
        emulated_value = -emulated_value;

        builtin_u128 emulated_bits;
        std::memcpy(&emulated_bits, &emulated_value, sizeof(builtin_u128));

        BOOST_TEST(emulated_bits == builtin_value);
    }
}

template <typename IntType>
void test_operator_equality()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    // Always equal
    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        BOOST_TEST(((value == emulated_value) == (emulated_value == value)) ==
                   ((value == builtin_value) == (builtin_value == value)));

        BOOST_TEST(emulated_value == emulated_value);
    }

    // Potentially equal
    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const IntType value2 {dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        BOOST_TEST(((value2 == emulated_value) == (emulated_value == value2)) ==
                   ((value2 == builtin_value) == (builtin_value == value2)));
    }

    const boost::int128::uint128 bool_val {dist(rng)};
    BOOST_TEST((true == bool_val) == (bool_val == true));
    const boost::int128::uint128 bool_val2 {1, static_cast<std::uint64_t>(dist(rng))};
    BOOST_TEST((true == bool_val2) == (bool_val2 == true));

    BOOST_INT128_IF_CONSTEXPR (sizeof(IntType) < sizeof(boost::int128::uint128))
    {
        for (std::size_t i {}; i < N; ++i)
        {
            IntType value {dist(rng)};

            BOOST_INT128_IF_CONSTEXPR (std::is_signed<IntType>::value)
            {
                if (value < 0)
                {
                    value = -value;
                }
            }

            boost::int128::uint128 emulated_value {1, static_cast<std::uint64_t>(value)};
            BOOST_TEST(!(emulated_value == value));
            BOOST_TEST(emulated_value == emulated_value);
        }
    }
}

template <typename IntType>
void test_operator_inequality()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    // Always equal
    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        BOOST_TEST(((value != emulated_value) == (emulated_value != value)) ==
                   ((value != builtin_value) == (builtin_value != value)));
    }

    // Potentially equal
    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const IntType value2 {dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        BOOST_TEST(((value2 != emulated_value) == (emulated_value != value2)) ==
                   ((value2 != builtin_value) == (builtin_value != value2)));
    }

    const boost::int128::uint128 bool_val {dist(rng)};
    BOOST_TEST((true != bool_val) == (bool_val != true));
    const boost::int128::uint128 bool_val2 {1, static_cast<std::uint64_t>(dist(rng))};
    BOOST_TEST((true == bool_val2) == (bool_val2 == true));

    BOOST_INT128_IF_CONSTEXPR (sizeof(IntType) < sizeof(boost::int128::uint128))
    {
        for (std::size_t i {}; i < N; ++i)
        {
            IntType value {dist(rng)};

            BOOST_INT128_IF_CONSTEXPR (std::is_signed<IntType>::value)
            {
                if (value < 0)
                {
                    value = -value;
                }
            }

            const boost::int128::uint128 emulated_value {1, static_cast<std::uint64_t>(value)};
            const boost::int128::uint128 emulated_value2 {2, static_cast<std::uint64_t>(value)};
            BOOST_TEST(emulated_value != value);
            BOOST_TEST(emulated_value != emulated_value2);
        }
    }
}

template <typename IntType>
void test_operator_less()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const IntType value2 {dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        // Some platforms get this wrong where for example -99 < 340282366920938463463374607431768211408 evaluates to false
        // These values happen to be bitwise equal
        #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable:4127)
        #endif

        BOOST_INT128_IF_CONSTEXPR (std::is_signed<IntType>::value)
        {
            if (value == value2 && value < 0)
            {
                continue;
            }
        }

        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif

        BOOST_TEST(((value2 < emulated_value) == (value2 < builtin_value)) ==
                   ((emulated_value < value2) == (builtin_value < value2)));
    }

    // On 32-bits we check the words all the way down
    // low low
    boost::int128::uint128 lhs {0, 1};
    boost::int128::uint128 rhs {0, 2};
    BOOST_TEST(lhs < rhs);
    BOOST_TEST(!(rhs < lhs));
    // low high
    lhs = {0, UINT32_MAX};
    rhs = {0, static_cast<std::uint64_t>(UINT32_MAX) + 1ULL};
    BOOST_TEST(lhs < rhs);
    BOOST_TEST(!(rhs < lhs));
    // high low
    lhs = {1, UINT64_MAX};
    rhs = {2, UINT64_MAX};
    BOOST_TEST(lhs < rhs);
    BOOST_TEST(!(rhs < lhs));
    // high high
    lhs = {UINT32_MAX, UINT64_MAX};
    rhs = {static_cast<std::uint64_t>(UINT32_MAX) + 1ULL, UINT64_MAX};
    BOOST_TEST(lhs < rhs);
    BOOST_TEST(!(rhs < lhs));
}

template <typename IntType>
void test_operator_le()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const IntType value2 {dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        // Some platforms get this wrong where for example -99 < 340282366920938463463374607431768211408 evaluates to false
        // These values happen to be bitwise equal
        #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable:4127)
        #endif

        BOOST_INT128_IF_CONSTEXPR (std::is_signed<IntType>::value)
        {
            if (value == value2 && value < 0)
            {
                continue;
            }
        }

        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif

        BOOST_TEST(((value2 <= emulated_value) == (value2 <= builtin_value)) ==
                   ((emulated_value <= value2) == (builtin_value <= value2)));
    }

    // On 32-bits we check the words all the way down
    // low low
    boost::int128::uint128 lhs {0, 1};
    boost::int128::uint128 rhs {0, 2};
    BOOST_TEST(lhs <= rhs);
    BOOST_TEST(!(rhs <= lhs));
    BOOST_TEST(lhs <= lhs);
    // low high
    lhs = {0, UINT32_MAX};
    rhs = {0, static_cast<std::uint64_t>(UINT32_MAX) + 1ULL};
    BOOST_TEST(lhs <= rhs);
    BOOST_TEST(!(rhs <= lhs));
    BOOST_TEST(lhs <= lhs);
    // high low
    lhs = {1, UINT64_MAX};
    rhs = {2, UINT64_MAX};
    BOOST_TEST(lhs <= rhs);
    BOOST_TEST(!(rhs <= lhs));
    BOOST_TEST(lhs <= lhs);
    // high high
    lhs = {UINT32_MAX, UINT64_MAX};
    rhs = {static_cast<std::uint64_t>(UINT32_MAX) + 1ULL, UINT64_MAX};
    BOOST_TEST(lhs <= rhs);
    BOOST_TEST(!(rhs <= lhs));
    BOOST_TEST(lhs <= lhs);
}

template <typename IntType>
void test_operator_greater()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const IntType value2 {dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        // Some platforms get this wrong where for example -99 < 340282366920938463463374607431768211408 evaluates to false
        // These values happen to be bitwise equal
        #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable:4127)
        #endif

        BOOST_INT128_IF_CONSTEXPR (std::is_signed<IntType>::value)
        {
            if (value == value2 && value < 0)
            {
                continue;
            }
        }

        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif

        BOOST_TEST(((value2 > emulated_value) == (value2 > builtin_value)) ==
                   ((emulated_value > value2) == (builtin_value > value2)));
    }

    // On 32-bits we check the words all the way down
    // low low
    boost::int128::uint128 lhs {0, 1};
    boost::int128::uint128 rhs {0, 2};
    BOOST_TEST(rhs > lhs);
    BOOST_TEST(!(lhs > rhs));
    // low high
    lhs = {0, UINT32_MAX};
    rhs = {0, static_cast<std::uint64_t>(UINT32_MAX) + 1ULL};
    BOOST_TEST(rhs > lhs);
    BOOST_TEST(!(lhs > rhs));
    // high low
    lhs = {1, UINT64_MAX};
    rhs = {2, UINT64_MAX};
    BOOST_TEST(rhs > lhs);
    BOOST_TEST(!(lhs > rhs));
    // high high
    lhs = {UINT32_MAX, UINT64_MAX};
    rhs = {static_cast<std::uint64_t>(UINT32_MAX) + 1ULL, UINT64_MAX};
    BOOST_TEST(rhs > lhs);
    BOOST_TEST(!(lhs > rhs));
}

template <typename IntType>
void test_operator_ge()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const IntType value2 {dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        // Some platforms get this wrong where for example -99 < 340282366920938463463374607431768211408 evaluates to false
        // These values happen to be bitwise equal
        #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable:4127)
        #endif

        BOOST_INT128_IF_CONSTEXPR (std::is_signed<IntType>::value)
        {
            if (value == value2 && value < 0)
            {
                continue;
            }
        }

        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif

        BOOST_TEST(((value2 >= emulated_value) == (value2 >= builtin_value)) ==
                   ((emulated_value >= value2) == (builtin_value >= value2)));
    }

    // On 32-bits we check the words all the way down
    // low low
    boost::int128::uint128 lhs {0, 1};
    boost::int128::uint128 rhs {0, 2};
    BOOST_TEST(rhs >= lhs);
    BOOST_TEST(!(lhs >= rhs));
    BOOST_TEST(lhs >= lhs);
    // low high
    lhs = {0, UINT32_MAX};
    rhs = {0, static_cast<std::uint64_t>(UINT32_MAX) + 1ULL};
    BOOST_TEST(rhs >= lhs);
    BOOST_TEST(!(lhs >= rhs));
    BOOST_TEST(lhs >= lhs);
    // high low
    lhs = {1, UINT64_MAX};
    rhs = {2, UINT64_MAX};
    BOOST_TEST(rhs >= lhs);
    BOOST_TEST(!(lhs >= rhs));
    BOOST_TEST(lhs >= lhs);
    // high high
    lhs = {UINT32_MAX, UINT64_MAX};
    rhs = {static_cast<std::uint64_t>(UINT32_MAX) + 1ULL, UINT64_MAX};
    BOOST_TEST(rhs >= lhs);
    BOOST_TEST(!(lhs >= rhs));
    BOOST_TEST(lhs >= lhs);
}

template <typename IntType>
void test_operator_not()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        BOOST_TEST(~emulated_value == ~builtin_value);
    }
}

template <typename IntType>
void test_operator_or()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const IntType value2 {dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        auto check_1_value {emulated_value};
        check_1_value |= value2;

        BOOST_INT128_IF_CONSTEXPR (!std::is_same<IntType, builtin_u128>::value)
        {
            BOOST_TEST(check_1_value == (builtin_value | value2));
            BOOST_TEST((value2 | emulated_value) == (value2 | builtin_value));
        }
        else
        {
            builtin_u128 emulated_checkpoint;
            std::memcpy(&emulated_checkpoint, &check_1_value, sizeof(emulated_checkpoint));

            builtin_u128 builtin_checkpoint;
            builtin_value |= value2;
            std::memcpy(&builtin_checkpoint, &builtin_value, sizeof(builtin_checkpoint));

            BOOST_TEST(emulated_checkpoint == builtin_checkpoint);
        }
    }
}

template <typename IntType>
void test_operator_and()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const IntType value2 {dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        auto check_1_value {emulated_value};
        check_1_value &= value2;

        BOOST_INT128_IF_CONSTEXPR (!std::is_same<IntType, builtin_u128>::value)
        {
            BOOST_TEST(check_1_value == (builtin_value & value2));
            BOOST_TEST((value2 & emulated_value) == (value2 & builtin_value));
        }
        else
        {
            builtin_u128 emulated_checkpoint;
            std::memcpy(&emulated_checkpoint, &check_1_value, sizeof(emulated_checkpoint));

            builtin_u128 builtin_checkpoint;
            builtin_value &= value2;
            std::memcpy(&builtin_checkpoint, &builtin_value, sizeof(builtin_checkpoint));

            BOOST_TEST(emulated_checkpoint == builtin_checkpoint);
        }
    }
}

template <typename IntType>
void test_operator_xor()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const IntType value2 {dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        auto check_1_value {emulated_value};
        check_1_value ^= value2;

        BOOST_INT128_IF_CONSTEXPR (!std::is_same<IntType, builtin_u128>::value)
        {
            BOOST_TEST(check_1_value == (builtin_value ^ value2));
            BOOST_TEST((value2 ^ emulated_value) == (value2 ^ builtin_value));
        }
        else
        {
            builtin_u128 emulated_checkpoint;
            std::memcpy(&emulated_checkpoint, &check_1_value, sizeof(emulated_checkpoint));

            builtin_u128 builtin_checkpoint;
            builtin_value ^= value2;
            std::memcpy(&builtin_checkpoint, &builtin_value, sizeof(builtin_checkpoint));

            BOOST_TEST(emulated_checkpoint == builtin_checkpoint);
        }
    }
}

template <typename IntType>
void test_operator_left_shift()
{
    boost::random::uniform_int_distribution<IntType> dist(static_cast<IntType>(0),
                                                          get_max<IntType>());

    boost::random::uniform_int_distribution<unsigned> shift_dist(0, sizeof(IntType) * CHAR_BIT - 1);

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const unsigned shift_value {shift_dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        // Test 1: Test the <<= operator
        auto builtin_copy = builtin_value;
        auto emulated_copy = emulated_value;

        builtin_copy <<= shift_value;
        emulated_copy <<= shift_value;

        BOOST_TEST(emulated_copy == builtin_copy);
        BOOST_TEST(emulated_copy == (emulated_value << static_cast<boost::int128::uint128>(shift_value)));

        // Test 2: Test the binary << operator
        auto shifted_builtin = builtin_value << shift_value;
        auto shifted_emulated = emulated_value << shift_value;

        BOOST_TEST(shifted_emulated == shifted_builtin);

        // Test 3: Test with IntType as left operand and int128 variants as right operand
        auto int_shift_builtin = value << static_cast<unsigned>(builtin_value);
        auto int_shift_emulated = value << static_cast<unsigned>(emulated_value);

        static_assert(std::is_same<decltype(int_shift_builtin),
                                   decltype(int_shift_emulated)>::value, "Mismatched types");

        BOOST_TEST(int_shift_emulated == int_shift_builtin);

        // Test 4: Test the consteval path
        auto shift_value_consteval = boost::int128::detail::default_ls_impl(emulated_value, shift_value);
        BOOST_TEST(shifted_emulated == shift_value_consteval);
    }

    // Edge cases. A shift by a negative amount or by an amount >= 128 is
    // undefined behavior, exactly as for the built-in shift operators (see the
    // documentation), so only the well-defined in-range counts are exercised.
    // The scalar, uint128-count, and consteval paths must all agree.
    const boost::int128::uint128 val {UINT64_MAX};
    BOOST_TEST((val << 0) == val);
    BOOST_TEST((val << static_cast<boost::int128::uint128>(0)) == val);
    for (unsigned s {}; s < 128U; ++s)
    {
        BOOST_TEST(boost::int128::detail::default_ls_impl(val, s) == (val << s));
        BOOST_TEST((val << static_cast<boost::int128::uint128>(s)) == (val << s));
    }

    const auto builtin_value {static_cast<builtin_u128>(dist(rng))};
    const boost::int128::uint128 small_shift {1u};
    BOOST_TEST((builtin_value << small_shift) == (builtin_value << 1u));
}

template <typename IntType>
void test_operator_right_shift()
{
    boost::random::uniform_int_distribution<IntType> dist(static_cast<IntType>(0),
                                                          get_max<IntType>());

    boost::random::uniform_int_distribution<unsigned> shift_dist(0, sizeof(IntType) * CHAR_BIT - 1);

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const unsigned shift_value {shift_dist(rng)};
        auto builtin_value = static_cast<builtin_u128>(value);
        const boost::int128::uint128 emulated_value {value};

        // Test 1: Test the >>= operator
        auto builtin_copy = builtin_value;
        auto emulated_copy = emulated_value;

        builtin_copy >>= shift_value;
        emulated_copy >>= shift_value;

        BOOST_TEST(emulated_copy == builtin_copy);
        BOOST_TEST(emulated_copy == (emulated_value >> static_cast<boost::int128::uint128>(shift_value)));

        // Test 2: Test the binary >> operator
        auto shifted_builtin = builtin_value >> shift_value;
        auto shifted_emulated = emulated_value >> shift_value;

        BOOST_TEST(shifted_emulated == shifted_builtin);

        // Test 3: Test with IntType as left operand and int128 variants as right operand
        auto int_shift_builtin = value >> static_cast<unsigned>(builtin_value);
        auto int_shift_emulated = value >> static_cast<unsigned>(emulated_value);

        static_assert(std::is_same<decltype(int_shift_builtin),
                                   decltype(int_shift_emulated)>::value, "Mismatched types");

        BOOST_TEST(int_shift_emulated == int_shift_builtin);

        // Test 4: Test the consteval path
        auto shift_value_consteval = boost::int128::detail::default_rs_impl(emulated_value, shift_value);
        BOOST_TEST(shifted_emulated == shift_value_consteval);
    }

    // Edge cases. A shift by a negative amount or by an amount >= 128 is
    // undefined behavior, exactly as for the built-in shift operators (see the
    // documentation), so only the well-defined in-range counts are exercised.
    // The scalar, uint128-count, and consteval paths must all agree.
    const boost::int128::uint128 val {UINT64_MAX};
    BOOST_TEST((val >> 0) == val);
    BOOST_TEST((val >> static_cast<boost::int128::uint128>(0)) == val);
    for (unsigned s {}; s < 128U; ++s)
    {
        BOOST_TEST(boost::int128::detail::default_rs_impl(val, s) == (val >> s));
        BOOST_TEST((val >> static_cast<boost::int128::uint128>(s)) == (val >> s));
    }

    const auto builtin_value {static_cast<builtin_u128>(dist(rng))};
    const boost::int128::uint128 small_shift {1u};
    BOOST_TEST((builtin_value >> small_shift) == (builtin_value >> 1u));
}

void test_increment_operator()
{
    boost::int128::uint128 emulated_value {UINT64_MAX - N/2};
    builtin_u128 builtin_value {UINT64_MAX - N/2};

    for (std::size_t i {}; i < N; ++i)
    {
        ++emulated_value;
        ++builtin_value;

        BOOST_TEST(emulated_value == builtin_value);
    }
}

void test_decrement_operator()
{
    boost::int128::uint128 emulated_value {UINT64_MAX + N/2};
    builtin_u128 builtin_value {UINT64_MAX + N/2};

    for (std::size_t i {}; i < N; ++i)
    {
        --emulated_value;
        --builtin_value;

        BOOST_TEST(emulated_value == builtin_value);
    }
}

template <typename IntType>
void test_operator_add()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        IntType value {dist(rng)};
        IntType value2 {dist(rng)};

        // Avoid UB from rollover
        BOOST_INT128_IF_CONSTEXPR (std::is_same<IntType, builtin_u128>::value || std::is_same<IntType, builtin_i128>::value)
        {
            value /= 100;
            value2 /= 100;
        }

        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        auto check_1_value {emulated_value};
        check_1_value += value2;
        BOOST_TEST(check_1_value == (builtin_value + value2));
        BOOST_TEST((value2 + emulated_value) == (value2 + builtin_value));
    }

    // Edge case where we go from low word to high word
    builtin_u128 builtin_value = UINT64_MAX - 2U;
    boost::int128::uint128 emulated_value = UINT64_MAX - 2U;

    for (std::size_t i {}; i < N; ++i)
    {
        BOOST_TEST(++builtin_value == ++emulated_value);
    }
}

template <typename IntType>
void test_operator_sub()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        IntType value {dist(rng)};
        IntType value2 {dist(rng)};

        // Avoid UB from rollover
        BOOST_INT128_IF_CONSTEXPR (std::is_same<IntType, builtin_u128>::value || std::is_same<IntType, builtin_i128>::value)
        {
            value /= 100;
            value2 /= 100;
        }

        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        auto check_1_value {emulated_value};
        check_1_value -= value2;
        BOOST_TEST(check_1_value == (builtin_value - value2));
        BOOST_TEST((value2 - emulated_value) == (value2 - builtin_value));
    }

    // Edge case where we go from low word to high word
    builtin_u128 builtin_value = UINT64_MAX;
    builtin_value += 2U;
    boost::int128::uint128 emulated_value = UINT64_MAX;
    emulated_value += 2U;

    for (std::size_t i {}; i < N; ++i)
    {
        BOOST_TEST(--builtin_value == --emulated_value);
    }
}

template <typename IntType>
void test_operator_mul()
{
    boost::random::uniform_int_distribution<IntType> dist(get_root_min<IntType>(), get_root_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        IntType value {dist(rng)};
        IntType value2 {dist(rng)};

        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        auto check_1_value {emulated_value};
        check_1_value *= value2;
        BOOST_TEST(check_1_value == (builtin_value * value2));
        BOOST_TEST((value2 * emulated_value) == (value2 * builtin_value));
    }
}

template <typename IntType>
void test_operator_div()
{
    // I am not entirely sure what the MSVC int128 is doing here for negative values.
    // In hardware you cast to common type, and the alternative would be -u128/abs(int)
    // Neither of these is what is happening...

    #ifdef _WIN32

    boost::random::uniform_int_distribution<IntType> dist(0, get_max<IntType>());

    #else

    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
        get_max<IntType>());

    #endif

    for (std::size_t i {}; i < N; ++i)
    {
        IntType value {dist(rng)};
        IntType value2 {dist(rng)};

        // Avoid UB of div by 0
        // LCOV_EXCL_START
        while (value == 0)
        {
            value = dist(rng);
        }
        while (value2 == 0)
        {
            value2 = dist(rng);
        }
        // LCOV_EXCL_STOP

        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        auto check_1_value {emulated_value};
        check_1_value /= value2;

        static_assert(sizeof(decltype(emulated_value / value2)) ==
                      sizeof(decltype(builtin_value / value2)), "Mismatch Return Types");

        static_assert(sizeof(decltype(value2 / emulated_value)) ==
                      sizeof(decltype(value2 / builtin_value)), "Mismatch Return Types");

        BOOST_TEST(check_1_value == (builtin_value / value2));
        BOOST_TEST((value2 / emulated_value) == (value2 / builtin_value));
    }

    // Test 2 word by 1 word and 1 word by 2 word
    BOOST_INT128_IF_CONSTEXPR (sizeof(IntType) < sizeof(boost::int128::uint128))
    {
        for (std::size_t i {}; i < N; ++i)
        {
            IntType value {dist(rng)};
            IntType value2 {dist(rng)};

            // Avoid UB of div by 0
            // LCOV_EXCL_START
            while (value == 0)
            {
                value = dist(rng);
            }
            while (value2 == 0)
            {
                value2 = dist(rng);
            }
            // LCOV_EXCL_STOP

            const auto builtin_value = (static_cast<builtin_u128>(static_cast<std::uint64_t>(value)) << 64) | static_cast<std::uint64_t>(value);
            const boost::int128::uint128 emulated_value {static_cast<std::uint64_t>(value), static_cast<std::uint64_t>(value)};
            const boost::int128::uint128 small_emulated_value {UINT64_C(0), static_cast<std::uint64_t>(value)};

            assert(builtin_value == emulated_value);

            auto check_1_value {emulated_value};
            check_1_value /= value2;

            static_assert(sizeof(decltype(emulated_value / value2)) ==
                          sizeof(decltype(builtin_value / value2)), "Mismatch Return Types");

            static_assert(sizeof(decltype(value2 / emulated_value)) ==
                          sizeof(decltype(value2 / builtin_value)), "Mismatch Return Types");

            BOOST_TEST(check_1_value == (builtin_value / value2));
            BOOST_TEST((value2 / emulated_value) == (value2 / builtin_value));

            // Forces decision process
            const boost::int128::uint128 check_2_value {value2};
            BOOST_TEST(check_1_value == (emulated_value / check_2_value));

            // Division by zero is undefined behavior (matching the builtin __int128 types), so it is not tested.

            // Always 0
            BOOST_TEST(small_emulated_value / emulated_value == 0);
            BOOST_TEST(small_emulated_value / small_emulated_value == 1);
        }
    }
}

template <typename IntType>
void test_operator_mod()
{
    // I am not entirely sure what the MSVC int128 is doing here for negative values.
    // In hardware you cast to common type, and the alternative would be -u128/abs(int)
    // Neither of these is what is happening...

    #ifdef _WIN32

    boost::random::uniform_int_distribution<IntType> dist(0, get_max<IntType>());

    #else

    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    #endif

    for (std::size_t i {}; i < N; ++i)
    {
        IntType value {dist(rng)};
        IntType value2 {dist(rng)};

        // Avoid UB of div by 0
        // LCOV_EXCL_START
        while (value == 0)
        {
            value = dist(rng);
        }
        while (value2 == 0)
        {
            value2 = dist(rng);
        }
        // LCOV_EXCL_STOP

        auto builtin_value = static_cast<builtin_u128>(value);
        boost::int128::uint128 emulated_value {value};

        auto check_1_value {emulated_value};
        check_1_value %= value2;

        static_assert(sizeof(decltype(emulated_value % value2)) ==
                      sizeof(decltype(builtin_value % value2)), "Mismatch Return Types");

        static_assert(sizeof(decltype(value2 % emulated_value)) ==
                      sizeof(decltype(value2 % builtin_value)), "Mismatch Return Types");

        BOOST_TEST(check_1_value == (builtin_value % value2));
        BOOST_TEST((value2 % emulated_value) == (value2 % builtin_value));
    }

    // Test 2 word by 1 word and 1 word by 2 word
    BOOST_INT128_IF_CONSTEXPR (sizeof(IntType) < sizeof(boost::int128::uint128))
    {
        for (std::size_t i {}; i < N; ++i)
        {
            IntType value {dist(rng)};
            IntType value2 {dist(rng)};

            // Avoid UB of div by 0
            // LCOV_EXCL_START
            while (value == 0)
            {
                value = dist(rng);
            }
            while (value2 == 0)
            {
                value2 = dist(rng);
            }
            // LCOV_EXCL_STOP

            const auto builtin_value = (static_cast<builtin_u128>(static_cast<std::uint64_t>(value)) << 64) | static_cast<std::uint64_t>(value);
            const boost::int128::uint128 emulated_value {static_cast<std::uint64_t>(value), static_cast<std::uint64_t>(value)};
            const boost::int128::uint128 small_emulated_value {UINT64_C(0), static_cast<std::uint64_t>(value)};

            auto check_1_value {emulated_value};
            check_1_value %= value2;

            static_assert(sizeof(decltype(emulated_value % value2)) ==
                          sizeof(decltype(builtin_value % value2)), "Mismatch Return Types");

            static_assert(sizeof(decltype(value2 % emulated_value)) ==
                          sizeof(decltype(value2 % builtin_value)), "Mismatch Return Types");

            BOOST_TEST(check_1_value == (builtin_value % value2));
            BOOST_TEST((value2 % emulated_value) == (value2 % builtin_value));

            // Forces decision process
            const boost::int128::uint128 check_2_value {value2};
            BOOST_TEST(check_1_value == (emulated_value % check_2_value));

            // Remainder by zero is undefined behavior (matching the builtin __int128 types), so it is not tested.

            BOOST_TEST(small_emulated_value % emulated_value == small_emulated_value);
            BOOST_TEST(small_emulated_value % small_emulated_value == 0);
        }
    }
}

// Exercises unary minus where the low word is zero, which the random parity tests
// almost never reach, plus the values at both ends of the range
void test_spot_unary_minus()
{
    using boost::int128::uint128;

    constexpr auto max_val {std::numeric_limits<uint128>::max()};

    static_assert(-uint128{0} == 0, "-0");
    static_assert(-uint128{1} == max_val, "-1");
    static_assert(-max_val == 1, "-max");
    static_assert(-uint128{UINT64_MAX, UINT64_C(0)} == uint128{UINT64_C(1), UINT64_C(0)}, "-(2^128 - 2^64)");
    static_assert(-uint128{UINT64_C(1), UINT64_C(0)} == uint128{UINT64_MAX, UINT64_C(0)}, "-(2^64)");

    struct case_t { std::uint64_t high; std::uint64_t low; std::uint64_t neg_high; std::uint64_t neg_low; };
    constexpr case_t cases[] {
        {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0)},
        {UINT64_C(0), UINT64_C(1), UINT64_MAX, UINT64_MAX},
        {UINT64_C(0), UINT64_MAX, UINT64_MAX, UINT64_C(1)},
        {UINT64_C(1), UINT64_C(0), UINT64_MAX, UINT64_C(0)},
        {UINT64_MAX, UINT64_C(0), UINT64_C(1), UINT64_C(0)},
        {UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0x8000000000000000), UINT64_C(0)},
        {UINT64_MAX, UINT64_MAX, UINT64_C(0), UINT64_C(1)},
        {UINT64_C(0x123456789ABCDEF0), UINT64_C(0), UINT64_C(0xEDCBA98765432110), UINT64_C(0)},
        {UINT64_C(0x123456789ABCDEF0), UINT64_C(0x0FEDCBA987654321), UINT64_C(0xEDCBA9876543210F), UINT64_C(0xF0123456789ABCDF)},
    };

    for (const auto& c : cases)
    {
        const uint128 value {c.high, c.low};
        BOOST_TEST(-value == uint128(c.neg_high, c.neg_low));
        BOOST_TEST(-(-value) == value);
        BOOST_TEST(-value == uint128{0} - value);
    }
}

template <typename IntType>
void test_spot_div(IntType value, IntType value2)
{
    auto builtin_value = static_cast<builtin_u128>(value);
    boost::int128::uint128 emulated_value {value};

    auto check_1_value {emulated_value};
    check_1_value /= value2;

    static_assert(sizeof(decltype(emulated_value / value2)) ==
                  sizeof(decltype(builtin_value / value2)), "Mismatch Return Types");

    static_assert(sizeof(decltype(value2 / emulated_value)) ==
                  sizeof(decltype(value2 / builtin_value)), "Mismatch Return Types");

    // The tested values are pulled out unlike the regular test
    // so that it's easier to read the values with GDB

    auto check_1_value_builtin {(builtin_value / value2)};

    BOOST_TEST(check_1_value == check_1_value_builtin);

    auto check_2_value {value2 / emulated_value};
    auto check_2_value_builtin {value2 / builtin_value};

    BOOST_TEST(check_2_value == check_2_value_builtin);
}

template <typename IntType>
void test_spot_div(IntType lhs, IntType rhs, IntType known_res)
{
    const auto check_1 {lhs / rhs};
    BOOST_TEST_EQ(check_1, known_res);

    lhs /= rhs;
    BOOST_TEST_EQ(lhs, known_res);
}

struct test_caller
{
    template<typename T>
    void operator()(T) const
    {
        test_arithmetic_constructor<T>();
        test_assignment_operators<T>();
        test_integer_conversion_operators<T>();
        
        test_unary_plus<T>();
        test_unary_minus<T>();
        
        test_operator_equality<T>();
        test_operator_inequality<T>();
        test_operator_less<T>();
        test_operator_le<T>();
        test_operator_greater<T>();
        test_operator_ge<T>();
        
        test_operator_not<T>();
        test_operator_or<T>();
        test_operator_and<T>();
        test_operator_xor<T>();

        #if !defined(UBSAN) && !defined(ASAN) && !defined(__SANITIZE_ADDRESS__)
        test_operator_left_shift<T>();
        test_operator_right_shift<T>();
        #endif
        
        test_increment_operator();
        test_decrement_operator();

        test_operator_add<T>();
        test_operator_sub<T>();
        test_operator_mul<T>();
        test_operator_div<T>();
        test_operator_mod<T>();
    }
};

int main()
{
    using test_types = boost::mp11::mp_list<
        char,
        unsigned char,
        char16_t,
        char32_t,
        wchar_t,
        short,
        unsigned short,
        int,
        unsigned int,
        long,
        unsigned long,
        long long,
        unsigned long long,
        #ifndef BOOST_INT128_HAS_MSVC_INT128
        builtin_i128,
        #endif
        builtin_u128
    >;

    boost::mp11::mp_for_each<test_types>(test_caller());

    test_spot_unary_minus();

    // MSVC does not provide a float conversion operator
    #ifndef BOOST_INT128_HAS_MSVC_INT128

    test_float_conversion_operators<float>();
    test_float_conversion_operators<double>();
    test_float_conversion_operators<long double>();

    #endif 

    test_spot_div<char>(1, -32);
    test_spot_div<char>(15, -91);
    test_spot_div<char>(39, -100);

    test_spot_div<long>(-888610053741375541L, 3110266252672496347L);

    test_spot_div<long long>(-3237361348456748317LL, 8011834041509972187LL);

    test_spot_div<boost::int128::uint128>(boost::int128::uint128{50012077812411ULL, 6429278683030093824ULL}, boost::int128::uint128{542101086ULL, 4477988020393345024ULL}, boost::int128::uint128{92256});

    return boost::report_errors();
}

#else

// The 32-bit coverage flags every call to distribution and BOOST_TEST as partials
// which is technically correct but also pure noise
// LCOV_EXCL_START

template <typename IntType>
void test_operator_equality()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    // Always equal
    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        boost::int128::uint128 emulated_value {value};

        BOOST_TEST(((value == emulated_value) == (emulated_value == value)));
    }

    // Potentially equal
    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const IntType value2 {dist(rng)};
        boost::int128::uint128 emulated_value {value};

        BOOST_TEST(((value2 == emulated_value) == (emulated_value == value2)));
    }

    // Never equal
    BOOST_INT128_IF_CONSTEXPR (sizeof(IntType) < sizeof(boost::int128::uint128))
    {
        for (std::size_t i {}; i < N; ++i)
        {
            const IntType value {dist(rng)};
            boost::int128::uint128 emulated_value {1, static_cast<std::uint64_t>(value)};
            BOOST_TEST((value == emulated_value) == (emulated_value == value));
        }
    }

    const boost::int128::uint128 bool_val {dist(rng)};
    BOOST_TEST((true == bool_val) == (bool_val == true));
    const boost::int128::uint128 bool_val2 {static_cast<std::uint64_t>(dist(rng)), 0};
    BOOST_TEST((true == bool_val2) == (bool_val2 == true));
    BOOST_TEST(!(bool_val == bool_val2));
    BOOST_TEST(bool_val == bool_val);
    BOOST_TEST(!(bool_val == (bool_val+1)));
    BOOST_TEST(!((bool_val+1) == bool_val));
    BOOST_TEST(bool_val2 == bool_val2);
}

template <typename IntType>
void test_operator_inequality()
{
    boost::random::uniform_int_distribution<IntType> dist(get_min<IntType>(),
                                                          get_max<IntType>());

    // Always equal
    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        boost::int128::uint128 emulated_value {value};

        BOOST_TEST(((value != emulated_value) == (emulated_value != value)));
    }

    // Potentially equal
    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const IntType value2 {dist(rng)};
        boost::int128::uint128 emulated_value {value};

        BOOST_TEST(((value2 != emulated_value) == (emulated_value != value2)));
    }

    // Never equal
    BOOST_INT128_IF_CONSTEXPR (sizeof(IntType) < sizeof(boost::int128::uint128))
    {
        for (std::size_t i {}; i < N; ++i)
        {
            const IntType value {dist(rng)};
            boost::int128::uint128 emulated_value {1, static_cast<std::uint64_t>(value)};
            BOOST_TEST((value != emulated_value) == (emulated_value != value));
        }
    }

    const boost::int128::uint128 bool_val {dist(rng)};
    BOOST_TEST((true != bool_val) == (bool_val != true));
    const boost::int128::uint128 bool_val2 {static_cast<std::uint64_t>(dist(rng)), 0};
    BOOST_TEST((true != bool_val2) == (bool_val2 != true));
    BOOST_TEST(bool_val != bool_val2);
    BOOST_TEST(!(bool_val != bool_val));
    BOOST_TEST((bool_val != (bool_val+1)));
    BOOST_TEST(((bool_val+1) != bool_val));
    BOOST_TEST(!(bool_val2 != bool_val2));
}

template <typename IntType>
void test_operator_less()
{
    boost::random::uniform_int_distribution<IntType> dist(0, get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        IntType value2 {dist(rng)};

        while (value == value2)
        {
            value2 = dist(rng);
        }

        const boost::int128::uint128 emulated_value {value};

        BOOST_TEST(((value2 < emulated_value) != (emulated_value < value2)));
    }

    // For sure equal
    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const boost::int128::uint128 emulated_value {value};

        BOOST_TEST(((value < emulated_value) == (emulated_value < value)));
    }
}

template <typename IntType>
void test_operator_greater()
{
    boost::random::uniform_int_distribution<IntType> dist(0, get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        IntType value2 {dist(rng)};

        while (value == value2)
        {
            value2 = dist(rng);
        }

        const boost::int128::uint128 emulated_value {value};

        BOOST_TEST(((value2 > emulated_value) != (emulated_value > value2)));
    }

    // For sure equal
    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const boost::int128::uint128 emulated_value {value};

        BOOST_TEST(((value > emulated_value) == (emulated_value > value)));
    }
}

template <typename IntType>
void test_operator_le()
{
    boost::random::uniform_int_distribution<IntType> dist(0, get_max<IntType>());

    // Possibly equal
    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const IntType value2 {dist(rng)};
        const boost::int128::uint128 emulated_value {value};

        if (value == value2)
        {
            BOOST_TEST(((value2 <= emulated_value) == (emulated_value <= value2)));
        }
        else
        {
            BOOST_TEST(((value2 <= emulated_value) != (emulated_value <= value2)));
        }
    }

    // For sure equal
    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const boost::int128::uint128 emulated_value {value};

        BOOST_TEST(((value <= emulated_value) == (emulated_value <= value)));
    }
}

template <typename IntType>
void test_operator_ge()
{
    boost::random::uniform_int_distribution<IntType> dist(0, get_max<IntType>());

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const IntType value2 {dist(rng)};
        const boost::int128::uint128 emulated_value {value};

        if (value == value2)
        {
            BOOST_TEST(((value2 >= emulated_value) == (emulated_value >= value2)));
        }
        else
        {
            BOOST_TEST(((value2 >= emulated_value) != (emulated_value >= value2)));
        }
    }

    // For sure equal
    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)};
        const boost::int128::uint128 emulated_value {value};

        const auto lhs {(value >= emulated_value)};
        const auto rhs {(emulated_value >= value)};

        if (!BOOST_TEST(lhs == rhs))
        {
            std::cerr << "Value: " << static_cast<std::int64_t>(value) << '\n'
            << "EmulV: " << emulated_value << std::endl;
        }
    }

    // Spot test
    const IntType value {0};
    const boost::int128::uint128 emulated_value {0};

    const auto lhs {(value >= emulated_value)};
    const auto rhs {(emulated_value >= value)};

    BOOST_TEST(lhs == rhs);
}

template <typename IntType>
void test_operator_add()
{
    boost::random::uniform_int_distribution<IntType> dist(0, get_max<IntType>() / 2);

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)}; // LCOV_EXCL_LINE
        const IntType value2 {dist(rng)}; // LCOV_EXCL_LINE
        const IntType res = value + value2;

        boost::int128::uint128 test_value {value};
        const boost::int128::uint128 test_value2 {value2};
        BOOST_TEST(test_value + test_value2 == res); // LCOV_EXCL_LINE

        test_value += value2;
        BOOST_TEST(test_value == res); // LCOV_EXCL_LINE
    }

    // Bigger value spots
    boost::int128::uint128 lhs{UINT64_MAX};
    boost::int128::uint128 rhs{1, 0};

    BOOST_TEST_EQ(lhs + 1, rhs);
    BOOST_TEST_EQ(++lhs, rhs);
}

template <typename IntType>
void test_operator_sub()
{
    boost::random::uniform_int_distribution<IntType> dist(0, get_max<IntType>() / 2);

    for (std::size_t i {}; i < N; ++i)
    {
        IntType value {dist(rng)}; // LCOV_EXCL_LINE
        IntType value2 {dist(rng)}; // LCOV_EXCL_LINE

        if (value < value2)
        {
            std::swap(value, value2);
        }

        const IntType res = value - value2;

        boost::int128::uint128 test_value {value};
        const boost::int128::uint128 test_value2 {value2};
        BOOST_TEST(test_value - test_value2 == res); // LCOV_EXCL_LINE

        test_value -= value2;
        BOOST_TEST(test_value == res); // LCOV_EXCL_LINE
    }

    // Bigger value spots
    boost::int128::uint128 rhs{UINT64_MAX};
    boost::int128::uint128 lhs{1, 0};

    BOOST_TEST_EQ(lhs - 1, rhs);
    BOOST_TEST_EQ(--lhs, rhs);
}

template <typename IntType>
void test_operator_mul()
{
    boost::random::uniform_int_distribution<IntType> dist(0, get_root_max<IntType>()); // LCOV_EXCL_LINE

    for (std::size_t i {}; i < N; ++i)
    {
        const IntType value {dist(rng)}; // LCOV_EXCL_LINE
        const IntType value2 {dist(rng)}; // LCOV_EXCL_LINE
        const IntType res = value * value2;

        boost::int128::uint128 test_value {value};
        const boost::int128::uint128 test_value2 {value2};

        BOOST_TEST(test_value * test_value2 == res); // LCOV_EXCL_LINE

        test_value *= value2;
        BOOST_TEST(test_value == res); // LCOV_EXCL_LINE
    }

    boost::int128::uint128 shift_val {1};
    boost::int128::uint128 mul_val {1};

    for (std::size_t i {1}; i < 128; ++i)
    {
        BOOST_TEST(shift_val == mul_val); // LCOV_EXCL_LINE

        shift_val <<= 1;
        mul_val *= 2;
    }
}

template <typename IntType>
void test_operator_div()
{
    boost::random::uniform_int_distribution<IntType> dist(0, get_root_max<IntType>()); // LCOV_EXCL_LINE

    for (std::size_t i {}; i < N; ++i)
    {
        IntType value {dist(rng)}; // LCOV_EXCL_LINE
        IntType value2 {dist(rng)}; // LCOV_EXCL_LINE

        while (value == 0)
        {
            value = dist(rng);
        }
        while (value2 == 0)
        {
            value2 = dist(rng);
        }

        if (value < value2)
        {
            std::swap(value, value2);
        }

        const IntType res = value / value2;

        boost::int128::uint128 test_value {value};
        const boost::int128::uint128 test_value2 {value2};

        BOOST_TEST(test_value / test_value2 == res); // LCOV_EXCL_LINE

        test_value /= value2;
        BOOST_TEST(test_value == res); // LCOV_EXCL_LINE
    }

    boost::int128::uint128 shift_val {UINT64_MAX, UINT64_MAX};
    boost::int128::uint128 mul_val {UINT64_MAX, UINT64_MAX};

    for (std::size_t i {1}; i < 127; ++i)
    {
        const auto current_shift_val {shift_val >> i};
        const auto current_mul_val {mul_val / (boost::int128::uint128{2} << (i-1))};

        BOOST_TEST(current_shift_val == current_mul_val); // LCOV_EXCL_LINE
    }

    // Check large value
    constexpr auto lhs {(std::numeric_limits<boost::int128::uint128>::max)()};
    constexpr boost::int128::uint128 rhs {0x1, UINT64_MAX};
    constexpr boost::int128::uint128 res {UINT64_C(0x8000000000000000)};

    BOOST_TEST(lhs / rhs == res);
}

// LCOV_EXCL_STOP

struct test_caller
{
    template<typename T>
    void operator()(T) const
    {
        test_operator_equality<T>();
        test_operator_inequality<T>();
        test_operator_less<T>();
        test_operator_greater<T>();
        test_operator_le<T>();
        test_operator_ge<T>();

        test_operator_add<T>();
        test_operator_sub<T>();
        test_operator_mul<T>();
        test_operator_div<T>();
    }
};

int main()
{
    using test_types = boost::mp11::mp_list<
        char,
        unsigned char,
        char16_t,
        char32_t,
        wchar_t,
        short,
        unsigned short,
        int,
        unsigned int,
        long,
        unsigned long,
        long long,
        unsigned long long
    >;

    boost::mp11::mp_for_each<test_types>(test_caller()); // LCOV_EXCL_LINE

    return boost::report_errors();
}

#endif // BOOST_INT128_HAS_INT128
