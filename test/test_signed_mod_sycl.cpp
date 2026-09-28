//  Copyright 2026 Matt Borland
//  Distributed under the Boost Software License, Version 1.0.
//  https://www.boost.org/LICENSE_1_0.txt

#include "sycl_test.hpp"

using boost::int128::int128;

int main()
{
    // Force the one defined signed overflow (MIN % -1 wraps to 0) and its
    // non-overflowing neighbor onto the device path every run, not only when a
    // random draw happens to land on them.
    const int128_sycl_test::directed_pair<int128> overrides[] {
        {(std::numeric_limits<int128>::min)(), int128{-1}},
        {(std::numeric_limits<int128>::min)(), int128{1}},
    };

    return int128_sycl_test::run<int128, int128>(
        [](int128 a, int128 b, int) { return int128_sycl_test::safe_mod(a, b); },
        overrides, 2);
}
