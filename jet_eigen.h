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