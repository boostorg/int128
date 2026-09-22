// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int128.hpp>
#include <boost/core/lightweight_test.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <limits>
#include <random>

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wold-style-cast"
#endif

#include <boost/random/uniform_int_distribution.hpp>

#ifdef __clang__
#  pragma clang diagnostic pop
#endif

static constexpr std::size_t N {1024};
static std::mt19937_64 rng{42};

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4127) // conditional expression is constant pre-C++17
#endif

template <typename T>
void test_istream()
{
    T val;
    std::stringstream in;
    in.str("42");
    in >> val;
    BOOST_TEST_EQ(val, static_cast<T>(42));

    // Hex value
    T hex_val;
    std::stringstream in_hex;
    in_hex.flags(std::ios_base::hex);
    in_hex.str("0xffff");
    in_hex >> hex_val;
    BOOST_TEST_EQ(hex_val, static_cast<T>(0xFFFF));

    T hex_val_2;
    std::stringstream in_hex_2;
    in_hex_2.flags(std::ios_base::hex);
    in_hex_2.str("FFFFFFFFFFFF");
    in_hex_2 >> hex_val_2;
    BOOST_TEST_EQ(hex_val_2, static_cast<T>(0xFFFFFFFFFFFF));

    // Octal Value
    T oct_val;
    std::stringstream in_oct;
    in_oct.flags(std::ios_base::oct);
    in_oct.str("01111");
    in_oct >> oct_val;
    BOOST_TEST_EQ(oct_val, static_cast<T>(01111));

    T oct_val_2;
    std::stringstream in_oct_2;
    in_oct_2.flags(std::ios_base::oct);
    in_oct_2.str("42");
    in_oct_2 >> oct_val_2;
    BOOST_TEST_EQ(oct_val_2, static_cast<T>(042));
}

template <typename T>
void test_ostream()
{
    T val {42};
    std::stringstream out;
    out << val;
    BOOST_TEST_CSTR_EQ(out.str().c_str(), "42");

    T zero_val {0};
    std::stringstream zero_out;
    zero_out << zero_val;
    BOOST_TEST_CSTR_EQ(zero_out.str().c_str(), "0");

    T hex_val {0xFF};
    std::stringstream hex_out;
    hex_out.flags(std::ios_base::hex);
    hex_out << hex_val;
    BOOST_TEST_CSTR_EQ(hex_out.str().c_str(), "ff");

    // 32-bit windows does not set the flags correctly in CI
    #ifndef _M_IX86

    std::stringstream hex_out_base;
    hex_out_base.flags(std::ios_base::hex | std::ios_base::showbase);
    hex_out_base << hex_val;
    BOOST_TEST_CSTR_EQ(hex_out_base.str().c_str(), "0xff");

    std::stringstream hex_out_upper;
    hex_out_upper.flags(std::ios_base::hex | std::ios_base::uppercase);
    hex_out_upper << hex_val;
    BOOST_TEST_CSTR_EQ(hex_out_upper.str().c_str(), "FF");

    std::stringstream hex_out_upper_base;
    hex_out_upper_base.flags(std::ios_base::hex | std::ios_base::uppercase | std::ios_base::showbase);
    hex_out_upper_base << hex_val;
    BOOST_TEST_CSTR_EQ(hex_out_upper_base.str().c_str(), "0XFF");

    #endif

    T octal_val {04};
    std::stringstream octal_out;
    octal_out.flags(std::ios_base::oct);
    octal_out << octal_val;
    BOOST_TEST_CSTR_EQ(octal_out.str().c_str(), "4");

    std::stringstream octal_out_upper;
    octal_out_upper.flags(std::ios_base::hex | std::ios_base::showbase);
    octal_out_upper << octal_val;
    BOOST_TEST_CSTR_EQ(octal_out.str().c_str(), "4");

    BOOST_INT128_IF_CONSTEXPR (std::is_same<T, boost::int128::uint128>::value)
    {
        boost::int128::uint128 max_val {std::numeric_limits<boost::int128::uint128>::max()};
        std::stringstream max_out;
        max_out << max_val;
        BOOST_TEST_CSTR_EQ(max_out.str().c_str(), "340282366920938463463374607431768211455");
    }
    else
    {
        boost::int128::int128 max_val {std::numeric_limits<boost::int128::int128>::max()};
        std::stringstream max_out;
        max_out << max_val;
        BOOST_TEST_CSTR_EQ(max_out.str().c_str(), "170141183460469231731687303715884105727");

        boost::int128::int128 min_val {std::numeric_limits<boost::int128::int128>::min()};
        std::stringstream min_out;
        min_out << min_val;
        BOOST_TEST_CSTR_EQ(min_out.str().c_str(), "-170141183460469231731687303715884105728");

        boost::int128::int128 regular_val {-4500};
        std::stringstream reg_out;
        reg_out << regular_val;
        BOOST_TEST_CSTR_EQ(reg_out.str().c_str(), "-4500");
    }
}

void test_error_values()
{
    using boost::int128::uint128;
    using boost::int128::int128;

    uint128 val;
    std::stringstream in;
    in.str("-42");
    in >> val;
    BOOST_TEST_EQ(val, 0U);

    uint128 val2;
    std::stringstream in2;
    in2.str("+42");
    in2 >> val2;
    BOOST_TEST_EQ(val2, 0U);

    uint128 val3;
    std::stringstream in3;
    in3.str("");
    in3 >> val3;
    BOOST_TEST_EQ(val3, 0U);

    int128 val4;
    std::stringstream in4;
    in4.str("-");
    in4 >> val4;
    BOOST_TEST_EQ(val4, 0);

    uint128 val5;
    std::stringstream in5;
    in5.str("F");
    in5 >> val5;
    BOOST_TEST_EQ(val5, 0U);

    uint128 val6;
    std::stringstream in6;
    in6.str("3F");
    in6 >> val6;
    BOOST_TEST_EQ(val6, 3U);

    uint128 val7;
    std::stringstream in7;
    in7.str("100000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    in7 >> val7;
    BOOST_TEST_EQ(val7, 0U);

    boost::int128::detail::from_chars(nullptr, nullptr, val7);
    BOOST_TEST_EQ(val7, 0U);

    uint128 overflow_val;
    std::stringstream huge_str;
    // 100 digits
    huge_str.str("123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345");
    huge_str << overflow_val;
    BOOST_TEST_EQ(overflow_val, 0U);
}

template <typename T>
void test_round_trip();

template <>
void test_round_trip<boost::int128::uint128>()
{
    boost::random::uniform_int_distribution<boost::int128::uint128> dist(0, (std::numeric_limits<boost::int128::uint128>::max)());

    // Decimal
    for (std::size_t i = 0; i < N; ++i)
    {
        const auto val {dist(rng)};
        std::stringstream out;
        out << val;
        boost::int128::uint128 return_val;
        out >> return_val;

        BOOST_TEST_EQ(val, return_val);
    }

    // Hex lower
    for (std::size_t i = 0; i < N; ++i)
    {
        const auto val {dist(rng)};
        std::stringstream out;
        out.flags(std::ios_base::hex);
        out << val;
        boost::int128::uint128 return_val;
        out >> return_val;

        BOOST_TEST_EQ(val, return_val);
    }

    // Hex upper
    for (std::size_t i = 0; i < N; ++i)
    {
        const auto val {dist(rng)};
        std::stringstream out;
        out.flags(std::ios_base::hex | std::ios_base::uppercase);
        out << val;
        boost::int128::uint128 return_val;
        out >> return_val;

        BOOST_TEST_EQ(val, return_val);
    }

    // Octal
    for (std::size_t i = 0; i < N; ++i)
    {
        const auto val {dist(rng)};
        std::stringstream out;
        out.flags(std::ios_base::oct);
        out << val;
        boost::int128::uint128 return_val;
        out >> return_val;

        BOOST_TEST_EQ(val, return_val);
    }
}

template <>
void test_round_trip<boost::int128::int128>()
{
    std::uniform_int_distribution<std::uint64_t> low(0, (std::numeric_limits<std::uint64_t>::max)());
    std::uniform_int_distribution<std::int64_t> high((std::numeric_limits<std::int64_t>::min)(), (std::numeric_limits<std::int64_t>::max)());

    for (std::size_t i = 0; i < N; ++i)
    {
        const boost::int128::int128 val {high(rng), low(rng)};
        std::stringstream out;
        out << val;
        boost::int128::int128 return_val;
        out >> return_val;

        BOOST_TEST_EQ(val, return_val);
    }

    boost::int128::int128 val {-1};
    std::stringstream out;
    out << val;
    BOOST_TEST_CSTR_EQ(out.str().c_str(), "-1");
    boost::int128::int128 return_val;
    out >> return_val;
    BOOST_TEST_EQ(val, return_val);
}

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

// Extraction must report failure through the stream state, zero the value, and put
// every character back, so that the idiomatic while (is >> v) loop terminates
template <typename T>
void test_istream_failures()
{
    // A first character that is not a digit
    {
        std::stringstream in;
        in.str("abc def");
        T v {static_cast<T>(7)};
        int iterations {};
        while (iterations < 1000 && (in >> v))
        {
            ++iterations;
        }
        BOOST_TEST_EQ(iterations, 0);
        BOOST_TEST(in.fail());
        BOOST_TEST_EQ(v, static_cast<T>(0));

        in.clear();
        std::string rest;
        in >> rest;
        BOOST_TEST_CSTR_EQ(rest.c_str(), "abc");
    }

    // Overflow: 2^128 fits neither type
    {
        std::stringstream in;
        in.str("340282366920938463463374607431768211456");
        T v {static_cast<T>(7)};
        in >> v;
        BOOST_TEST(in.fail());
        BOOST_TEST_EQ(v, static_cast<T>(0));
    }

    // Digits that are not valid in the active base
    {
        std::stringstream in;
        in.flags(std::ios_base::hex);
        in.str("zz");
        T v {static_cast<T>(7)};
        in >> v;
        BOOST_TEST(in.fail());
        BOOST_TEST_EQ(v, static_cast<T>(0));

        in.clear();
        std::string rest;
        in >> rest;
        BOOST_TEST_CSTR_EQ(rest.c_str(), "zz");
    }

    // Empty input
    {
        std::stringstream in;
        in.str("");
        T v {static_cast<T>(7)};
        in >> v;
        BOOST_TEST(in.fail());
    }

    // A sign parses on the signed type and is a failure on the unsigned one
    {
        std::stringstream in;
        in.str("-5");
        T v {static_cast<T>(7)};
        in >> v;

        BOOST_INT128_IF_CONSTEXPR (std::numeric_limits<T>::is_signed)
        {
            BOOST_TEST(!in.fail());
            BOOST_TEST_EQ(v, static_cast<T>(-5));
        }
        else
        {
            BOOST_TEST(in.fail());
            BOOST_TEST_EQ(v, static_cast<T>(0));
        }
    }
}

// Only a real 0x or 0X prefix is skipped in hex, a bare leading zero is a digit in every
// base, and whatever was consumed (prefix included) does not come back
template <typename T>
void test_istream_prefixes()
{
    // Hex "0" is the value zero with nothing left over
    {
        std::stringstream in;
        in.flags(std::ios_base::hex);
        in.str("0");
        T v {static_cast<T>(7)};
        in >> v;
        BOOST_TEST(!in.fail());
        BOOST_TEST_EQ(v, static_cast<T>(0));

        in.clear();
        std::string rest;
        in >> rest;
        BOOST_TEST(rest.empty());
    }

    // Hex "0f" is 15: the leading zero is a digit, not a prefix
    {
        std::stringstream in;
        in.flags(std::ios_base::hex);
        in.str("0f");
        T v {};
        in >> v;
        BOOST_TEST(!in.fail());
        BOOST_TEST_EQ(v, static_cast<T>(15));
    }

    // Hex "0x1fz" is 31 and only the z remains in the stream
    {
        std::stringstream in;
        in.flags(std::ios_base::hex);
        in.str("0x1fz");
        T v {};
        in >> v;
        BOOST_TEST(!in.fail());
        BOOST_TEST_EQ(v, static_cast<T>(31));

        std::string rest;
        in >> rest;
        BOOST_TEST_CSTR_EQ(rest.c_str(), "z");
    }

    // Uppercase prefix
    {
        std::stringstream in;
        in.flags(std::ios_base::hex);
        in.str("0X1F");
        T v {};
        in >> v;
        BOOST_TEST(!in.fail());
        BOOST_TEST_EQ(v, static_cast<T>(31));
    }

    // Octal "017" is 15 and fully consumed
    {
        std::stringstream in;
        in.flags(std::ios_base::oct);
        in.str("017");
        T v {};
        in >> v;
        BOOST_TEST(!in.fail());
        BOOST_TEST_EQ(v, static_cast<T>(15));

        in.clear();
        std::string rest;
        in >> rest;
        BOOST_TEST(rest.empty());
    }

    // Octal "08" is 0 with the 8 left in the stream, as num_get does for the builtins
    {
        std::stringstream in;
        in.flags(std::ios_base::oct);
        in.str("08");
        T v {static_cast<T>(7)};
        in >> v;
        BOOST_TEST(!in.fail());
        BOOST_TEST_EQ(v, static_cast<T>(0));

        std::string rest;
        in >> rest;
        BOOST_TEST_CSTR_EQ(rest.c_str(), "8");
    }
}

// showbase never adds a prefix to a zero, the same as the builtin types
template <typename T>
void test_ostream_showbase_zero()
{
    const T zero {0};

    std::stringstream hex_out;
    hex_out.flags(std::ios_base::hex | std::ios_base::showbase);
    hex_out << zero;
    BOOST_TEST_CSTR_EQ(hex_out.str().c_str(), "0");

    std::stringstream oct_out;
    oct_out.flags(std::ios_base::oct | std::ios_base::showbase);
    oct_out << zero;
    BOOST_TEST_CSTR_EQ(oct_out.str().c_str(), "0");

    std::stringstream dec_out;
    dec_out.flags(std::ios_base::dec | std::ios_base::showbase);
    dec_out << zero;
    BOOST_TEST_CSTR_EQ(dec_out.str().c_str(), "0");

    // A nonzero value keeps its prefix
    std::stringstream hex_one;
    hex_one.flags(std::ios_base::hex | std::ios_base::showbase);
    hex_one << T {1};
    BOOST_TEST_CSTR_EQ(hex_one.str().c_str(), "0x1");
}

int main()
{
    test_istream<boost::int128::uint128>();
    test_istream<boost::int128::int128>();

    test_istream_failures<boost::int128::uint128>();
    test_istream_failures<boost::int128::int128>();

    test_istream_prefixes<boost::int128::uint128>();
    test_istream_prefixes<boost::int128::int128>();

    test_ostream<boost::int128::uint128>();
    test_ostream<boost::int128::int128>();

    test_ostream_showbase_zero<boost::int128::uint128>();
    test_ostream_showbase_zero<boost::int128::int128>();

    // 32-bit windows does not set the iomanip flags correctly in CI
    #ifndef _M_IX86

    test_round_trip<boost::int128::uint128>();
    test_round_trip<boost::int128::int128>();

    #endif

    test_error_values();

    return boost::report_errors();
}
