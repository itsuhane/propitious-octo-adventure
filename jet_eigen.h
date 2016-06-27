#pragma once

#include <Eigen/Eigen>
#include "jet.h"

namespace Eigen {
    template<typename _Real>
    struct NumTraits<jet<_Real> > : GenericNumTraits<_Real> {
        typedef jet<_Real> Real;
        typedef jet<_Real> NonInteger;
        typedef jet<_Real> Nested;
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
        static inline Real dummy_precision() { return Real(NumTraits<_Real>::dummy_precision()); }
    };

    namespace internal {

        template<class Base>
        struct significant_decimals_default_impl<jet<Base>, false>
        {
            typedef jet<Base> Scalar;
            typedef typename NumTraits<Scalar>::Real RealScalar;

            static inline int run()
            {
                Scalar neg_log_eps = -log(NumTraits<RealScalar>::epsilon());
                int ceil_neg_log_eps = int(neg_log_eps.value());
                return (Scalar(ceil_neg_log_eps) < neg_log_eps) ? (ceil_neg_log_eps + 1) : ceil_neg_log_eps;
            }
        };
    }

}

template<typename _Real, int _Rows, int _Cols>
Eigen::Matrix<_Real, _Rows, _Cols> partial(const Eigen::Matrix<jet<_Real>, _Rows, _Cols> &f, const jet<_Real> &x) {
    Eigen::Matrix<_Real, _Rows, _Cols> result;
    for (int c = 0; c < _Cols; ++c) {
        for (int r = 0; r < _Rows; ++r) {
            result(r, c) = f(r, c).partial(x);
        }
    }
    return result;
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