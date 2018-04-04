#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <Eigen/Eigen>
#include "Random.h"

// This is an implementation of Robert Bridson's "Fast Poisson Disk Sampling in Arbitrary Dimensions"
class PoissonSampler2D {
public:
    PoissonSampler2D(size_t width, size_t height, double radius, int k = 30, UniformNoise<double> &random_real = UniformNoise<double>(), UniformInteger<size_t> &random_integer = UniformInteger<size_t>()) : m_random_real(random_real), m_random_integer(random_integer) {
        m_radius = radius;
        m_radius_squared = radius*radius;
        m_width = width;
        m_height = height;
        m_grid_size = radius * M_SQRT1_2;
        m_grid_width = (size_t)(width / m_grid_size) + 1;
        m_grid_height = (size_t)(height / m_grid_size) + 1;
        m_grid.resize(m_grid_width*m_grid_height, nil);
        m_maxiter = k;
    }

    const std::vector<Eigen::Vector2d>& samples() const { return m_samples; }

    void clear() {
        m_samples.clear();
        m_grid.swap(std::vector<size_t>(m_grid_width*m_grid_height, nil));
        m_active_list.clear();
    }

    void generate() {
        clear();

        Eigen::Vector2d init_point{ m_random_real.next()*m_width, m_random_real.next()*m_height };
        m_grid[gi(init_point[0]) + gi(init_point[1])*m_grid_width] = 0;
        m_active_list.emplace_back(0);
        m_samples.emplace_back(std::move(init_point));

        while (m_active_list.size() > 0) {
            size_t current = m_random_integer.next(0, m_active_list.size() - 1);
            std::swap(m_active_list[current], m_active_list.back());
            const Eigen::Vector2d &p = m_samples[m_active_list.back()];
            bool deactivate = true;
            for (size_t iter = 0; iter < m_maxiter && deactivate; ++iter) {
                Eigen::Vector2d q = make_sample(p);

                if (q[0] < 0 || q[1] < 0 || q[0] >= m_width || q[1] >= m_height) {
                    continue;
                }

                int ix = (int)gi(q[0]);
                int iy = (int)gi(q[1]);

                bool alone = true;
                for (int y = std::max(iy - 2, 0); y <= std::min(iy + 2, (int)m_grid_height - 1) && alone; ++y) {
                    for (int x = std::max(ix - 2, 0); x <= std::min(ix + 2, (int)m_grid_width - 1) && alone; ++x) {
                        size_t neighbor = m_grid[x + y*m_grid_width];
                        if (neighbor != nil) {
                            if ((m_samples[neighbor] - q).squaredNorm() < m_radius_squared) {
                                alone = false;
                            }
                        }
                    }
                }

                if (alone) {
                    size_t new_id = m_samples.size();
                    m_grid[gi(q[0]) + gi(q[1])*m_grid_width] = new_id;
                    m_active_list.emplace_back(new_id);
                    m_samples.emplace_back(std::move(q));
                    deactivate = false;
                }
            }
            if (deactivate) {
                m_active_list.pop_back();
            }
        }
    }

private:
    Eigen::Vector2d make_sample(const Eigen::Vector2d &p) {
        double t = m_random_real.next()*M_PI*2.0;
        double r = (m_random_real.next()*0.75 + 0.25)*m_radius*2.0;
        return{ p[0] + r*cos(t), p[1] + r*sin(t) };
    }

    size_t gi(double v) const { return size_t(v / m_grid_size); }

    static const size_t nil = size_t(-1);

    double m_radius, m_radius_squared;
    size_t m_width, m_height;
    double m_grid_size;
    size_t m_grid_width, m_grid_height;
    size_t m_maxiter;

    std::vector<Eigen::Vector2d> m_samples;
    std::vector<size_t> m_grid;
    std::vector<size_t> m_active_list;

    UniformNoise<double> &m_random_real;
    UniformInteger<size_t> &m_random_integer;
};
