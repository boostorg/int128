//  Copyright John Maddock 2016.
//  Copyright Matt Borland 2024 - 2026.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <limits>
#include <boost/int128.hpp>
#include <boost/int128/random.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "cuda_managed_ptr.hpp"
#include "stopwatch.hpp"

// For the CUDA runtime routines (prefixed with "cuda_")
#include <cuda_runtime.h>

using test_type = boost::int128::int128;

/**
 * CUDA Kernel Device code
 *
 */
__global__ void cuda_test(const test_type *in, const test_type *in2, test_type *out, int numElements)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;

    if (i < numElements)
    {
        out[i] = in[i] / in2[i];
    }
}

/**
 * Host main routine
 */
int main(void)
{
    std::mt19937_64 rng {42};

    // Error code to check return values for CUDA calls
    cudaError_t err = cudaSuccess;

    // Print the vector length to be used, and compute its size
    int numElements = 50000;
    std::cout << "[Vector operation on " << numElements << " elements]" << std::endl;

    // Allocate the managed input vector A
    cuda_managed_ptr<test_type> input_vector(numElements);
    cuda_managed_ptr<test_type> input_vector2(numElements);

    // Allocate the managed output vector C
    cuda_managed_ptr<test_type> output_vector(numElements);

    // Initialize the input vectors. Avoid zero divisors; use full signed range for both inputs
    boost::random::uniform_int_distribution<test_type> dist {(std::numeric_limits<test_type>::min)(), (std::numeric_limits<test_type>::max)()};
    for (std::size_t i = 0; i < numElements; ++i)
    {
        input_vector[i] = dist(rng);
        // Ensure non-zero divisor
        do
        {
            input_vector2[i] = dist(rng);
        } while (input_vector2[i] == 0);
    }

    // Force the one defined signed overflow (MIN / -1 wraps to MIN) and its
    // non-overflowing neighbor onto the device path every run, not only when a
    // random draw happens to land on them.
    if (numElements > 1)
    {
        input_vector[0] = (std::numeric_limits<test_type>::min)();
        input_vector2[0] = test_type{-1};
        input_vector[1] = (std::numeric_limits<test_type>::min)();
        input_vector2[1] = test_type{1};
    }

    // Launch the Vector Add CUDA Kernel
    int threadsPerBlock = 256;
    int blocksPerGrid =(numElements + threadsPerBlock - 1) / threadsPerBlock;
    std::cout << "CUDA kernel launch with " << blocksPerGrid << " blocks of " << threadsPerBlock << " threads" << std::endl;

    watch w;

    cuda_test<<<blocksPerGrid, threadsPerBlock>>>(input_vector.get(), input_vector2.get(), output_vector.get(), numElements);
    cudaDeviceSynchronize();

    std::cout << "CUDA kernal done in: " << w.elapsed() << "s" << std::endl;

    err = cudaGetLastError();

    if (err != cudaSuccess)
    {
        std::cerr << "Failed to launch vectorAdd kernel (error code " << cudaGetErrorString(err) << ")!" << std::endl;
        return EXIT_FAILURE;
    }

    // Verify that the result vector is correct
    std::vector<test_type> results;
    results.reserve(numElements);
    w.reset();
    for(int i = 0; i < numElements; ++i)
    {
        results.push_back(input_vector[i] / input_vector2[i]);
    }
    double t = w.elapsed();
    // check the results
    for(int i = 0; i < numElements; ++i)
    {
        if (output_vector[i] != results[i])
        {
            std::cerr << "Result verification failed at element " << i << "!" << '\n'
                      << "Got: " << output_vector[i] << "\n"
                      << "Expected: " << results[i] << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "Test PASSED, normal calculation time: " << t << "s" << std::endl;
    std::cout << "Done\n";

    return 0;
}
