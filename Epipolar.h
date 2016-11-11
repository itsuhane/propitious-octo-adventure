#pragma once

#include <vector>
#include "skew_matrix.h"

/*
    When q = Rp+T maps coordinate from p in camera system 1 to q in camera system 2.
    Gives the essential matrix that satisfy q^T*E*p = 0.
    ** R must be a proper rotation matrix.
*/
inline Eigen::Matrix3d compose_essential(const Eigen::Matrix3d &R, const Eigen::Vector3d &T) {
    return skew_matrix(T)*R;
}

/*
    Essential matrix must be a rank-2 matrix with two singular-values equal to 1.
*/
inline bool fix_essential(const Eigen::Matrix3d &E, Eigen::Matrix3d &fixed_E) {
    Eigen::Vector3d fs{ 1.0, 1.0, 0.0 };
    Eigen::JacobiSVD<Eigen::Matrix3d> svd(E, Eigen::ComputeFullU | Eigen::ComputeFullV);
    Eigen::Vector3d s = svd.singularValues();
    fixed_E = svd.matrixU()*fs.asDiagonal()*svd.matrixV().transpose();
    if (s[1] / s[0] > 0) {
        return true;
    }
    else {
        return false;
    }
}

/*
    Recovering Baseline and Orientation from 'Essential' Matrix, B.K.P. Horn, 1990.
    Each E gives a pair of R, T. However notice that E and -E is indistinguisable just
    from epipolar constraint, hence there are 4 groups of solutions.
    Among the 4, only one solution where points are in front of both cameras.
*/
inline bool decompose_essential(const Eigen::Matrix3d &E, Eigen::Matrix3d &R1, Eigen::Matrix3d &R2, Eigen::Vector3d &T1, Eigen::Vector3d &T2) {
    Eigen::Matrix3d EET = E*E.transpose();
    double halfTrace = 0.5*EET.trace();
    Eigen::Vector3d b;

    Eigen::Vector3d e0e1 = E.col(0).cross(E.col(1));
    Eigen::Vector3d e1e2 = E.col(1).cross(E.col(2));
    Eigen::Vector3d e2e0 = E.col(2).cross(E.col(0));

#if 0
    Eigen::Matrix3d bbT = halfTrace*Eigen::Matrix3d::Identity() - EET;
    Eigen::Vector3d bbT_diag = bbT.diagonal();
    if (bbT_diag(0) > bbt_diag(1) && bbT_diag(0) > bbT_diag(2)) {
        b = bbT.row(0) / sqrt(bbT_diag(0));
    }
    else if (bbT_diag(1) > bbT_diag(0) && bbT_diag(1) > bbT_diag(2)) {
        b = bbT.row(1) / sqrt(bbT_diag(1));
    }
    else {
        b = bbT.row(2) / sqrt(bbT_diag(2));
    }
#else
    if (e0e1.norm() > e1e2.norm() && e0e1.norm() > e2e0.norm()) {
        b = e0e1.normalized()*sqrt(halfTrace);
    }
    else if (e1e2.norm() > e0e1.norm() && e1e2.norm() > e2e0.norm()) {
        b = e1e2.normalized()*sqrt(halfTrace);
    }
    else {
        b = e2e0.normalized()*sqrt(halfTrace);
    }
#endif

    Eigen::Matrix3d cofactorsT;
    cofactorsT.col(0) = e1e2;
    cofactorsT.col(1) = e2e0;
    cofactorsT.col(2) = e0e1;

    R1 = (cofactorsT - skew_matrix(b)*E) / b.dot(b);
    T1 = b;
    R2 = (cofactorsT + skew_matrix(b)*E) / b.dot(b);
    T2 = -b;

    return true;
}

/*
    Solve essential matrix E that pb^T E pa = 0.
    In function, pa and pb are their projective coordinate, assuming the last factor is 1.
    pa and pb should be normalized, if not, use solve_essential.
*/
inline bool solve_essential_normalized(const std::vector<Eigen::Vector2d> &pa, const std::vector<Eigen::Vector2d> &pb, Eigen::Matrix3d &E) {
    if (pa.size() < 8 || pa.size() != pb.size()) {
        return false;
    }

    Eigen::MatrixXd A;
    A.resize(pa.size(), 9);

    for (size_t i = 0; i < pa.size(); ++i) {
        A(i, 0) = pa[i](0)*pb[i](0);
        A(i, 1) = pa[i](0)*pb[i](1);
        A(i, 2) = pa[i](0);

        A(i, 3) = pa[i](1)*pb[i](0);
        A(i, 4) = pa[i](1)*pb[i](1);
        A(i, 5) = pa[i](1);

        A(i, 6) = pb[i](0);
        A(i, 7) = pb[i](1);
        A(i, 8) = 1;
    }

    Eigen::VectorXd e = A.jacobiSvd(Eigen::ComputeFullV).matrixV().col(8);
    E = Eigen::Map<Eigen::Matrix3d>(e.data());

    return true;
}

// Solve essential matrix with coordinate normalization
inline bool solve_essential(const std::vector<Eigen::Vector2d> &pa, const std::vector<Eigen::Vector2d> &pb, Eigen::Matrix3d &E) {
    if (pa.size() < 8 || pa.size() != pb.size()) {
        return false;
    }
    //else if (pa.size() == 8) {
    //    return solve_essential_normalized(pa, pb, E);
    //}

    Eigen::Vector2d pa_mean = Eigen::Vector2d::Zero();
    Eigen::Vector2d pb_mean = Eigen::Vector2d::Zero();
    for (size_t i = 0; i < pa.size(); ++i) {
        pa_mean += pa[i];
        pb_mean += pb[i];
    }
    pa_mean /= (double)pa.size();
    pb_mean /= (double)pb.size();

    double sa = 0;
    double sb = 0;
    double sqrt2 = sqrt(2.0);

    for (size_t i = 0; i < pa.size(); ++i) {
        sa += (pa[i] - pa_mean).norm();
        sb += (pb[i] - pb_mean).norm();
    }

    sa = 1.0 / (sqrt2*sa);
    sb = 1.0 / (sqrt2*sb);

    std::vector<Eigen::Vector2d> na(pa.size());
    std::vector<Eigen::Vector2d> nb(pb.size());
    for (size_t i = 0; i < pa.size(); ++i) {
        na[i] = (pa[i] - pa_mean)*sa;
        nb[i] = (pb[i] - pb_mean)*sb;
    }

    solve_essential_normalized(na, nb, E);

    Eigen::Matrix3d Na, Nb;
    Nb <<            sb,              0, 0,
                      0,             sb, 0,
         -sb*pb_mean(0), -sb*pb_mean(1), 1;
    Na << sa,  0, -sa*pa_mean(0),
           0, sa, -sa*pa_mean(1),
           0,  0,              1;

    E = Nb*E*Na;

    return true;
}