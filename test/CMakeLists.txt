# Copyright 2018, 2019 Peter Dimov
# Copyright 2023 Matt Borland
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt

include(BoostTestJamfile OPTIONAL RESULT_VARIABLE HAVE_BOOST_TEST)

if(HAVE_BOOST_TEST)

    enable_testing()

    if(BOOST_INT128_ENABLE_CUDA)

        message(STATUS "Building Boost.int128 with CUDA")

        enable_language(CUDA)
        find_package(CUDAToolkit REQUIRED)
        set(CMAKE_CUDA_EXTENSIONS OFF)

        enable_testing()

        boost_test_jamfile(FILE cuda_jamfile LINK_LIBRARIES Boost::int128 Boost::random Boost::charconv CUDA::cudart COMPILE_DEFINITIONS BOOST_INT128_ENABLE_CUDA=1  )

    elseif(BOOST_INT128_ENABLE_SYCL)

        message(STATUS "Building Boost.int128 with SYCL")

        set(CMAKE_CXX_COMPILER "icpx")
        set(CMAKE_C_COMPILER "icx")

        # -fsycl must be on the link line too (it bundles the device image); COMPILE_OPTIONS
        # only reaches the compile step, so add it to the linker flags as well.
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsycl")

        enable_testing()

        boost_test_jamfile(FILE sycl_jamfile LINK_LIBRARIES Boost::int128 Boost::charconv sycl COMPILE_DEFINITIONS BOOST_INT128_ENABLE_SYCL=1 COMPILE_OPTIONS -fsycl)

    elseif(BOOST_INT128_BUILD_MODULE)

        message(STATUS "Building Boost.int128 as a C++ module")

        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../module" module)

    else()

        boost_test_jamfile(FILE Jamfile LINK_LIBRARIES Boost::int128 Boost::core Boost::random Boost::multiprecision Boost::mp11 Boost::charconv Boost::container_hash Boost::unordered)

        boost_test(SOURCES check_cmake_version.cpp ARGUMENTS ${PROJECT_VERSION} LINK_LIBRARIES Boost::int128 Boost::core)

    endif()

endif()
