#pragma once

#include <vector>
#include "polynomial.h"

template <typename F>
class sturm_chain {
    typedef polynomial<F> poly_type;
    typedef F f_type;

public:
    sturm_chain(const poly_type &p) {
        m_chain.push_back(p);
        m_chain.push_back(p.derivative());
        while (true) {
            size_t i = m_chain.size() - 2;
            poly_type r = m_chain[i] % m_chain[i + 1];
            if (r.is_zero()) {
                break;
            }
            else {
                m_chain.push_back(-r);
            }
        }
    }

    size_t sign_changes(const f_type &v) const {
        size_t count = 0;
        bool current_sign;
        f_type current_value;
        size_t i = 0;
        while (i < m_chain.size() && ((current_value = m_chain[i](v)) == 0)) {
            ++i;
        }
        current_sign = (current_value > 0);
        for (++i; i < m_chain.size(); ++i) {
            current_value = m_chain[i](v);
            if (current_value == 0) {
                continue;
            }
            else {
                bool new_sign = (current_value > 0);
                if (current_sign != new_sign) {
                    current_sign = new_sign;
                    count++;
                }
            }
        }
        return count;
    }

    size_t root_in_range(const f_type &a, const f_type &b) const {
        return sign_changes(a) - sign_changes(b);
    }

private:
    std::vector<poly_type> m_chain;
};
