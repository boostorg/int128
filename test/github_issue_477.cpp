// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/int128/issues/477

#include <boost/int128.hpp>

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4459)
#endif

constexpr boost::int128::uint128 unsigned_value{1, 0};

constexpr bool a = unsigned_value;
static_assert(a, "Wrong value");

constexpr bool b(unsigned_value);
static_assert(b, "Wrong value");

constexpr bool c = static_cast<bool>(unsigned_value);
static_assert(c, "Wrong value");

constexpr boost::int128::int128 signed_value{1, 0};

constexpr bool d = signed_value;
static_assert(d, "Wrong value");

constexpr bool e(signed_value);
static_assert(e, "Wrong value");

constexpr bool f = static_cast<bool>(signed_value);
static_assert(f, "Wrong value");
