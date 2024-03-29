//
// Created by Linbin Yang on 2019-12-28.
//

#ifndef C_INFERENCE_ACTIVATOR_H
#define C_INFERENCE_ACTIVATOR_H
#include <cmath>

namespace MATHLIB{
    template<typename T>
    T sigmoid(T x){
        return T(1.0)/(T(1.0) + exp(-x));
    }

    template<typename T>
    T relu(T x){
        return x < T(0.0) ? T(0.0) : x;
    }

    template<typename T>
    T broadcastAdd(T x, T y){
        return x + y;
    }
}

#endif