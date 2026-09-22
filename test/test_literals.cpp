// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT128_BUILD_MODULE

#include <boost/int128/int128.hpp>
#include <boost/int128/literals.hpp>
#include <boost/int128/iostream.hpp>

#else

import boost.int128;
// The convenience macros are not part of the module interface (macros never are).
#include <boost/int128/detail/literal_macros.hpp>

#endif

#include <boost/core/lightweight_test.hpp>
#include <limits>
#include <string>

using namespace boost::int128::literals;

void test_u128_literals()
{
    BOOST_TEST(boost::int128::uint128{0} == 0_u128);
    BOOST_TEST(boost::int128::uint128{10} == "10"_U128);
    BOOST_TEST(boost::int128::uint128{0} == BOOST_INT128_UINT128_C(0));

    const boost::int128::uint128 max_val {std::numeric_limits<boost::int128::uint128>::max()};
    const auto macro_val {BOOST_INT128_UINT128_C(340282366920938463463374607431768211455)};
    BOOST_TEST(max_val == macro_val);
}

// Warning with only MSVC 14.1
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4307)
#endif

void test_u128_digit_separators()
{
    // C++ digit separators (') are ignored inside numeric literals
    BOOST_TEST(boost::int128::uint128{1234567} == 1'234'567_u128);
    BOOST_TEST(boost::int128::uint128{1000} == 1'000_U128);
    BOOST_TEST(boost::int128::uint128{1234} == 12'34_u128);

    // Separators are also honored through the convenience macro
    BOOST_TEST(boost::int128::uint128{1234} == BOOST_INT128_UINT128_C(1'234));

    // The string form of the literal skips separators as well
    BOOST_TEST(boost::int128::uint128{1234} == "1'234"_u128);
    BOOST_TEST(boost::int128::uint128{1234} == "1'234"_U128);

    // Full-width value with a separator between every group of three digits
    const boost::int128::uint128 max_val {std::numeric_limits<boost::int128::uint128>::max()};
    BOOST_TEST(max_val == 340'282'366'920'938'463'463'374'607'431'768'211'455_u128);

    // MSVC 14.1 ICE
    #if !defined(_MSC_VER) || _MSC_VER >= 1920
    // Separators must be usable in a constant expression
    static_assert(100'000_u128 == boost::int128::uint128{100000}, "constexpr separator");
    #endif
}

void test_u128_base_prefixes()
{
    // A C++ base prefix is recognized and the digits parsed in that base

    // Hexadecimal (0x / 0X)
    BOOST_TEST(boost::int128::uint128{255} == 0xFF_u128);
    BOOST_TEST(boost::int128::uint128{255} == 0XfF_U128);
    BOOST_TEST(boost::int128::uint128{255} == "0xff"_u128);
    BOOST_TEST(boost::int128::uint128{0xDEADBEEFULL} == 0xDEAD'BEEF_u128);

    // Binary (0b / 0B)
    BOOST_TEST(boost::int128::uint128{10} == 0b1010_u128);
    BOOST_TEST(boost::int128::uint128{240} == 0B1111'0000_U128);

    // Octal (leading 0)
    BOOST_TEST(boost::int128::uint128{511} == 0777_u128);
    BOOST_TEST(boost::int128::uint128{8} == 010_u128);
    BOOST_TEST(boost::int128::uint128{0} == 00_u128);

    // A full-width value written in hexadecimal is the maximum
    const boost::int128::uint128 max_val {std::numeric_limits<boost::int128::uint128>::max()};
    BOOST_TEST(max_val == 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF_u128);

    // MSVC 14.1 ICE
    #if !defined(_MSC_VER) || _MSC_VER >= 1920
    // Prefixes must be usable in a constant expression
    static_assert(0x10_u128 == boost::int128::uint128{16}, "constexpr hex");
    static_assert(0b100_u128 == boost::int128::uint128{4}, "constexpr binary");
    static_assert(010_u128 == boost::int128::uint128{8}, "constexpr octal");
    #endif
}

void test_i128_literals()
{
    BOOST_TEST(boost::int128::int128{0} == 0_i128);
    BOOST_TEST(boost::int128::int128{10} == "10"_I128);
    BOOST_TEST(boost::int128::int128{0} == BOOST_INT128_INT128_C(0));

    const boost::int128::int128 max_val {std::numeric_limits<boost::int128::int128>::max()};
    const auto macro_val {BOOST_INT128_INT128_C(170141183460469231731687303715884105727)};
    BOOST_TEST(max_val == macro_val);

    const boost::int128::int128 min_val {std::numeric_limits<boost::int128::int128>::min()};
    const auto min_macro_val {BOOST_INT128_INT128_C(-170141183460469231731687303715884105728)};
    BOOST_TEST(min_val == min_macro_val);
    
    BOOST_TEST("-42"_i128 == -42);
}

void test_i128_digit_separators()
{
    // C++ digit separators (') are ignored inside numeric literals
    BOOST_TEST(boost::int128::int128{1000} == 1'000_i128);
    BOOST_TEST(boost::int128::int128{9999} == 9'999_I128);

    // A leading unary minus is applied after the (separated) literal is parsed
    BOOST_TEST(boost::int128::int128{-1000000} == -1'000'000_i128);

    // Separators are also honored through the convenience macro
    BOOST_TEST(boost::int128::int128{9999} == BOOST_INT128_INT128_C(9'999));

    // The string form of the literal skips separators as well
    BOOST_TEST(boost::int128::int128{1234} == "1'234"_i128);
    BOOST_TEST(boost::int128::int128{1234} == "1'234"_I128);

    // Full-width value with a separator between every group of three digits
    const boost::int128::int128 max_val {std::numeric_limits<boost::int128::int128>::max()};
    BOOST_TEST(max_val == 170'141'183'460'469'231'731'687'303'715'884'105'727_i128);

    // MSVC 14.1 ICE
    #if !defined(_MSC_VER) || _MSC_VER >= 1920
    // Separators must be usable in a constant expression
    static_assert(100'000_i128 == boost::int128::int128{100000}, "constexpr separator");
    #endif
}

void test_i128_base_prefixes()
{
    // Hexadecimal, binary and octal prefixes are recognized for the signed type too
    BOOST_TEST(boost::int128::int128{127} == 0x7F_i128);
    BOOST_TEST(boost::int128::int128{255} == "0xFF"_I128);
    BOOST_TEST(boost::int128::int128{10} == 0b1010_i128);
    BOOST_TEST(boost::int128::int128{511} == 0777_i128);

    // A leading unary minus is applied after the (prefixed) literal is parsed
    BOOST_TEST(boost::int128::int128{-255} == -0xFF_i128);
    BOOST_TEST(boost::int128::int128{-511} == -0777_i128);

    // The string form may embed the sign ahead of the prefix
    BOOST_TEST(boost::int128::int128{-255} == "-0xFF"_i128);
    BOOST_TEST(boost::int128::int128{-10} == "-0b1010"_I128);
    BOOST_TEST(boost::int128::int128{-511} == "-0777"_i128);

    // A full-width positive value written in hexadecimal is the maximum
    const boost::int128::int128 max_val {std::numeric_limits<boost::int128::int128>::max()};
    BOOST_TEST(max_val == 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF_i128);

    // MSVC 14.1 ICE
    #if !defined(_MSC_VER) || _MSC_VER >= 1920
    static_assert(-0x10_i128 == boost::int128::int128{-16}, "constexpr signed hex");
    #endif
}

// The literal operators are noexcept: a malformed or out-of-range literal is a compile-time
// error in a constant expression (compile_tests pin that) and terminates at run time. A
// value that fits still parses at run time through the string form.
void test_noexcept_contract()
{
    static_assert(noexcept(boost::int128::literals::operator ""_u128("0")), "literal operators are noexcept");
    static_assert(noexcept(boost::int128::literals::operator ""_U128("0")), "literal operators are noexcept");
    static_assert(noexcept(boost::int128::literals::operator ""_i128("0")), "literal operators are noexcept");
    static_assert(noexcept(boost::int128::literals::operator ""_I128("0")), "literal operators are noexcept");
    static_assert(noexcept(boost::int128::literals::operator ""_u128("0", 1U)), "literal operators are noexcept");
    static_assert(noexcept(boost::int128::literals::operator ""_i128("0", 1U)), "literal operators are noexcept");

    const std::string max_str {"340282366920938463463374607431768211455"};
    const auto max_val {boost::int128::literals::operator ""_u128(max_str.c_str(), max_str.size())};
    BOOST_TEST(max_val == std::numeric_limits<boost::int128::uint128>::max());

    const std::string min_str {"-170141183460469231731687303715884105728"};
    const auto min_val {boost::int128::literals::operator ""_i128(min_str.c_str(), min_str.size())};
    BOOST_TEST(min_val == std::numeric_limits<boost::int128::int128>::min());
}

int main()
{
    test_u128_literals();
    test_u128_digit_separators();
    test_u128_base_prefixes();
    test_i128_literals();
    test_i128_digit_separators();
    test_i128_base_prefixes();
    test_noexcept_contract();

    return boost::report_errors();
}
