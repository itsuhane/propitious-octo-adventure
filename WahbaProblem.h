#pragma once

#include <vector>
#include <Eigen/Eigen>

inline Eigen::Matrix3d Kabsch(const std::vector<Eigen::Vector3d> &src, const std::vector<Eigen::Vector3d> &dst) {
    Eigen::Matrix3d cov;
    for (size_t i = 0; i < src.size(); ++i) {
        cov += src[i] * dst[i].transpose();
    }
    cov = cov * (1.0 / src.size());
    Eigen::JacobiSVD<Eigen::Matrix3d> svd(cov, Eigen::ComputeFullU | Eigen::ComputeFullV);
    const Eigen::Matrix3d &U = svd.matrixU();
    const Eigen::Matrix3d &V = svd.matrixV();
    Eigen::Matrix3d E = Eigen::Matrix3d::Identity();
    if ((V*U.transpose()).determinant() >= 0.0) {
        E(2, 2) = 1.0;
    }
    else {
        E(2, 2) = -1.0;
    }

    return V*E*U.transpose();
}

inline std::tuple<double, Eigen::Matrix3d, Eigen::Vector3d> FindSRT(std::vector<Eigen::Vector3d> src, std::vector<Eigen::Vector3d> dst) {
    Eigen::Vector3d src_avg = Eigen::Vector3d::Zero();
    Eigen::Vector3d dst_avg = Eigen::Vector3d::Zero();
    for (size_t i = 0; i < src.size(); ++i) {
        src_avg += src[i];
        dst_avg += dst[i];
    }
    src_avg /= (double)src.size();
    dst_avg /= (double)dst.size();

    double src_d2 = 0;
    double dst_d2 = 0;
    for (size_t i = 0; i < src.size(); ++i) {
        src[i] -= src_avg;
        dst[i] -= dst_avg;
        src_d2 += src[i].squaredNorm();
        dst_d2 += dst[i].squaredNorm();
    }

    double S = sqrt(dst_d2 / src_d2);
    Eigen::Matrix3d R = Kabsch(src, dst);
    Eigen::Vector3d T = dst_avg - S*R*src_avg;

    return{ S, R, T };
}
