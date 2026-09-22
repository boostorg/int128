// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/int128/issues/221

#include <boost/int128.hpp>
#include <boost/core/lightweight_test.hpp>
#include <random>
#include <limits>

constexpr std::size_t N {1024u};
static std::mt19937_64 gen(42);

template <typename T>
void test()
{
    constexpr T numerator {0};

    using dist_type = std::conditional_t<std::numeric_limits<T>::is_signed, std::int64_t, std::uint64_t>;
    std::uniform_int_distribution<dist_type> dist {std::numeric_limits<dist_type>::min(), std::numeric_limits<dist_type>::max()};

    for (std::size_t i {0}; i < N; ++i)
    {
        const auto value {dist(gen)};
        BOOST_TEST_EQ(numerator / value, numerator);
    }
}

int main()
{
    test<boost::int128::int128>();
    test<boost::int128::uint128>();

    return boost::report_errors();
}
