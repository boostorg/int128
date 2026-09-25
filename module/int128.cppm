// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

module;

// The platform intrinsic headers and the MSVC built-in 128-bit header are not
// part of the standard library module, so they are always brought in here.
#if __has_include(<__msvc_int128.hpp>) && _MSVC_LANG >= 202002L

#include <__msvc_int128.hpp>

#endif

#ifdef __x86_64__
#  include <x86intrin.h>
#  include <emmintrin.h>
#elif defined(_M_AMD64)
#  include <intrin.h>
#elif defined(__i386__)
#  include <emmintrin.h>
#elif defined(_M_IX86)
#  include <intrin.h>
#endif

// import std exports declarations but not macros. The headers below provide the
// feature-test macros and the object-like macros the library uses (assert, errno,
// UINT64_C, ...) so they are included regardless of whether import std is used.
#include <version>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdint>

// When the standard library module is available these are provided by import std,
// otherwise they are supplied here in the global module fragment.
#ifndef BOOST_INT128_USE_STD_MODULE

#include <array>
#include <compare>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <format>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

// Unlike its neighbours above, this one is conditional: <stdfloat> only exists from
// C++23, and only when the standard library ships it.
#if (__cplusplus > 202002L || (defined(_MSVC_LANG) && _MSVC_LANG > 202002L)) && defined(__has_include) && __has_include(<stdfloat>)
#include <stdfloat>
#endif

#endif

#define BOOST_INT128_BUILD_MODULE

// Marks this translation unit as the module interface unit (as opposed to a
// consumer that imports the module). Some headers key off this to declare
// entities here that consumers instead receive through the import.
#define BOOST_INT128_INTERFACE_UNIT

export module boost.int128;

#ifdef BOOST_INT128_USE_STD_MODULE
import std;
#endif

// Wrap the exported declarations in extern "C++" so they attach to the global
// module (classic mangling), keeping the module and header forms ABI compatible.
extern "C++" {

export namespace boost::int128 {

struct int128;
struct uint128;

} // namespace boost::int128

export namespace std {

template <>
class numeric_limits<boost::int128::int128>;

template <>
class numeric_limits<boost::int128::uint128>;

} // namespace std

#ifdef _MSC_VER
#  pragma warning( push )
#  pragma warning( disable : 5244 )
#elif defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Winclude-angled-in-module-purview"
#endif

#include <boost/int128.hpp>

#ifdef _MSC_VER
#  pragma warning( pop )
#elif defined(__clang__)
#  pragma clang diagnostic pop
#endif

} // extern "C++"
