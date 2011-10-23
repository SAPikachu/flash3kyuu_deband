#include "stdafx.h"

#include "random.h"

#include <math.h>

#include <assert.h>

typedef float (*rand_impl_t)(int& seed);

float rand_old(int& seed);

float rand_uniform(int& seed);

float rand_gaussian(int& seed);

static const rand_impl_t rand_algorithms[] = {
    rand_old,
    rand_uniform,
    rand_gaussian
};

float round(float r) {
    return (r > 0.0f) ? floor(r + 0.5f) : ceil(r - 0.5f);
}

short random(RANDOM_ALGORITHM algo, int& seed, short range)
{
    assert(algo >= 0 && algo < RANDOM_ALGORITHM_COUNT);

    float num = rand_algorithms[algo](seed);
    assert(num >= -1.0 && num <= 1.0);
    return (short)round(num * range);
}

// most algorithms below are stolen from AddGrainC

// maps lower 23 bits of an int32 to [-1.0, 1.0)
float rand_to_float(int rand_num)
{
    unsigned long itemp = 0x3f800000 | (0x007fffff & rand_num);
    return ((*(float*)&itemp) - 1.0f) * 2 - 1.0f;
}

float rand_old(int& seed)
{
    int seed_tmp = (((seed << 13) ^ (unsigned int)seed) >> 17) ^ (seed << 13) ^ seed;
    seed = 32 * seed_tmp ^ seed_tmp;
    return rand_to_float(seed);
}

float rand_uniform(int& seed)
{
    seed = 1664525 * seed + 1013904223;
    return rand_to_float(seed);
}

// http://www.bearcave.com/misl/misl_tech/wavelets/hurst/random.html
float rand_gaussian(int& seed)
{
    double ret;
    double x, y, r2;

    do
    {
        do
        {
            /* choose x,y in uniform square (-1,-1) to (+1,+1) */

            x = rand_uniform (seed);
            y = rand_uniform (seed);

            /* see if it is in the unit circle */
            r2 = x * x + y * y;
        }
        while (r2 > 1.0 || r2 == 0);
        /* Box-Muller transform */

        ret = y * sqrt (-2.0 * log (r2) / r2);

    } while (ret <= -1.0 || ret >= 1.0);
    // we need to clip the result because the wrapper accepts [-1.0, 1.0] only

    return (float)ret;
}