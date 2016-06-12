#pragma once
#include <vector>
#include <map>
#include <algorithm>

class polynomial {
    typedef std::map<unsigned int, double> __coeff_list_type;
public:
    polynomial() {}

    polynomial(const double & v) {
        m_coeffs[0] = v;
    }

    bool is_zero() {
        gc();
        return m_coeffs.empty();
    }

    polynomial operator- () const {
        __coeff_list_type cc = m_coeffs;
        for (__coeff_list_type::iterator it = cc.begin(); it != cc.end(); ++it) {
            it->second = -it->second;
        }

        return polynomial(std::move(cc));
    }

    polynomial derivative() const {
        __coeff_list_type cc;
        for (__coeff_list_type::const_iterator it = m_coeffs.cbegin(); it != m_coeffs.cend(); ++it) {
            if (it->first > 0) {
                cc[it->first - 1] = it->first*it->second;
            }
        }
        return polynomial(std::move(cc));
    }

    polynomial operator+ (const polynomial &p) const {
        __coeff_list_type cc = p.m_coeffs;
        for (__coeff_list_type::const_iterator it = m_coeffs.cbegin(); it != m_coeffs.cend(); ++it) {
            cc[it->first] += it->second;
        }
        return polynomial(std::move(cc));
    }

    polynomial operator- (const polynomial &p) const {
        return operator+(-p);
    }

    polynomial operator* (const polynomial &p) const {
        __coeff_list_type cc;
        for (__coeff_list_type::const_iterator ita = m_coeffs.cbegin(); ita != m_coeffs.cend(); ++ita) {
            for (__coeff_list_type::const_iterator itb = p.m_coeffs.cbegin(); itb != p.m_coeffs.cend(); ++itb) {
                cc[ita->first + itb->first] += ita->second*itb->second;
            }
        }
        return polynomial(std::move(cc));
    }

    polynomial operator% (polynomial &p) {
        gc(); p.gc();
        __coeff_list_type cc = m_coeffs;
        const __coeff_list_type &cp = p.m_coeffs;
        while (!cc.empty() && cc.crbegin()->first >= cp.crbegin()->first) {
            unsigned int rp = cc.crbegin()->first - cp.crbegin()->first;
            double s = cc.crbegin()->second / cp.crbegin()->second;
            cc.erase(cc.crbegin()->first);
            for (__coeff_list_type::const_reverse_iterator it = (++cp.crbegin()); it != cp.crend(); ++it) {
                unsigned int tp = it->first + rp;
                cc[tp] -= s*it->second;
                if (cc.at(tp) == 0) {
                    cc.erase(tp);
                }
            }
        }
        return polynomial(std::move(cc));
    }

    const double & at(unsigned int p) const {
        return m_coeffs.at(p);
    }

    double & at(unsigned int p) {
        return m_coeffs.at(p);
    }

    double & operator[] (unsigned int p) {
        return m_coeffs[p];
    }

    void gc() {
        std::map<unsigned int, double>::iterator it = m_coeffs.begin();
        while (it != m_coeffs.end()) {
            if (it->second == 0) {
                it = m_coeffs.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    double operator() (const double &x) const {
        double r = 0;
        unsigned int p = 0;
        if (m_coeffs.size() > 0) {
            p = m_coeffs.crbegin()->first;
        }
        for (__coeff_list_type::const_reverse_iterator it = m_coeffs.crbegin(); it != m_coeffs.crend(); ++it) {
            for (; p > it->first; --p) {
                r *= x;
            }
            r += it->second;
        }
        return r;
    }

private:
    polynomial(__coeff_list_type&& m) : m_coeffs(std::move(m)) {}

    __coeff_list_type m_coeffs;
};

class strum_chain {
public:
    strum_chain(const polynomial &p) {
        m_chain.push_back(p);
        m_chain.push_back(p.derivative());
        while (true) {
            size_t i = m_chain.size() - 2;
            polynomial r = m_chain[i] % m_chain[i + 1];
            if (r.is_zero()) {
                break;
            }
            else {
                m_chain.push_back(-r);
            }
        }
    }

    size_t sign_changes(double v) const {
        size_t count = 0;
        bool current_sign;
        double current_value;
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

    size_t root_in_range(double a, double b) const {
        return sign_changes(a) - sign_changes(b);
    }

private:
    std::vector<polynomial> m_chain;
};

