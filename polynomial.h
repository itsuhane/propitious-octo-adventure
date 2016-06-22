#pragma once

#include <vector>
#include <map>
#include <algorithm>

template <typename F>
class polynomial {
public:
    typedef unsigned int e_type;
    typedef F f_type;

private:
    typedef std::map<e_type, f_type> clist_type;

public:
    polynomial() {}

    polynomial(const f_type & v) {
        m_coeffs[0] = v;
    }

    bool is_zero() const {
        return m_coeffs.empty();
    }

    bool is_constant() const {
        return degree() == 0;
    }

    e_type degree() const {
        if (m_coeffs.size() > 0) {
            return m_coeffs.crbegin()->first;
        }
        else {
            return e_type(0);
        }
    }

    polynomial operator- () const {
        clist_type cc = m_coeffs;
        for (clist_type::iterator it = cc.begin(); it != cc.end(); ++it) {
            it->second = -it->second;
        }

        return polynomial(std::move(cc));
    }

    polynomial derivative() const {
        clist_type cc;
        for (clist_type::const_iterator it = m_coeffs.cbegin(); it != m_coeffs.cend(); ++it) {
            if (it->first > 0) {
                cc[it->first - 1] = f_type(it->first)*it->second;
            }
        }
        return polynomial(std::move(cc));
    }

    polynomial operator+ (const polynomial &p) const {
        clist_type cc = p.m_coeffs;
        for (clist_type::const_iterator it = m_coeffs.cbegin(); it != m_coeffs.cend(); ++it) {
            cc[it->first] += it->second;
        }
        return polynomial(std::move(cc));
    }

    polynomial operator- (const polynomial &p) const {
        return operator+(-p);
    }

    polynomial operator* (const polynomial &p) const {
        clist_type cc;
        for (clist_type::const_iterator ita = m_coeffs.cbegin(); ita != m_coeffs.cend(); ++ita) {
            for (clist_type::const_iterator itb = p.m_coeffs.cbegin(); itb != p.m_coeffs.cend(); ++itb) {
                cc[ita->first + itb->first] += ita->second*itb->second;
            }
        }
        return polynomial(std::move(cc));
    }

    polynomial operator% (polynomial &p) {
        clist_type cc = m_coeffs;
        const clist_type &cp = p.m_coeffs;
        while (!cc.empty() && cc.crbegin()->first >= cp.crbegin()->first) {
            e_type rp = cc.crbegin()->first - cp.crbegin()->first;
            f_type s = cc.crbegin()->second / cp.crbegin()->second;
            cc.erase(cc.crbegin()->first);
            for (clist_type::const_reverse_iterator it = (++cp.crbegin()); it != cp.crend(); ++it) {
                e_type tp = it->first + rp;
                cc[tp] -= s*it->second;
                if (cc.at(tp) == f_type(0)) {
                    cc.erase(tp);
                }
            }
        }
        return polynomial(std::move(cc));
    }

    bool has_term(e_type p) const {
        return m_coeffs.count(p) > 0;
    }

    const f_type & at(e_type p) const {
        return m_coeffs.at(p);
    }

    const f_type & get(e_type p) const {
        if (m_coeffs.count(p) > 0) {
            return m_coeffs.at(p);
        }
        else {
            static const f_type dummy(0);
            return dummy;
        }
    }

    void set(e_type p, const f_type &x) {
        if (x != f_type(0)) {
            m_coeffs[p] = x;
        }
        else {
            m_coeffs.erase(p);
        }
    }

    f_type operator() (const f_type &x) const {
        f_type r(0);
        e_type p(0);
        if (m_coeffs.size() > 0) {
            p = m_coeffs.crbegin()->first;
        }
        for (clist_type::const_reverse_iterator it = m_coeffs.crbegin(); it != m_coeffs.crend(); ++it) {
            for (; p > it->first; --p) {
                r *= x;
            }
            r += it->second;
        }
        return r;
    }

private:
    polynomial(clist_type&& m) : m_coeffs(std::move(m)) {
        clist_type::iterator it = m_coeffs.begin();
        while (it != m_coeffs.end()) {
            if (it->second == 0) {
                it = m_coeffs.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    clist_type m_coeffs;
};
