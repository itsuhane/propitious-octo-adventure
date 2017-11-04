#pragma once

#include <Eigen/Eigen>
#include <assert.h>
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

    static inline Real epsilon() {
        return Real(NumTraits<_Real>::epsilon());
    }
    static inline Real dummy_precision() {
        return Real(NumTraits<_Real>::dummy_precision());
    }
};
#if 0
namespace internal {

template<class Base>
struct significant_decimals_default_impl<jet<Base>, false> {
    typedef jet<Base> Scalar;
    typedef typename NumTraits<Scalar>::Real RealScalar;

    static inline int run() {
        Scalar neg_log_eps = -log(NumTraits<RealScalar>::epsilon());
        int ceil_neg_log_eps = int(neg_log_eps.value());
        return (Scalar(ceil_neg_log_eps) < neg_log_eps) ? (ceil_neg_log_eps + 1) : ceil_neg_log_eps;
    }
};
}
#endif

}

template<typename _Real, int _Rows, int _Cols>
void make_variable(Eigen::Matrix<jet<_Real>, _Rows, _Cols>& x) {
    for (int c = 0; c < x.cols(); ++c) {
        for (int r = 0; r < x.rows(); ++r) {
            make_variable(x(r, c));
        }
    }
}

template<typename _Real, int _Rows, int _Cols>
Eigen::Matrix<_Real, _Rows, _Cols> partial(const Eigen::Matrix<jet<_Real>, _Rows, _Cols>& f, const jet<_Real>& x) {
    Eigen::Matrix<_Real, _Rows, _Cols> result(f.rows(), f.cols());
    for (int c = 0; c < f.cols(); ++c) {
        for (int r = 0; r < f.rows(); ++r) {
            result(r, c) = partial(f(r, c), x);
        }
    }
    return result;
}

template<typename _Real, int _Rows, int _Cols>
Eigen::Matrix<_Real, 1, _Rows> partial(const jet<_Real>& f, const Eigen::Matrix<jet<_Real>, _Rows, _Cols>& x) {
    if (x.cols() == 1) {
        Eigen::Matrix<_Real, 1, _Rows> result(1, x.rows());
        for (int r = 0; r < x.rows(); ++r) {
            result(r) = partial(f, x(r));
        }
        return result;
    } else {
        assert(("x must be column-vector, please use vec() to vectorize it") && (x.cols() != 1));
        return Eigen::Matrix<_Real, 1, _Rows>();
    }
}

template<typename _Real, int _RowsY, int _ColsY, int _RowsX, int _ColsX>
typename std::enable_if<_ColsY!=1&&_ColsX!=1, Eigen::Matrix<_Real, _RowsY, _RowsX>>::type partial(const Eigen::Matrix<jet<_Real>, _RowsY, _ColsY>& y, const Eigen::Matrix<jet<_Real>, _RowsX, _ColsX>& x) {
    if (y.cols() == 1 && x.cols() == 1) {
        Eigen::Matrix<_Real, _RowsY, _RowsX> result(y.rows(), x.rows());
        for (int ry = 0; ry < y.rows(); ++ry) {
            for (int rx = 0; rx < x.rows(); ++rx) {
                result(ry, rx) = partial(y(ry), x(rx));
            }
        }
        return result;
    }
    else {
        assert(("x and y must be column-vector, please use vec() to vectorize them") && (x.cols() != 1 || y.cols() != 1));
        return Eigen::Matrix<_Real, _RowsY, _RowsX>();
    }
}

template<typename _Real, int _RowsY, int _RowsX, int _ColsX>
typename std::enable_if<_ColsX != 1, Eigen::Matrix<_Real, _RowsY, _RowsX>>::type partial(const Eigen::Matrix<jet<_Real>, _RowsY, 1>& y, const Eigen::Matrix<jet<_Real>, _RowsX, _ColsX>& x) {
    if (x.cols() == 1) {
        Eigen::Matrix<_Real, _RowsY, _RowsX> result(y.rows(), x.rows());
        for (int ry = 0; ry < y.rows(); ++ry) {
            for (int rx = 0; rx < x.rows(); ++rx) {
                result(ry, rx) = partial(y(ry), x(rx));
            }
        }
        return result;
    }
    else {
        assert(("x must be column-vector, please use vec() to vectorize it") && (x.cols() != 1));
        return Eigen::Matrix<_Real, _RowsY, _RowsX>();
    }
}

template<typename _Real, int _RowsY, int _ColsY, int _RowsX>
typename std::enable_if<_ColsY != 1, Eigen::Matrix<_Real, _RowsY, _RowsX>>::type partial(const Eigen::Matrix<jet<_Real>, _RowsY, _ColsY>& y, const Eigen::Matrix<jet<_Real>, _RowsX, 1>& x) {
    if (y.cols() == 1) {
        Eigen::Matrix<_Real, _RowsY, _RowsX> result(y.rows(), x.rows());
        for (int ry = 0; ry < y.rows(); ++ry) {
            for (int rx = 0; rx < x.rows(); ++rx) {
                result(ry, rx) = partial(y(ry), x(rx));
            }
        }
        return result;
    }
    else {
        assert(("y must be column-vector, please use vec() to vectorize it") && (y.cols() != 1));
        return Eigen::Matrix<_Real, _RowsY, _RowsX>();
    }
}

template<typename _Real, int _RowsY, int _RowsX>
Eigen::Matrix<_Real, _RowsY, _RowsX> partial(const Eigen::Matrix<jet<_Real>, _RowsY, 1>& y, const Eigen::Matrix<jet<_Real>, _RowsX, 1>& x) {
    Eigen::Matrix<_Real, _RowsY, _RowsX> result(y.rows(), x.rows());
    for (int ry = 0; ry < y.rows(); ++ry) {
        for (int rx = 0; rx < x.rows(); ++rx) {
            result(ry, rx) = partial(y(ry), x(rx));
        }
    }
    return result;
}

template<typename _Real, int _Rows, int _Cols>
Eigen::Matrix<jet<_Real>, Eigen::internal::size_at_compile_time<_Rows, _Cols>::ret, 1>
vec(const Eigen::Matrix<jet<_Real>, _Rows, _Cols>& x) {
    Eigen::Map<const Eigen::Matrix<jet<_Real>, _Rows, _Cols>> v(x.data(), x.rows()*x.cols(), 1);
    return v;
}
