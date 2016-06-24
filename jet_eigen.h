#pragma once

#include <Eigen/Eigen>
#include "jet.h"

namespace Eigen {
    template<typename _Real>
    struct NumTraits<jet<_Real> > {
        typedef jet<_Real> Real;
        enum {
            IsComplex = 0,
            IsInteger = 0,
            IsSigned = 1,
            RequireInitialization = 1,
            ReadCost = 1,
            AddCost = 1,
            MulCost = 5,
            HasFloatingPoint = 1
        };

        static inline Real epsilon() { return Real(NumTraits<_Real>::epsilon()); }
        static inline Real dummy_precision() { return Real(NumTraits<Real>::dummy_precision()); }
    };
}

template<typename _Real, int _Rows, int _Cols>
Eigen::Matrix<_Real, _Rows, _Cols> nabla(const jet<_Real> &f, const Eigen::Matrix<jet<_Real>, _Rows, _Cols> &x) {
    Eigen::Matrix<_Real, _Rows, _Cols> result;
    for (int c = 0; c < _Cols; ++c) {
        for (int r = 0; r < _Rows; ++r) {
            result(r, c) = f.partial(x(r, c));
        }
    }
    return result;
}
