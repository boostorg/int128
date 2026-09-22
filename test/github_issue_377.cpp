// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/int128/issues/377

#include <boost/int128.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>
#include <cstdint>

using namespace boost::int128;

template <typename T>
void test_div_by_one()
{
    constexpr auto min_val {std::numeric_limits<int128>::min()};
    BOOST_TEST_EQ(min_val, min_val / T{1});
}

template <typename T>
void test_other_vals()
{
    constexpr auto min_val {std::numeric_limits<int128>::min()};
    const auto min_div_2 {BOOST_INT128_INT128_C(-85070591730234615865843651857942052864)};
    const auto min_div_4 {BOOST_INT128_INT128_C(-42535295865117307932921825928971026432)};
    const auto min_div_16 {BOOST_INT128_INT128_C(-10633823966279326983230456482242756608)};

    BOOST_TEST_EQ(min_div_2, min_val / T{2});
    BOOST_TEST_EQ(min_div_4, min_val / T{4});
    BOOST_TEST_EQ(min_div_16, min_val / T{16});
}

// Bug 1: operator>>(int128, int128) was calling << instead of >>
void test_right_shift_int128_amount()
{
    const auto val {int128(0, 0xFF00)};
    const auto shift_4 {int128(0, 4)};

    // Right-shift with int128 shift amount must match integer shift
    BOOST_TEST_EQ(val >> shift_4, val >> 4);

    const auto expected_ff0 {int128(0, 0xFF0)};
    BOOST_TEST_EQ(val >> shift_4, expected_ff0);

    // Test >>= with int128 rhs
    auto val2 {val};
    val2 >>= shift_4;
    BOOST_TEST_EQ(val2, expected_ff0);

    // Cross-word shift
    const auto big_val {int128(0x1234, 0)};
    const auto shift_64 {int128(0, 64)};
    const auto expected_1234 {int128(0, 0x1234)};
    BOOST_TEST_EQ(big_val >> shift_64, expected_1234);

    // Arithmetic right shift preserves sign for negative values
    constexpr auto min_val {std::numeric_limits<int128>::min()};
    const auto shift_1 {int128(0, 1)};
    BOOST_TEST_EQ(min_val >> shift_1, min_val >> 1);
    BOOST_TEST((min_val >> shift_1) < 0);
}

// Bug 2: UnsignedInteger / int128 returned {rhs.high, res} instead of proper sign handling
void test_unsigned_div_negative_int128()
{
    const std::uint64_t lhs {10};
    const auto neg3 {-int128(0, 3)};
    const auto pos3 {int128(0, 3)};
    const auto expected_neg3 {-int128(0, 3)};
    const auto expected_pos3 {int128(0, 3)};

    // 10 / -3 = -3
    BOOST_TEST_EQ(lhs / neg3, expected_neg3);

    // 10 / 3 = 3
    BOOST_TEST_EQ(lhs / pos3, expected_pos3);

    // 7 / -1 = -7
    const std::uint64_t seven {7};
    const auto neg1 {-int128(0, 1)};
    const auto expected_neg7 {-int128(0, 7)};
    BOOST_TEST_EQ(seven / neg1, expected_neg7);
}

// Bug 3: UnsignedInteger % int128 used rhs.low instead of abs_rhs.low
// and applied wrong sign to remainder
void test_unsigned_mod_negative_int128()
{
    const std::uint64_t lhs {10};
    const auto neg3 {-int128(0, 3)};
    const auto pos3 {int128(0, 3)};
    const auto expected_1 {int128(0, 1)};

    // 10 % -3 = 1 (remainder has sign of dividend, which is unsigned/positive)
    BOOST_TEST_EQ(lhs % neg3, expected_1);

    // 10 % 3 = 1
    BOOST_TEST_EQ(lhs % pos3, expected_1);

    // 12 % -5 = 2
    const std::uint64_t twelve {12};
    const auto neg5 {-int128(0, 5)};
    const auto expected_2 {int128(0, 2)};
    BOOST_TEST_EQ(twelve % neg5, expected_2);
}

// Bug 4: operator%(int128, int128) early return was wrong when lhs = INT128_MIN
// because abs(INT128_MIN) overflows back to INT128_MIN
void test_min_val_modulo()
{
    constexpr auto min_val {std::numeric_limits<int128>::min()};
    const auto zero {int128(0, 0)};

    // INT128_MIN % 1 = 0
    const auto one {int128(0, 1)};
    BOOST_TEST_EQ(min_val % one, zero);

    // INT128_MIN % 2 = 0 (2^127 is even)
    const auto two {int128(0, 2)};
    BOOST_TEST_EQ(min_val % two, zero);

    // INT128_MIN % 3 = -2
    // -170141183460469231731687303715884105728 = -56713727820156410577229101238628035242 * 3 + (-2)
    const auto three {int128(0, 3)};
    const auto expected_neg2 {BOOST_INT128_INT128_C(-2)};
    BOOST_TEST_EQ(min_val % three, expected_neg2);

    // INT128_MIN % INT128_MIN = 0
    BOOST_TEST_EQ(min_val % min_val, zero);
}

int main()
{
    test_div_by_one<std::int32_t>();
    test_div_by_one<std::int64_t>();
    test_div_by_one<int128>();

    test_other_vals<std::int32_t>();
    test_other_vals<std::int64_t>();
    test_other_vals<int128>();

    test_right_shift_int128_amount();
    test_unsigned_div_negative_int128();
    test_unsigned_mod_negative_int128();
    test_min_val_modulo();

    return boost::report_errors();
}
