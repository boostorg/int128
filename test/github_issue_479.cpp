// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/int128/issues/479

#include <boost/int128.hpp>
#include <boost/core/lightweight_test.hpp>

template <typename T>
void test()
{
    constexpr auto value = T{1} << 127;
    BOOST_TEST_EQ(value, boost::int128::saturating_mul(value, T{1}));
}

int main()
{
    test<boost::int128::uint128>();
    test<boost::int128::int128>();

    return boost::report_errors();
}
