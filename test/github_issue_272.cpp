// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/int128/issues/272

#include <boost/int128.hpp>
#include <boost/core/lightweight_test.hpp>
#include <sstream>
#include <iostream>

template <typename T>
void endpos_using_istream(const std::string& str, const int expected_endpos)
{
    std::istringstream is(str);
    T x;

    is >> x;
    is.clear();
    const auto endpos = is.tellg();

    BOOST_TEST_EQ(endpos, expected_endpos);
}

template <typename T>
void check_endpos()
{
    endpos_using_istream<T>("Inf", 0);
    endpos_using_istream<T>("Nan", 0);

    endpos_using_istream<T>("127.0.0.1", 3);
    endpos_using_istream<T>("42", 2);

    endpos_using_istream<T>("42Junk", 2);
}

int main()
{
    check_endpos<boost::int128::uint128>();
    check_endpos<boost::int128::int128>();

    return boost::report_errors();
}
