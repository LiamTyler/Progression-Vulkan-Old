#pragma once

#include <time.h>
#include <stdlib.h>

namespace Progression { namespace random {

    inline void setSeed(unsigned int seed = time(NULL)) {
        srand(seed);
    }

    inline int randInt(int l, int h) {
        return l + rand() % (h - l);
    }

    inline float randFloat(float l, float h) {
        float r = rand() / static_cast<float>(RAND_MAX);
        return r * (h - l) + l;
    }

} } // namespace Progression::random
