// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// Exercises boost::int128::detail::from_chars across every base it supports
// (2..36) for both int128 and uint128. Locks in the fix to the per-iteration
// overflow threshold (the spurious overflow_value <<= 1 was masking MAX+1
// overflow in mini_from_chars).

#ifndef BOOST_INT128_BUILD_MODULE

#include <boost/int128/int128.hpp>
#include <boost/int128/detail/mini_from_chars.hpp>

#else

import boost.int128;

#endif

#include <boost/core/lightweight_test.hpp>

#include <algorithm>
#include <cerrno>
#include <limits>
#include <string>

namespace {

using boost::int128::int128;
using boost::int128::uint128;

// Format a uint128 into a base-N string (lowercase). Self-contained so the
// test does not pull in boost::charconv just to generate inputs.
std::string format_unsigned(uint128 value, int base)
{
    if (value == uint128{0U})
    {
        return "0";
    }

    const auto ubase {static_cast<unsigned>(base)};
    std::string out;
    while (value != uint128{0U})
    {
        const auto digit {static_cast<unsigned>(value % ubase)};
        const char c {digit < 10U ? static_cast<char>('0' + digit)
                                  : static_cast<char>('a' + digit - 10U)};
        out.push_back(c);
        value /= ubase;
    }
    std::reverse(out.begin(), out.end());
    return out;
}

std::string format_signed(int128 value, int base)
{
    if (value == int128{0})
    {
        return "0";
    }

    if (value == (std::numeric_limits<int128>::min)())
    {
        // |INT128_MIN| does not fit in int128; do the magnitude in uint128.
        const uint128 magnitude {uint128{1} << 127U};
        std::string out {format_unsigned(magnitude, base)};
        out.insert(out.begin(), '-');
        return out;
    }

    if (value < int128{0})
    {
        std::string out {format_unsigned(static_cast<uint128>(-value), base)};
        out.insert(out.begin(), '-');
        return out;
    }

    return format_unsigned(static_cast<uint128>(value), base);
}

inline std::string format_value(int128 value, int base)
{
    return format_signed(value, base);
}

inline std::string format_value(uint128 value, int base)
{
    return format_unsigned(value, base);
}

template <typename T>
void check_roundtrip(T expected, int base)
{
    const std::string s {format_value(expected, base)};

    T parsed {};
    const auto r {boost::int128::detail::from_chars(s.data(), s.data() + s.size(), parsed, base)};

    BOOST_TEST_LT(r, 0);
    BOOST_TEST(parsed == expected);
}

template <typename T>
void check_overflow(const std::string& s, int base)
{
    T parsed {};
    const auto r {boost::int128::detail::from_chars(s.data(), s.data() + s.size(), parsed, base)};

    BOOST_TEST_EQ(r, EDOM);
}

void test_uint128_all_bases()
{
    constexpr auto max_value {(std::numeric_limits<uint128>::max)()};

    for (int base {2}; base <= 36; ++base)
    {
        // Canonical small values.
        check_roundtrip<uint128>(uint128{0U}, base);
        check_roundtrip<uint128>(uint128{1U}, base);
        check_roundtrip<uint128>(static_cast<uint128>(static_cast<unsigned>(base) - 1U), base);
        check_roundtrip<uint128>(static_cast<uint128>(static_cast<unsigned>(base)), base);

        // A handful of mid-range values that span the per-base digit window.
        check_roundtrip<uint128>(uint128{42U}, base);
        check_roundtrip<uint128>(uint128{1234567890U}, base);
        check_roundtrip<uint128>(uint128{0xFFFFFFFFFFFFFFFFULL}, base);
        check_roundtrip<uint128>(uint128{1U} << 100U, base);

        // The boundary itself parses correctly.
        check_roundtrip<uint128>(max_value, base);

        // MAX with any extra digit appended is at least MAX * base, which
        // overflows uint128 for every base in [2, 36].
        const auto max_str {format_unsigned(max_value, base)};
        check_overflow<uint128>(max_str + "0", base);
    }
}

void test_int128_all_bases()
{
    constexpr auto max_value {(std::numeric_limits<int128>::max)()};
    constexpr auto min_value {(std::numeric_limits<int128>::min)()};

    for (int base {2}; base <= 36; ++base)
    {
        check_roundtrip<int128>(int128{0}, base);
        check_roundtrip<int128>(int128{1}, base);
        check_roundtrip<int128>(int128{-1}, base);
        check_roundtrip<int128>(int128{42}, base);
        check_roundtrip<int128>(int128{-42}, base);
        check_roundtrip<int128>(int128{1234567890}, base);
        check_roundtrip<int128>(int128{-1234567890}, base);

        // Both signed boundaries parse correctly.
        check_roundtrip<int128>(max_value, base);
        check_roundtrip<int128>(min_value, base);

        // Append a digit to push past the magnitude bound on each side.
        const auto max_str {format_signed(max_value, base)};
        check_overflow<int128>(max_str + "0", base);

        const auto min_str {format_signed(min_value, base)};
        check_overflow<int128>(min_str + "0", base);
    }
}

void test_decimal_boundaries()
{
    // Tight base-10 boundary cases: the spurious <<= 1 in the threshold made
    // these silently produce wrong values instead of returning EDOM.

    // UINT128_MAX exactly.
    {
        const std::string s {"340282366920938463463374607431768211455"};
        uint128 v {};
        const auto r {boost::int128::detail::from_chars(s.data(), s.data() + s.size(), v)};
        BOOST_TEST_LT(r, 0);
        BOOST_TEST(v == (std::numeric_limits<uint128>::max)());
    }

    // UINT128_MAX + 1.
    check_overflow<uint128>("340282366920938463463374607431768211456", 10);

    // INT128_MAX exactly.
    {
        const std::string s {"170141183460469231731687303715884105727"};
        int128 v {};
        const auto r {boost::int128::detail::from_chars(s.data(), s.data() + s.size(), v)};
        BOOST_TEST_LT(r, 0);
        BOOST_TEST(v == (std::numeric_limits<int128>::max)());
    }

    // INT128_MAX + 1.
    check_overflow<int128>("170141183460469231731687303715884105728", 10);

    // INT128_MIN exactly.
    {
        const std::string s {"-170141183460469231731687303715884105728"};
        int128 v {};
        const auto r {boost::int128::detail::from_chars(s.data(), s.data() + s.size(), v)};
        BOOST_TEST_LT(r, 0);
        BOOST_TEST(v == (std::numeric_limits<int128>::min)());
    }

    // INT128_MIN - 1.
    check_overflow<int128>("-170141183460469231731687303715884105729", 10);
}

void test_invalid_inputs()
{
    // Empty range is EINVAL.
    {
        const char* s {""};
        uint128 v {};
        const auto r {boost::int128::detail::from_chars(s, s, v)};
        BOOST_TEST_EQ(r, EINVAL);
    }

    // Lone sign is EINVAL.
    {
        const std::string s {"-"};
        int128 v {};
        const auto r {boost::int128::detail::from_chars(s.data(), s.data() + s.size(), v)};
        BOOST_TEST_EQ(r, EINVAL);
    }

    // Leading sign on the unsigned overload is EINVAL.
    {
        const std::string s {"-1"};
        uint128 v {};
        const auto r {boost::int128::detail::from_chars(s.data(), s.data() + s.size(), v)};
        BOOST_TEST_EQ(r, EINVAL);
    }
}

// A parse that consumes nothing returns 0 and leaves the output untouched, an overflow
// returns EDOM and also leaves it untouched, and a base outside 2..36 is EINVAL
void test_failure_contract()
{
    {
        const std::string s {"abc"};
        uint128 v {UINT64_C(0xDEADBEEF)};
        const auto r {boost::int128::detail::from_chars(s.data(), s.data() + s.size(), v)};
        BOOST_TEST_EQ(r, 0);
        BOOST_TEST(v == uint128{UINT64_C(0xDEADBEEF)});
    }

    {
        const std::string s {"zz"};
        int128 v {42};
        const auto r {boost::int128::detail::from_chars(s.data(), s.data() + s.size(), v, 16)};
        BOOST_TEST_EQ(r, 0);
        BOOST_TEST(v == int128{42});
    }

    // A sign followed by a non digit consumes nothing either
    {
        const std::string s {"-x"};
        int128 v {42};
        const auto r {boost::int128::detail::from_chars(s.data(), s.data() + s.size(), v)};
        BOOST_TEST_EQ(r, 0);
        BOOST_TEST(v == int128{42});
    }

    {
        const std::string s {"340282366920938463463374607431768211456"};
        uint128 v {7};
        const auto r {boost::int128::detail::from_chars(s.data(), s.data() + s.size(), v)};
        BOOST_TEST_EQ(r, EDOM);
        BOOST_TEST(v == uint128{7});
    }

    for (const int base : {0, 1, 37, -1})
    {
        const std::string s {"11"};
        uint128 v {7};
        const auto r {boost::int128::detail::from_chars(s.data(), s.data() + s.size(), v, base)};
        BOOST_TEST_EQ(r, EINVAL);
        BOOST_TEST(v == uint128{7});
    }
}

} // anonymous namespace

int main()
{
    test_uint128_all_bases();
    test_int128_all_bases();
    test_decimal_boundaries();
    test_invalid_inputs();
    test_failure_contract();

    return boost::report_errors();
}
