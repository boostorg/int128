// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int128.hpp>
#include <boost/int128/format.hpp>
#include <boost/int128/literals.hpp>
#include <boost/core/lightweight_test.hpp>

#ifdef BOOST_INT128_HAS_FORMAT

template <typename T>
void test_empty()
{
    BOOST_TEST_CSTR_EQ(std::format("{}", T{2}).c_str(), "2");
    BOOST_TEST_CSTR_EQ(std::format("{}", T{22}).c_str(), "22");
    BOOST_TEST_CSTR_EQ(std::format("{}", T{222}).c_str(), "222");
    BOOST_TEST_CSTR_EQ(std::format("{}", T{2222}).c_str(), "2222");
    BOOST_TEST_CSTR_EQ(std::format("{}", T{22222}).c_str(), "22222");
    BOOST_TEST_CSTR_EQ(std::format("{}", T{222222}).c_str(), "222222");

    if constexpr (std::is_same_v<T, boost::int128::int128>)
    {
        BOOST_TEST_CSTR_EQ(std::format("{}", T{-2}).c_str(), "-2");
        BOOST_TEST_CSTR_EQ(std::format("{}", T{-22}).c_str(), "-22");
        BOOST_TEST_CSTR_EQ(std::format("{}", T{-222}).c_str(), "-222");
        BOOST_TEST_CSTR_EQ(std::format("{}", T{-2222}).c_str(), "-2222");
        BOOST_TEST_CSTR_EQ(std::format("{}", T{-22222}).c_str(), "-22222");
        BOOST_TEST_CSTR_EQ(std::format("{}", T{-222222}).c_str(), "-222222");
    }
}

template <typename T>
void test_binary()
{
    BOOST_TEST_CSTR_EQ(std::format("{:b}", T{2}).c_str(), "10");
    BOOST_TEST_CSTR_EQ(std::format("{:#b}", T{2}).c_str(), "0b10");
    BOOST_TEST_CSTR_EQ(std::format("{:#B}", T{2}).c_str(), "0B10");

    BOOST_TEST_CSTR_EQ(std::format("{:b}", T{5}).c_str(), "101");
    BOOST_TEST_CSTR_EQ(std::format("{:#b}", T{5}).c_str(), "0b101");
    BOOST_TEST_CSTR_EQ(std::format("{:#B}", T{5}).c_str(), "0B101");

    BOOST_TEST_CSTR_EQ(std::format("{:6b}", T{5}).c_str(), "   101");
    BOOST_TEST_CSTR_EQ(std::format("{:#6b}", T{5}).c_str(), " 0b101");
    BOOST_TEST_CSTR_EQ(std::format("{:#06B}", T{5}).c_str(), "0B0101");
}

template <typename T>
void test_octal()
{
    BOOST_TEST_CSTR_EQ(std::format("{:o}", T{42}).c_str(), "52");
    BOOST_TEST_CSTR_EQ(std::format("{:#o}", T{42}).c_str(), "052");

    BOOST_TEST_CSTR_EQ(std::format("{:4o}", T{42}).c_str(), "  52");
    BOOST_TEST_CSTR_EQ(std::format("{:#4o}", T{42}).c_str(), " 052");
}

template <typename T>
void test_decimal()
{
    BOOST_TEST_CSTR_EQ(std::format("{:d}", T{42}).c_str(), "42");
    BOOST_TEST_CSTR_EQ(std::format("{:#d}", T{42}).c_str(), "42");

    BOOST_TEST_CSTR_EQ(std::format("{:+d}", T{42}).c_str(), "+42");
    BOOST_TEST_CSTR_EQ(std::format("{:+#d}", T{42}).c_str(), "+42");

    BOOST_TEST_CSTR_EQ(std::format("{: d}", T{42}).c_str(), " 42");
    BOOST_TEST_CSTR_EQ(std::format("{: #d}", T{42}).c_str(), " 42");

    BOOST_TEST_CSTR_EQ(std::format("{:+3d}", T{42}).c_str(), "+42");
    BOOST_TEST_CSTR_EQ(std::format("{:+#3d}", T{42}).c_str(), "+42");

    BOOST_TEST_CSTR_EQ(std::format("{:-3d}", T{42}).c_str(), " 42");
    BOOST_TEST_CSTR_EQ(std::format("{:-#3d}", T{42}).c_str(), " 42");

    if constexpr (std::is_same_v<T, boost::int128::int128>)
    {
        BOOST_TEST_CSTR_EQ(std::format("{: 3d}", T{42}).c_str(), " 42");
        BOOST_TEST_CSTR_EQ(std::format("{: #3d}", T{42}).c_str(), " 42");

        BOOST_TEST_CSTR_EQ(std::format("{:-3d}", T{-42}).c_str(), "-42");
        BOOST_TEST_CSTR_EQ(std::format("{:-#3d}", T{-42}).c_str(), "-42");

        BOOST_TEST_CSTR_EQ(std::format("{: 3d}", T{-42}).c_str(), "-42");
        BOOST_TEST_CSTR_EQ(std::format("{: #3d}", T{-42}).c_str(), "-42");
    }

    BOOST_TEST_CSTR_EQ(std::format("{:+d}", T{42}).c_str(), "+42");
}

template <typename T>
void test_hex()
{
    BOOST_TEST_CSTR_EQ(std::format("{:x}", T{42}).c_str(), "2a");
    BOOST_TEST_CSTR_EQ(std::format("{:#x}", T{42}).c_str(), "0x2a");

    BOOST_TEST_CSTR_EQ(std::format("{:X}", T{42}).c_str(), "2A");
    BOOST_TEST_CSTR_EQ(std::format("{:#X}", T{42}).c_str(), "0X2A");

    BOOST_TEST_CSTR_EQ(std::format("{: X}", T{42}).c_str(), " 2A");
    BOOST_TEST_CSTR_EQ(std::format("{: #X}", T{42}).c_str(), " 0X2A");

    BOOST_TEST_CSTR_EQ(std::format("{:+X}", T{42}).c_str(), "+2A");
    BOOST_TEST_CSTR_EQ(std::format("{:+#X}", T{42}).c_str(), "+0X2A");

    if constexpr (std::is_same_v<T, boost::int128::int128>)
    {
        BOOST_TEST_CSTR_EQ(std::format("{:-X}", T{-42}).c_str(), "-2A");
        BOOST_TEST_CSTR_EQ(std::format("{:-#X}", T{-42}).c_str(), "-0X2A");
    }

    BOOST_TEST_CSTR_EQ(std::format("{:5X}", T{42}).c_str(), "   2A");
    BOOST_TEST_CSTR_EQ(std::format("{:#5X}", T{42}).c_str(), " 0X2A");

    BOOST_TEST_CSTR_EQ(std::format("{: 5X}", T{42}).c_str(), "   2A");
    BOOST_TEST_CSTR_EQ(std::format("{: #5X}", T{42}).c_str(), " 0X2A");
}

template <typename T>
void test_string_insertion()
{
    BOOST_TEST_CSTR_EQ(std::format("Height is: {:d}", T {0}).c_str(), "Height is: 0");
    BOOST_TEST_CSTR_EQ(std::format("Height is: {}", T {2}).c_str(), "Height is: 2");

    BOOST_TEST_CSTR_EQ(std::format("Height is: {:d} meters", T {0}).c_str(), "Height is: 0 meters");
    BOOST_TEST_CSTR_EQ(std::format("Height is: {} meters", T {2}).c_str(), "Height is: 2 meters");
}

template <typename T>
void test_alignment()
{
    // Left alignment with default fill (space)
    BOOST_TEST_CSTR_EQ(std::format("{:<6d}", T{42}).c_str(), "42    ");
    BOOST_TEST_CSTR_EQ(std::format("{:<6}", T{42}).c_str(), "42    ");

    // Right alignment with default fill (space)
    BOOST_TEST_CSTR_EQ(std::format("{:>6d}", T{42}).c_str(), "    42");
    BOOST_TEST_CSTR_EQ(std::format("{:>6}", T{42}).c_str(), "    42");

    // Center alignment with default fill (space)
    BOOST_TEST_CSTR_EQ(std::format("{:^6d}", T{42}).c_str(), "  42  ");
    BOOST_TEST_CSTR_EQ(std::format("{:^7d}", T{42}).c_str(), "  42   ");

    // Left alignment with custom fill
    BOOST_TEST_CSTR_EQ(std::format("{:*<6d}", T{42}).c_str(), "42****");
    BOOST_TEST_CSTR_EQ(std::format("{:-<6d}", T{42}).c_str(), "42----");
    BOOST_TEST_CSTR_EQ(std::format("{:0<6d}", T{42}).c_str(), "420000");

    // Right alignment with custom fill
    BOOST_TEST_CSTR_EQ(std::format("{:*>6d}", T{42}).c_str(), "****42");
    BOOST_TEST_CSTR_EQ(std::format("{:>>6d}", T{42}).c_str(), ">>>>42");
    BOOST_TEST_CSTR_EQ(std::format("{:0>6d}", T{42}).c_str(), "000042");

    // Center alignment with custom fill
    BOOST_TEST_CSTR_EQ(std::format("{:*^6d}", T{42}).c_str(), "**42**");
    BOOST_TEST_CSTR_EQ(std::format("{:*^7d}", T{42}).c_str(), "**42***");

    // Alignment with sign
    BOOST_TEST_CSTR_EQ(std::format("{:>+6d}", T{42}).c_str(), "   +42");
    BOOST_TEST_CSTR_EQ(std::format("{:<+6d}", T{42}).c_str(), "+42   ");
    BOOST_TEST_CSTR_EQ(std::format("{:^+7d}", T{42}).c_str(), "  +42  ");

    // Alignment with hex
    BOOST_TEST_CSTR_EQ(std::format("{:>6x}", T{42}).c_str(), "    2a");
    BOOST_TEST_CSTR_EQ(std::format("{:<6x}", T{42}).c_str(), "2a    ");
    BOOST_TEST_CSTR_EQ(std::format("{:*^6x}", T{42}).c_str(), "**2a**");

    // Alignment with prefix (# comes before width in format spec)
    BOOST_TEST_CSTR_EQ(std::format("{:>#8x}", T{42}).c_str(), "    0x2a");
    BOOST_TEST_CSTR_EQ(std::format("{:<#8x}", T{42}).c_str(), "0x2a    ");
    BOOST_TEST_CSTR_EQ(std::format("{:*^#8x}", T{42}).c_str(), "**0x2a**");
}

template <typename T>
void test_alignment_negative()
{
    // Alignment with negative values
    BOOST_TEST_CSTR_EQ(std::format("{:>6d}", T{-42}).c_str(), "   -42");
    BOOST_TEST_CSTR_EQ(std::format("{:<6d}", T{-42}).c_str(), "-42   ");
    BOOST_TEST_CSTR_EQ(std::format("{:^7d}", T{-42}).c_str(), "  -42  ");
    BOOST_TEST_CSTR_EQ(std::format("{:*>6d}", T{-42}).c_str(), "***-42");
    BOOST_TEST_CSTR_EQ(std::format("{:*<6d}", T{-42}).c_str(), "-42***");
    BOOST_TEST_CSTR_EQ(std::format("{:*^7d}", T{-42}).c_str(), "**-42**");
}

#ifdef BOOST_INT128_HAS_CONSTEXPR_FORMAT

using namespace boost::int128::literals;
static_assert(std::format("num: {}", 1234_i128) == "num: 1234");
static_assert(std::format("num: {}", 1234_u128) == "num: 1234");

#endif

// The alternate form adds no octal prefix to a zero, matching std::format for the builtins
template <typename T>
void test_zero_alternate()
{
    BOOST_TEST_CSTR_EQ(std::format("{:#o}", T{0}).c_str(), "0");
    BOOST_TEST_CSTR_EQ(std::format("{:#x}", T{0}).c_str(), "0x0");
    BOOST_TEST_CSTR_EQ(std::format("{:#X}", T{0}).c_str(), "0X0");
    BOOST_TEST_CSTR_EQ(std::format("{:#b}", T{0}).c_str(), "0b0");
    BOOST_TEST_CSTR_EQ(std::format("{:#o}", T{8}).c_str(), "010");
}

int main()
{
    test_zero_alternate<boost::int128::uint128>();
    test_zero_alternate<boost::int128::int128>();

    test_empty<boost::int128::uint128>();
    test_empty<boost::int128::int128>();

    test_binary<boost::int128::uint128>();
    test_binary<boost::int128::int128>();

    test_octal<boost::int128::uint128>();
    test_octal<boost::int128::int128>();

    test_decimal<boost::int128::uint128>();
    test_decimal<boost::int128::int128>();

    test_hex<boost::int128::uint128>();
    test_hex<boost::int128::int128>();

    test_string_insertion<boost::int128::uint128>();
    test_string_insertion<boost::int128::int128>();

    test_alignment<boost::int128::uint128>();
    test_alignment<boost::int128::int128>();
    test_alignment_negative<boost::int128::int128>();

    return boost::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
