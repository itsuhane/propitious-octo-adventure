#pragma once

#include <Eigen/Eigen>

inline Eigen::Matrix3d skew_matrix(const Eigen::Vector3d &u) {
    return (Eigen::Matrix3d() <<
             0, -u.z(),  u.y(),
         u.z(),      0, -u.x(),
        -u.y(),  u.x(),      0
        ).finished();
}

inline Eigen::Matrix4d left_mult_matrix(const Eigen::Quaterniond &q) {
    return (Eigen::Matrix4d() <<
         q.w(), -q.z(),  q.y(), q.x(),
         q.z(),  q.w(), -q.x(), q.y(),
        -q.y(),  q.x(),  q.w(), q.z(),
        -q.x(), -q.y(), -q.z(), q.w()
        ).finished();
}

inline Eigen::Matrix4d right_mult_matrix(const Eigen::Quaterniond &q) {
    return (Eigen::Matrix4d() <<
         q.w(),  q.z(), -q.y(), q.x(),
        -q.z(),  q.w(),  q.x(), q.y(),
         q.y(), -q.x(),  q.w(), q.z(),
        -q.x(), -q.y(), -q.z(), q.w()
        ).finished();
}

inline Eigen::Matrix4d omega_matrix(const Eigen::Vector3d &w) {
    return (Eigen::Matrix4d() <<
             0, -w.z(),  w.y(), w.x(),
         w.z(),      0, -w.x(), w.y(),
        -w.y(),  w.x(),      0, w.z(),
        -w.x(), -w.y(), -w.z(),     0
        ).finished();
}
