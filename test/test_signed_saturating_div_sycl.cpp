//  Copyright 2026 Matt Borland
//  Distributed under the Boost Software License, Version 1.0.
//  https://www.boost.org/LICENSE_1_0.txt

#include "sycl_test.hpp"

using boost::int128::int128;

int main()
{
    // Force the one input saturating_div actually saturates on (MIN / -1, which the
    // plain operator wraps to MIN but saturating_div clamps to MAX) onto the device
    // path every run, not only when a random draw happens to land on it.
    const int128_sycl_test::directed_pair<int128> overrides[] {
        {(std::numeric_limits<int128>::min)(), int128{-1}},
    };

    return int128_sycl_test::run<int128, int128>(
        [](int128 a, int128 b, int) { return boost::int128::saturating_div(a, int128_sycl_test::safe_divisor(a, b)); },
        overrides, 1);
}
