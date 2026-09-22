// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// This example demonstrates Boost.ContainerHash integration with int128 types.
// Including <boost/int128/hash.hpp> injects hash_value overloads for the library
// types, so boost::hash, boost::hash_combine, boost::hash_range, and the
// boost::unordered containers (which default to boost::hash) all work with no
// extra configuration.

#include <boost/int128/int128.hpp>
#include <boost/int128/hash.hpp>
// tag::exclude[]
// The Boost.ContainerHash and Boost.Unordered headers trip strict warnings the
// test build enables (old-style casts, and a float-to-double promotion in
// std::ceil on older clang); silence them for these third-party headers only.
// This is not needed in normal use.
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable: 4848)
#endif
// end::exclude[]
#include <boost/container_hash/hash.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
// tag::exclude[]
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif
// end::exclude[]
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>

using boost::int128::uint128;
using boost::int128::int128;

// A user-defined composite key that holds 128-bit fields. Providing a hash_value
// overload in the type's own namespace lets Boost.ContainerHash find it via ADL,
// and boost::hash_combine reuses the int128 hashes supplied by hash.hpp.
struct point
{
    int128 x;
    int128 y;
};

bool operator==(const point& lhs, const point& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

std::size_t hash_value(const point& p)
{
    std::size_t seed {0};
    boost::hash_combine(seed, p.x);
    boost::hash_combine(seed, p.y);
    return seed;
}

int main()
{
    std::cout << "=== boost::hash on 128-bit types ===" << std::endl;

    // boost::hash<T> dispatches to the hash_value overloads from hash.hpp, which
    // delegate to std::hash, so the two functors always agree.
    const uint128 big {UINT64_C(0xDEADBEEF), UINT64_C(0xCAFEBABE12345678)};
    const int128 neg {-123456789012345678LL};

    std::cout << "boost::hash matches std::hash (uint128): "
              << std::boolalpha << (boost::hash<uint128>{}(big) == std::hash<uint128>{}(big)) << std::endl;
    std::cout << "boost::hash matches std::hash (int128):  "
              << (boost::hash<int128>{}(neg) == std::hash<int128>{}(neg)) << std::endl;

    std::cout << "\n=== boost::unordered_map<uint128, ...> ===" << std::endl;

    // boost::unordered_map defaults to boost::hash<Key>, so uint128 keys need
    // no explicit hasher.
    boost::unordered_map<uint128, std::string> labels {};
    labels[uint128{1, 0}] = "two to the sixty-fourth";                   // 2^64
    labels[uint128{UINT64_C(0x8000000000000000), 0}] = "two to the one hundred twenty-seventh"; // 2^127
    labels[uint128{42}] = "forty-two";

    std::cout << "Entries: " << labels.size() << std::endl;
    std::cout << "Label at 2^64: " << labels[uint128{1, 0}] << std::endl;
    std::cout << "Contains 42: " << (labels.find(uint128{42}) != labels.end()) << std::endl;

    std::cout << "\n=== hash_combine for a composite key ===" << std::endl;

    // The point hasher combines two int128 fields; boost::hash<point> finds it
    // via ADL, letting point be used as a key directly.
    boost::unordered_map<point, long> populations {};
    populations[point{10, 20}] = 5000000;
    populations[point{-30, 40}] = 250000;

    std::cout << "Cities stored: " << populations.size() << std::endl;
    std::cout << "Population at (10, 20): " << populations[point{10, 20}] << std::endl;
    std::cout << "Same coordinate hashes equal: "
              << (boost::hash<point>{}(point{10, 20}) == boost::hash<point>{}(point{10, 20})) << std::endl;

    std::cout << "\n=== boost::unordered_flat_map<int128, ...> ===" << std::endl;

    // The modern flat container also defaults to boost::hash.
    boost::unordered_flat_map<int128, int> counts {};
    counts[int128{-1}] = 1;
    counts[int128{0}] = 2;
    counts[int128{1}] = 3;

    std::cout << "Flat map size: " << counts.size() << std::endl;
    std::cout << "counts[-1] = " << counts[int128{-1}] << std::endl;

    return 0;
}
