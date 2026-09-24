// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int128.hpp>
#include <boost/core/lightweight_test.hpp>
#include <random>
#include <cmath>

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsign-compare"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#ifdef BOOST_INT128_HAS_INT128

static std::mt19937_64 rng{42};
// Use sqrt-bounded ranges so multiplication doesn't overflow the 64-bit oracle, but cover
// negative signed values to exercise the sign-extension path.
static std::uniform_int_distribution<std::uint64_t> u_dist{1, static_cast<std::uint64_t>(std::sqrt(UINT64_MAX))};
static std::uniform_int_distribution<std::int64_t> i_dist{
    -static_cast<std::int64_t>(std::sqrt(INT64_MAX)),
    static_cast<std::int64_t>(std::sqrt(INT64_MAX))};
static constexpr std::size_t N {1024U};

using namespace boost::int128;

void test()
{
    using boost::int128::detail::builtin_u128;

    for (std::size_t i {0}; i < N; ++i)
    {
        const auto u_val {u_dist(rng)};
        const auto i_val {i_dist(rng)};
        if (i_val == 0)
        {
            continue;  // skip divide/modulo by zero
        }

        const uint128 lhs_u {u_val};
        const int128 rhs_i {i_val};

        // Builtin oracle: both operands promoted to unsigned __int128
        const builtin_u128 builtin_lhs {u_val};
        const builtin_u128 builtin_rhs = static_cast<builtin_u128>(static_cast<__int128>(i_val));

        BOOST_TEST_EQ(lhs_u + rhs_i, uint128{builtin_lhs + builtin_rhs});
        BOOST_TEST_EQ(lhs_u - rhs_i, uint128{builtin_lhs - builtin_rhs});
        BOOST_TEST_EQ(lhs_u * rhs_i, uint128{builtin_lhs * builtin_rhs});
        BOOST_TEST_EQ(lhs_u / rhs_i, uint128{builtin_lhs / builtin_rhs});
        BOOST_TEST_EQ(lhs_u % rhs_i, uint128{builtin_lhs % builtin_rhs});

        // Reverse operand order
        if (u_val == 0)
        {
            continue;
        }
        BOOST_TEST_EQ(rhs_i + lhs_u, uint128{builtin_rhs + builtin_lhs});
        BOOST_TEST_EQ(rhs_i - lhs_u, uint128{builtin_rhs - builtin_lhs});
        BOOST_TEST_EQ(rhs_i * lhs_u, uint128{builtin_rhs * builtin_lhs});
        BOOST_TEST_EQ(rhs_i / lhs_u, uint128{builtin_rhs / builtin_lhs});
        BOOST_TEST_EQ(rhs_i % lhs_u, uint128{builtin_rhs % builtin_lhs});
    }
}

void test_compound_assignment()
{
    using boost::int128::detail::builtin_u128;

    for (std::size_t i {0}; i < N; ++i)
    {
        auto i_val {i_dist(rng)};
        if (i_val > 0)
        {
            i_val = -i_val;  // force the left operand negative every iteration
        }
        if (i_val == 0)
        {
            continue;
        }

        auto u_val {u_dist(rng)};
        if (u_val == 0)
        {
            continue;  // skip divide/modulo by zero
        }

        const uint128 rhs_u {u_val};
        const builtin_u128 builtin_rhs {u_val};

        #define BOOST_INT128_CHECK_COMPOUND(op) \
        { \
            int128 lhs_i {i_val}; \
            lhs_i op rhs_u; \
            builtin_u128 builtin_lhs = static_cast<builtin_u128>(static_cast<__int128>(i_val)); \
            builtin_lhs op builtin_rhs; \
            BOOST_TEST_EQ(lhs_i, int128{builtin_lhs}); \
        }

        BOOST_INT128_CHECK_COMPOUND(|=)
        BOOST_INT128_CHECK_COMPOUND(&=)
        BOOST_INT128_CHECK_COMPOUND(^=)
        BOOST_INT128_CHECK_COMPOUND(+=)
        BOOST_INT128_CHECK_COMPOUND(-=)
        BOOST_INT128_CHECK_COMPOUND(*=)
        BOOST_INT128_CHECK_COMPOUND(/=)
        BOOST_INT128_CHECK_COMPOUND(%=)

        #undef BOOST_INT128_CHECK_COMPOUND

        // Shifts are the exception: value and result type come from the int128 lhs
        // (>> stays arithmetic), and only the count comes from the uint128 rhs, so the
        // reference is int128's own shift, not the uint128 magnitude's logical one.
        {
            int128 lhs_shl {i_val};
            const uint128 count {3U};
            lhs_shl <<= count;
            BOOST_TEST_EQ(lhs_shl, int128{i_val} << count);
        }
        {
            int128 lhs_shr {i_val};
            const uint128 count {3U};
            lhs_shr >>= count;
            BOOST_TEST_EQ(lhs_shr, int128{i_val} >> count);
        }

        // uint128 /= int128 needs no dedicated overload: it already resolves through
        // the member, converting the int128 operand to uint128 first (a bit copy that
        // lands directly in the domain the operation runs in), matching the builtin.
        {
            uint128 lhs_div {rhs_u};
            const int128 rhs_i {i_val};
            lhs_div /= rhs_i;
            builtin_u128 builtin_div {builtin_rhs};
            builtin_div /= static_cast<builtin_u128>(static_cast<__int128>(i_val));
            BOOST_TEST_EQ(lhs_div, uint128{builtin_div});
        }
    }
}

#endif // BOOST_INT128_HAS_INT128

int main()
{
#ifdef BOOST_INT128_HAS_INT128
    test();
    test_compound_assignment();
#endif

    return boost::report_errors();
}
