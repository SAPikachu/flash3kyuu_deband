#pragma once

#include "constants.h"

// returns a random number in [-range, range]
int random(RANDOM_ALGORITHM algo, int& seed, int range);