// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Exhaustive compile coverage for uint128/int128 crossed with the compiler's own
// builtin 128-bit types (detail::builtin_u128/builtin_i128), in both operand orders,
// for every binary and compound operator, plus construction and conversion both ways.
//
// On GCC/clang this exercises native __int128/unsigned __int128 (fundamental types,
// never ambiguous with a user-defined operator). On MSVC /std:c++20 it exercises
// std::_Unsigned128/_Signed128 (class types with their own hidden-friend operators),
// which is where a missing exact-match overload turns into a C2666 ambiguity: MSVC's
// own hidden friend and this library's operator taking two library-type operands both
// need one user-defined conversion, on different arguments, and different conversion
// functions are never comparable to each other, so neither wins unless a third,
// EXACT match candidate (this file's whole reason to exist) is also in the running.
//
// No Boost.Random: both libraries' existing test_i128.cpp/test_u128.cpp exclude
// builtin_i128 (std::_Signed128) on MSVC because Boost.Random's distributions do not
// support it, which means no existing test anywhere instantiates these operators with
// a _Signed128 operand on MSVC. This file is deliberately self-contained so it has no
// such gap, and runs (as a permanent compile test) on every configuration including
// MSVC CI.
//
// A no-op before HAS_INT128/HAS_MSVC_INT128 (32-bit MSVC, or a portable-only build).

#include <boost/int128.hpp>

#if defined(BOOST_INT128_HAS_INT128) || defined(BOOST_INT128_HAS_MSVC_INT128)

using boost::int128::uint128;
using boost::int128::int128;
using boost::int128::detail::builtin_u128;
using boost::int128::detail::builtin_i128;

template <typename Lib, typename Builtin>
void test_construction_and_conversion()
{
    const Builtin b {2};
    const Lib from_builtin {b};
    static_cast<void>(from_builtin);

    const Lib l {3};
    const auto to_builtin {static_cast<Builtin>(l)};
    static_cast<void>(to_builtin);
}

template <typename Lib, typename Builtin>
void test_binary_ops()
{
    const Lib l {5};
    const Builtin b {2};

    // Comparisons, both orders
    static_cast<void>(l == b);
    static_cast<void>(b == l);
    static_cast<void>(l != b);
    static_cast<void>(b != l);
    static_cast<void>(l < b);
    static_cast<void>(b < l);
    static_cast<void>(l <= b);
    static_cast<void>(b <= l);
    static_cast<void>(l > b);
    static_cast<void>(b > l);
    static_cast<void>(l >= b);
    static_cast<void>(b >= l);

    #ifdef BOOST_INT128_HAS_SPACESHIP_OPERATOR
    static_cast<void>(l <=> b);
    static_cast<void>(b <=> l);
    #endif

    // Arithmetic, both orders
    static_cast<void>(l + b);
    static_cast<void>(b + l);
    static_cast<void>(l - b);
    static_cast<void>(b - l);
    static_cast<void>(l * b);
    static_cast<void>(b * l);
    static_cast<void>(l / b);
    static_cast<void>(b / l);
    static_cast<void>(l % b);
    static_cast<void>(b % l);

    // Bitwise, both orders
    static_cast<void>(l & b);
    static_cast<void>(b & l);
    static_cast<void>(l | b);
    static_cast<void>(b | l);
    static_cast<void>(l ^ b);
    static_cast<void>(b ^ l);

    // Shifts: the builtin operand is a count on one side, a value on the other
    static_cast<void>(l << b);
    static_cast<void>(b << l);
    static_cast<void>(l >> b);
    static_cast<void>(b >> l);
}

template <typename Lib, typename Builtin>
void test_compound_ops()
{
    Lib l {5};
    const Builtin b {2};

    l += b;
    l -= b;
    l *= b;
    l /= b;
    l %= b;
    l &= b;
    l |= b;
    l ^= b;
    l <<= b;
    l >>= b;

    static_cast<void>(l);
}

template <typename Lib, typename Builtin>
void test_all()
{
    test_construction_and_conversion<Lib, Builtin>();
    test_binary_ops<Lib, Builtin>();
    test_compound_ops<Lib, Builtin>();
}

#endif // BOOST_INT128_HAS_INT128 || BOOST_INT128_HAS_MSVC_INT128

int main()
{
    #if defined(BOOST_INT128_HAS_INT128) || defined(BOOST_INT128_HAS_MSVC_INT128)

    test_all<uint128, builtin_u128>();
    test_all<uint128, builtin_i128>();
    test_all<int128, builtin_u128>();
    test_all<int128, builtin_i128>();

    #endif

    return 0;
}
