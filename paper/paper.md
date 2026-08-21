---
title: 'Boost.Int128: A C++14 Library for Portable and Performant 128-Bit Integer Arithmetic'
tags:
  - C++
  - GPU
  - CUDA
  - SYCL
authors:
  - name: Matt Borland
    orcid: 0009-0005-8183-4254
    affiliation: 1
affiliations:
 - name: The C++ Alliance, United States
   index: 1
date: 21 August 2026
bibliography: paper.bib
---

# Summary

Boost.Int128 is a header-only, C++14 implementation of 128-bit integer arithmetic through two types, `int128` and `uint128`, that behave like built-in integers and integrate with the C++ Standard Library and other Boost libraries [@boost].
Both types are exactly 128 bits wide on every supported platform, and both are usable unchanged in GPU kernels through CUDA [@Nickolls:2008] and SYCL [@Reyes:2020].
The library is available at <https://github.com/boostorg/int128>, and in the Boost distribution starting with version 1.93.

# Statement of need

C++ specifies no integer type wider than `long long`, which in practice means 64 bits, yet quantities exceeding that range are common.
An IPv6 address [@rfc4291] and a UUID [@rfc9562] are each 128 bits, so each needs a 128-bit integer or a byte array that cannot be compared, masked, or incremented directly.
The widening 64x64 to 128-bit product is the central primitive of unbiased random integer generation in an interval [@Lemire:2019] and of shortest round-trip floating-point formatting [@Adams:2018].
GCC and Clang expose `__int128` only on 64-bit targets, with no `std::numeric_limits`, `<iostream>`, or `std::hash` support, and MSVC lacks it entirely, so dependent code either restricts its portability or maintains a second implementation.
The portable alternative, a general multiprecision integer, pays for that generality in object size and in throughput.
The types began as the arithmetic backend of Boost.Decimal [@Borland2026], which needs an exact 128-bit product on every platform Boost supports, and were extracted and generalized once that need proved broader than one library.
What is needed is a type that is exactly 16 bytes everywhere, as fast as the compiler's extension where that exists, and usable across the host-to-device boundary without an interchange format.

# State of the field

The portable host implementations are Abseil's `absl::int128` and `absl::uint128` [@abseil], Boost.Multiprecision's 128-bit `cpp_int` instantiation [@multiprecision], and MSVC's undocumented `std::_Signed128` and `std::_Unsigned128` [@msvc_int128].
Abseil is a close functional match, but it requires C++17 and GCC 10 or later and carries no device annotations.
Boost.Multiprecision's types instantiate a general `number` template rather than a purpose-built class, so they carry an extra word of bookkeeping and cost more per operation.

On the device side, `nvcc` accepts `__int128` only when the host compiler supports it [@cuda_guide], which excludes every MSVC and 32-bit host, and SYCL's `spir64` target has no native 128-bit integer at all [@sycl_spec].
The implementations filling this gap, such as CUDA-uint128 [@cuda_uint128], are unsigned only and tied to one toolchain.
We are not aware of another implementation offering one 128-bit type usable on the host and in both CUDA and SYCL device code with bitwise identical results.

# Software design

The library requires only C++14, is header-only, and has no external dependencies, so a user need only `#include <boost/int128.hpp>`, even with a toolchain as old as `g++-5`.
A single-header amalgamation is published, and under C++20 the library can be consumed as a module via `import boost.int128`.
Relaxed C++14 `constexpr` covers nearly the whole interface.

Each type is a struct of two `std::uint64_t` words ordered by target endianness, so the object representation and alignment match a native 128-bit integer.
Keeping both words unsigned lets the compiler treat the pair as one wide access, so loops over these types vectorize rather than scalarize.
Operators forward to a native 128-bit type or intrinsic where one exists, and follow Knuth [@Knuth:1998] and Warren [@Warren:2013] over the two words elsewhere, with identical results.
The library also supplies the interfaces the extension lacks, among them `<limits>`, `<bit>`, `<charconv>`, `<format>`, `<iostream>`, hashing, Boost.Random and Boost.Math integration, saturating arithmetic, the C23 checked-arithmetic interface [@c23], and the `div_*` family of P3724 [@p3724].

Continuous integration runs the test suite natively on Linux, macOS, and Windows across x86_64, x86_32, aarch64, ARM32v7, ARM64, and big-endian s390x, plus PPC64LE under QEMU, using GCC 5 and later, Clang 5 and later, Visual Studio 2017 and later, Intel oneAPI DPC++, and NVCC.
Because half of these targets have no native 128-bit type, the software fallbacks are tested as thoroughly as the intrinsic paths.
GPU support is expressed through one annotation macro, `BOOST_INT128_HOST_DEVICE`, which expands to `__host__ __device__` under `nvcc` and `SYCL_EXTERNAL` under a SYCL compiler.
More than eighty CUDA and eighty SYCL tests compare device results against the host, so agreement across that boundary is tested rather than assumed.

Where a native type exists, Boost.Int128 matches or beats it: with GCC 14.2 on Linux x86_64, `uint128` division and modulo are roughly 16 percent faster than `unsigned __int128`, and signed multiplication is 1.5 times faster on big-endian s390x.
Where no native type exists, the margins are larger: signed multiplication is 1.7 times faster than `std::_Signed128` on MSVC x86_64 and 6.2 times faster on MSVC ARM64, and 32-bit x86 addition is 8 times faster than Boost.Multiprecision.
A handful of cells favor an alternative, so the complete tables, rerun on every pull request, are published rather than a summary, at <https://develop.int128.cpp.al/int128/u128_benchmarks.html> and <https://develop.int128.cpp.al/int128/i128_benchmarks.html>.

Figure 1 shows the difference between a 64-bit and a 128-bit product.

```c++
#include <boost/int128.hpp>
#include <cstdint>
#include <iostream>

int main()
{
    using boost::int128::uint128;

    // Two values that each fit in 64 bits, but whose product does not
    constexpr std::uint64_t a {11400714819323198485ULL};
    constexpr std::uint64_t b {14029467366897019727ULL};

    // A 64-bit product silently wraps modulo 2^64
    std::cout << " 64-bit: a * b = " << a * b << std::endl;

    // The same multiplication is exact when the result is 128 bits wide
    constexpr uint128 exact {uint128{a} * uint128{b}};
    std::cout << "128-bit: a * b = " << exact << std::endl;

    // Like a built-in integer, the types work in constant expressions
    static_assert(exact / uint128{b} == uint128{a}, "Recovers a exactly");
    static_assert(sizeof(uint128) == 16, "Exactly 128 bits on every platform");

    return 0;
}
```

```text
 64-bit: a * b = 17693923505768731003
128-bit: a * b = 159945956516994065446882290339781513595
```

*Figure 1: The 64-bit product wraps modulo 2^64 while the `uint128` product is exact and compile-time checked. The lower block is the program output.*

# Research Impact Statement

Boost.Int128 supports reproducible numerical research wherever 64 bits of integer precision are not enough.
Computational number theory, cryptography, and hashing reduce products of 64-bit residues and need the full 128-bit intermediate, as do combinatorial enumeration and exact sums of products.
Because the same type gives the same results on every architecture, big-endian or little-endian, and with or without a native 128-bit type underneath, a result computed on one machine is bit-for-bit reproducible on another.

The device support extends that guarantee to accelerators.
A researcher can port a kernel to a GPU without substituting a numeric type and can validate the port against the host because the two agree by specification.
Replacing a floating-point accumulator with a 128-bit integer one makes a parallel reduction independent of how the work is partitioned, since integer addition is associative.
Supporting both CUDA and SYCL means this holds across vendors.

# AI usage disclosure

Generative AI tools were used in developing this library and preparing this manuscript, for drafting and editing prose, platform-specific optimizations, and generating test cases.
All library code, benchmark results, and text were reviewed and verified by the author, who takes full responsibility for their correctness.

# Acknowledgements

We thank The C++ Alliance for sponsoring this work, the domain experts who peer-reviewed the library in July 2026 before its acceptance into the Boost library collection, and Arnaud Bechler for managing that review.

# References
