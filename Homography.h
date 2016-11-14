#pragma once

#include <vector>
#include <Eigen/Eigen>

/*
When q = Rp+T maps coordinate from p in camera system 1 to q in camera system 2,
a plane has normal n (in camera system 1), and distance d to the origin of system 1.
** R must be a proper rotation matrix.
*/
inline Eigen::Matrix3d compose_homography(const Eigen::Matrix3d &R, const Eigen::Vector3d &T, const Eigen::Vector3d &n, double d = 1.0) {
    return R + (T / d)*n.transpose();
}

/*
Deeper understanding of the homography decomposition for vision-based control, E. Malis, M. Vargas, 2007.
Function returns two results: <R1, T1, n1> and <R2, T2, n2>.
And the system has a total of 4 results: <R1, T1, n1>, <R1, -T1, -n1>, <R2, T2, n2> and <R2, -T2, -n2>.
*/
inline bool decompose_homography(const Eigen::Matrix3d &H, Eigen::Matrix3d &R1, Eigen::Matrix3d &R2, Eigen::Vector3d &T1, Eigen::Vector3d &T2, Eigen::Vector3d &n1, Eigen::Vector3d &n2) {
    Eigen::Matrix3d S = H.transpose()*H - Eigen::Matrix3d::Identity();
    bool isPureRotation = true;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (abs(S(i, j)) > 1e-9) {
                isPureRotation = false;
                goto decompose_homography_continued;
            }
        }
    }

decompose_homography_continued:
    if (isPureRotation) {
        // pure rotation
        //Eigen::JacobiSVD<Eigen::Matrix3d> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);
        R1 = R2 = H; // svd.matrixU()*Eigen::Matrix3d::Identity()*svd.matrixV().transpose();
        T1 = T2 = Eigen::Vector3d::Zero();
        n1 = n2 = Eigen::Vector3d::Zero();
    }
    else {
        double Ms00 = S(1, 2)*S(1, 2) - S(1, 1)*S(2, 2);
        double Ms11 = S(0, 2)*S(0, 2) - S(0, 0)*S(2, 2);
        double Ms22 = S(0, 1)*S(0, 1) - S(0, 0)*S(1, 1);
        double sqrtMs00 = sqrt(Ms00);
        double sqrtMs11 = sqrt(Ms11);
        double sqrtMs22 = sqrt(Ms22);

        double nu = 2.0*sqrt(1 + S.trace() - Ms00 - Ms11 - Ms22);
        double tenormsq = 2 + S.trace() - nu;

        Eigen::Vector3d tstar1, tstar2;

        if (S(0, 0) > S(1, 1) && S(0, 0) > S(2, 2)) {
            double epslMs12 = (((S(0, 1)*S(0, 2) - S(0, 0)*S(1, 2)) < 0) ? -1 : 1);
            n1 << S(0, 0), S(0, 1) + sqrtMs22, S(0, 2) + epslMs12*sqrtMs11;
            n2 << S(0, 0), S(0, 1) - sqrtMs22, S(0, 2) - epslMs12*sqrtMs11;
            tstar1 = n1.norm()*n2 / S(0, 0);
            tstar2 = n2.norm()*n1 / S(0, 0);
        }
        else if (S(1, 1) > S(0, 0) && S(1, 1) > S(2, 2)) {
            double epslMs02 = (((S(1, 1)*S(0, 2) - S(0, 1)*S(1, 2)) < 0) ? -1 : 1);
            n1 << S(0, 1) + sqrtMs22, S(1, 1), S(1, 2) - epslMs02*sqrtMs00;
            n2 << S(0, 1) - sqrtMs22, S(1, 1), S(1, 2) + epslMs02*sqrtMs00;
            tstar2 = n2.norm()*n1 / S(1, 1);
            tstar1 = n1.norm()*n2 / S(1, 1);
        }
        else {
            double epslMs01 = (((S(1, 2)*S(0, 2) - S(0, 1)*S(2, 2)) < 0) ? -1 : 1);
            n1 << S(0, 2) + epslMs01*sqrtMs11, S(1, 2) + sqrtMs00, S(2, 2);
            n2 << S(0, 2) - epslMs01*sqrtMs11, S(1, 2) - sqrtMs00, S(2, 2);
            tstar1 = n1.norm()*n2 / S(2, 2);
            tstar2 = n2.norm()*n1 / S(2, 2);
        }
        n1.normalize();
        n2.normalize();
        tstar1 -= tenormsq*n1;
        tstar2 -= tenormsq*n2;
        R1 = H*(Eigen::Matrix3d::Identity() - (tstar1 / nu)*n1.transpose());
        R2 = H*(Eigen::Matrix3d::Identity() - (tstar2 / nu)*n2.transpose());
        tstar1 *= 0.5;
        tstar2 *= 0.5;
        T1 = R1*tstar1;
        T2 = R2*tstar2;
    }
    return true;
}


bool solve_homography_normalized(const std::vector<Eigen::Vector2d> &pa, const std::vector<Eigen::Vector2d> &pb, Eigen::Matrix3d &H) {
    if (pa.size() < 4 || pa.size() != pb.size()) {
        return false;
    }

    Eigen::MatrixXd A;
    A.resize(pa.size() * 2, 9);
    A.setZero();

    for (size_t i = 0; i < pa.size(); ++i) {
        const Eigen::Vector2d &a = pa[i];
        const Eigen::Vector2d &b = pb[i];
        A(i * 2, 1)     = -a(0)     ;
        A(i * 2, 2)     =  a(0)*b(1);
        A(i * 2, 4)     = -a(1)     ;
        A(i * 2, 5)     =  a(1)*b(1);
        A(i * 2, 7)     = -   1     ;
        A(i * 2, 8)     =       b(1);
        A(i * 2 + 1, 0) =  a(0)     ;
        A(i * 2 + 1, 2) = -a(0)*b(0);
        A(i * 2 + 1, 3) =  a(1)     ;
        A(i * 2 + 1, 5) = -a(1)*b(0);
        A(i * 2 + 1, 6) =     1     ;
        A(i * 2 + 1, 8) = -     b(0);
    }

    Eigen::VectorXd h = A.jacobiSvd(Eigen::ComputeFullV).matrixV().col(8);
    H = Eigen::Map<Eigen::Matrix3d>(h.data());

    return true;
}

bool solve_homography(const std::vector<Eigen::Vector2d> &pa, const std::vector<Eigen::Vector2d> &pb, Eigen::Matrix3d &H) {
    if (pa.size() < 4 || pa.size() != pb.size()) {
        return false;
    }
    //else if (pa.size() == 4) {
    //    return solve_homography_normalized(pa, pb, H);
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

    solve_homography_normalized(na, nb, H);

    Eigen::Matrix3d Na, Nb;
    Nb << 1/sb,    0, pb_mean(0),
             0, 1/sb, pb_mean(1),
             0,    0,          1;
    Na << sa,  0, -sa*pa_mean(0),
           0, sa, -sa*pa_mean(1),
           0,  0,              1;

    H = Nb*H*Na;

    return true;
}
