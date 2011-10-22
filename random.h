#pragma once

typedef enum {
    RANDOM_ALGORITHM_OLD = 0,
    RANDOM_ALGORITHM_UNIFORM,
    RANDOM_ALGORITHM_GAUSSIAN,
    RANDOM_ALGORITHM_COUNT
} RANDOM_ALGORITHM;

// returns a random number in [-range, range]
short random(RANDOM_ALGORITHM algo, int& seed, short range);