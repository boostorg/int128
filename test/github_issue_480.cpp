// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/int128/issues/479

#define BOOST_HAS_INT128

#ifndef BOOST_INT128_NO_BUILTIN_INT128
#define BOOST_INT128_NO_BUILTIN_INT128
#endif

#include <boost/int128.hpp>

#ifdef BOOST_INT128_HAS_INT128
#error "Should not be defined"
#endif
