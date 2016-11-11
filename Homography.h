#pragma once

#include <vector>
#include "skew_matrix.h"

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
