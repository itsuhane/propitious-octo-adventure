#pragma once

// the PRNG module in C++ <random> needs too many setup code!

#include <random>

inline unsigned int get_random_seed() {
    static std::random_device rd;
    return rd();
}

template<typename T>
class GaussianNoise {
public:
    typedef T value_type;

    GaussianNoise(value_type sigma = value_type(1.0), value_type mean = value_type(0.0)) : distribution(mean, sigma) {
        seed();
    }

    void seed() {
        engine.seed(get_random_seed());
    }

    void seed(unsigned int value) {
        engine.seed(value);
    }

    value_type next() {
        return distribution(engine);
    }

private:
    std::default_random_engine engine;
    std::normal_distribution<value_type> distribution;
};

/*
GaussianNoise<double> rng(2.0);
rng.seed(961216); // for deterministic randomness
double v = rng.next();
*/