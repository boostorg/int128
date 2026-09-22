// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int128/int128.hpp>
#include <boost/int128/hash.hpp>

// The Boost.ContainerHash and Boost.Unordered headers trip strict warnings
// enabled by the test build (old-style casts, and a float-to-double promotion
// in std::ceil on older clang); silence them for these third-party headers only.
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable: 4848)
#endif

#include <boost/container_hash/hash.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_set.hpp>

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif

#include <boost/core/lightweight_test.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <vector>

// boost::hash<T> dispatches (via ADL) to boost::int128::hash_value, which in
// turn delegates to std::hash. All three entry points must agree.
void test_boost_hash_matches_std_and_hash_value()
{
    using boost::int128::uint128;
    using boost::int128::int128;

    const boost::hash<uint128> u_hasher {};
    const std::hash<uint128> u_std {};

    std::mt19937_64 rng {42};
    for (int i {0}; i < 1024; ++i)
    {
        const uint128 v {rng(), rng()};
        BOOST_TEST_EQ(u_hasher(v), boost::int128::hash_value(v));
        BOOST_TEST_EQ(u_hasher(v), u_std(v));
    }

    const boost::hash<int128> s_hasher {};
    const std::hash<int128> s_std {};

    for (int i {0}; i < 1024; ++i)
    {
        const int128 v {static_cast<std::int64_t>(rng()), rng()};
        BOOST_TEST_EQ(s_hasher(v), boost::int128::hash_value(v));
        BOOST_TEST_EQ(s_hasher(v), s_std(v));
    }
}

// Equal values must hash equal through boost::hash.
void test_boost_hash_equal_values()
{
    using boost::int128::uint128;
    using boost::int128::int128;

    const boost::hash<uint128> u_hasher {};
    const uint128 ua {UINT64_C(0xDEADBEEF), UINT64_C(0xCAFEBABE12345678)};
    const uint128 ub {UINT64_C(0xDEADBEEF), UINT64_C(0xCAFEBABE12345678)};
    BOOST_TEST_EQ(u_hasher(ua), u_hasher(ub));

    const boost::hash<int128> s_hasher {};
    const int128 sa {INT64_C(-1), UINT64_C(0xCAFEBABE)};
    const int128 sb {INT64_C(-1), UINT64_C(0xCAFEBABE)};
    BOOST_TEST_EQ(s_hasher(sa), s_hasher(sb));

    // A value and its negation must not collide.
    const boost::hash<int128> hasher {};
    for (std::int64_t i {1}; i <= 512; ++i)
    {
        const int128 pos {i};
        const int128 neg {-i};
        BOOST_TEST_NE(hasher(pos), hasher(neg));
    }
}

// boost::hash_combine must be deterministic and order sensitive.
void test_hash_combine()
{
    using boost::int128::uint128;

    const uint128 a {UINT64_C(1), UINT64_C(2)};
    const uint128 b {UINT64_C(3), UINT64_C(4)};

    std::size_t seed1 {0};
    boost::hash_combine(seed1, a);
    boost::hash_combine(seed1, b);

    std::size_t seed2 {0};
    boost::hash_combine(seed2, a);
    boost::hash_combine(seed2, b);

    // Same values combined in the same order produce the same seed.
    BOOST_TEST_EQ(seed1, seed2);

    std::size_t seed3 {0};
    boost::hash_combine(seed3, b);
    boost::hash_combine(seed3, a);

    // Order matters when the combined values differ.
    BOOST_TEST_NE(seed1, seed3);
}

// boost::hash_range over a container of 128-bit values.
void test_hash_range()
{
    using boost::int128::uint128;

    const std::vector<uint128> v1 {uint128{0, 1}, uint128{1, 0}, uint128{UINT64_MAX, UINT64_MAX}};
    const std::vector<uint128> v2 {uint128{0, 1}, uint128{1, 0}, uint128{UINT64_MAX, UINT64_MAX}};
    const std::vector<uint128> v3 {uint128{1, 0}, uint128{0, 1}, uint128{UINT64_MAX, UINT64_MAX}};

    // Equal ranges hash equal.
    BOOST_TEST_EQ(boost::hash_range(v1.begin(), v1.end()), boost::hash_range(v2.begin(), v2.end()));

    // Reordered ranges hash differently.
    BOOST_TEST_NE(boost::hash_range(v1.begin(), v1.end()), boost::hash_range(v3.begin(), v3.end()));
}

// boost::unordered_map defaults to boost::hash, so the library types drop in
// with no explicit hasher.
void test_unordered_map_default_hash()
{
    using boost::int128::uint128;
    using boost::int128::int128;

    boost::unordered_map<uint128, int> umap {};
    umap[uint128{0, 1}] = 1;
    umap[uint128{0, 2}] = 2;
    umap[uint128{1, 0}] = 3;
    umap[uint128{UINT64_MAX, UINT64_MAX}] = 4;

    BOOST_TEST_EQ((umap[uint128{0, 1}]), 1);
    BOOST_TEST_EQ((umap[uint128{0, 2}]), 2);
    BOOST_TEST_EQ((umap[uint128{1, 0}]), 3);
    BOOST_TEST_EQ((umap[uint128{UINT64_MAX, UINT64_MAX}]), 4);
    BOOST_TEST_EQ(umap.size(), (std::size_t {4}));

    boost::unordered_map<int128, int> smap {};
    smap[int128{0, 1}] = 1;
    smap[int128{-1, UINT64_MAX}] = 2;
    smap[int128{1, 0}] = 3;
    smap[int128{-1, 0}] = 4;

    BOOST_TEST_EQ((smap[int128{0, 1}]), 1);
    BOOST_TEST_EQ((smap[int128{-1, UINT64_MAX}]), 2);
    BOOST_TEST_EQ((smap[int128{1, 0}]), 3);
    BOOST_TEST_EQ((smap[int128{-1, 0}]), 4);
    BOOST_TEST_EQ(smap.size(), (std::size_t {4}));
}

// boost::unordered_set likewise deduplicates via boost::hash.
void test_unordered_set_default_hash()
{
    using boost::int128::uint128;

    boost::unordered_set<uint128> set {};
    set.insert(uint128{0, 1});
    set.insert(uint128{0, 1}); // duplicate
    set.insert(uint128{1, 0});

    BOOST_TEST_EQ(set.size(), (std::size_t {2}));
    BOOST_TEST(set.find(uint128{0, 1}) != set.end());
    BOOST_TEST(set.find(uint128{1, 0}) != set.end());
    BOOST_TEST(set.find(uint128{9, 9}) == set.end());
}

int main()
{
    test_boost_hash_matches_std_and_hash_value();
    test_boost_hash_equal_values();
    test_hash_combine();
    test_hash_range();
    test_unordered_map_default_hash();
    test_unordered_set_default_hash();

    return boost::report_errors();
}
