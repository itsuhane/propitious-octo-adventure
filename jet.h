#pragma once

/*
jet.h - dual number for automatic differentiation
this is inspired by the Jet type in ceres-solver.

this is a toy implementation, IT IS SLOW!
*/

#include <cmath>
#include <map>
#include <ostream>

template <typename T>
class jet {
    template<typename T> friend jet<T> operator+(const T& x, const jet<T>& d);
    template<typename T> friend jet<T> operator-(const T& x, const jet<T>& d);
    template<typename T> friend jet<T> operator*(const T& x, const jet<T>& d);
    template<typename T> friend jet<T> operator/(const T& x, const jet<T>& d);

    template<typename CharT, typename Traits, typename T>
    friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& s, const jet<T>& d);

public:
    typedef T value_type;

    jet() : jet(0) {}
    jet(const value_type& x) : id(0), x(x) {}

    jet(const jet& d) = default;
    jet(jet&& d) : id(d.id), x(d.x), u(std::move(d.u)) {}

    void make_variable() {
        u[id = new_id()] = 1;
    }

    jet as_value() const {
        jet result = *this;
        result.id = 0;
        return result;
    }

    const value_type& value() const {
        return x;
    }

    value_type& value() {
        return x;
    }

    value_type partial(const jet& d) const {
        if (u.count(d.id) > 0) {
            return u.at(d.id);
        } else {
            return 0;
        }
    }

    jet& operator=(const jet& d) = default;

    jet& operator=(const value_type& v) {
        x = v;
        return *this;
    }

    jet operator-() const {
        jet result = *this;
        result.x = -x;
        for (auto& p : result.u) {
            p.second = -p.second;
        }
        return result;
    }

    jet& operator+=(const jet& d) {
        id = 0;
        x += d.x;
        for (const auto& p : d.u) {
            u[p.first] += p.second;
        }
        return *this;
    }

    jet& operator-=(const jet& d) {
        id = 0;
        x -= d.x;
        for (const auto& p : d.u) {
            u[p.first] -= p.second;
        }
        return *this;
    }

    jet& operator*=(const jet& d) {
        id = 0;
        for (auto& p : u) {
            p.second *= d.x;
        }
        for (const auto& p : d.u) {
            u[p.first] += p.second * x;
        }
        x *= d.x;
        return *this;
    }

    jet& operator/=(const jet& d) {
        id = 0;
        x /= d.x;
        for (const auto& p : d.u) {
            u[p.first] -= p.second * x;
        }
        for (auto& p : u) {
            p.second /= d.x;
        }
        return *this;
    }

    jet operator+(const jet& d) const {
        jet result = *this;
        result += d;
        return result;
    }

    jet operator-(const jet& d) const {
        jet result = *this;
        result -= d;
        return result;
    }

    jet operator*(const jet& d) const {
        jet result = *this;
        result *= d;
        return result;
    }

    jet operator/(const jet& d) const {
        jet result = *this;
        result /= d;
        return result;
    }

    bool operator<(const jet& d) const {
        return x < d.x;
    }

    bool operator>(const jet& d) const {
        return x > d.x;
    }

    bool operator<=(const jet& d) const {
        return x <= d.x;
    }

    bool operator>=(const jet& d) const {
        return x >= d.x;
    }

    bool operator==(const jet& d) const {
        return x == d.x;
    }

    bool operator!=(const jet& d) const {
        return x != d.x;
    }

    void push_forward(const value_type& s) {
        for (auto& p : u) {
            p.second *= s;
        }
    }

private:
    typedef size_t uvid_type;
    uvid_type id;

    static uvid_type new_id() {
        static uvid_type current_id = 0;
        current_id++;
        //if (current_id == 0) {
        // TODO: id exhausted, throw exception
        //}
        return current_id;
    }

    value_type x;
    std::map<uvid_type, value_type> u;
};

template<typename T>
jet<T> operator+(const T& x, const jet<T>& d) {
    return jet<T>(x) + d;
}

template<typename T>
jet<T> operator-(const T& x, const jet<T>& d) {
    return jet<T>(x) - d;
}

template<typename T>
jet<T> operator*(const T& x, const jet<T>& d) {
    return jet<T>(x) * d;
}

template<typename T>
jet<T> operator/(const T& x, const jet<T>& d) {
    return jet<T>(x) / d;
}

template<typename T>
jet<T> abs(const jet<T>& d) {
    return (d.value() < 0) ? (-d) : d;
}

template<typename T>
jet<T> sin(const jet<T>& d) {
    jet<T> result = d;
    result.value() = sin(d.value());
    result.push_forward(cos(d.value()));
    return result;
}

template<typename T>
jet<T> cos(const jet<T>& d) {
    jet<T> result = d;
    result.value() = cos(d.value());
    result.push_forward(-sin(d.value()));
    return result;
}

template<typename T>
jet<T> tan(const jet<T>& d) {
    jet<T> result = d;
    result.value() = tan(d.value());
    double sec = 1 / cos(d.value());
    result.push_forward(sec * sec);
    return result;
}

template<typename T>
jet<T> sinc(const jet<T>& d) {
    static const T root1_eps = std::numeric_limits<T>::epsilon();
    static const T root2_eps = sqrt(root1_eps);
    static const T root3_eps = cbrt(root1_eps);
    static const T root4_eps = sqrt(root2_eps);
    jet<T> result = d;

    const T& x = d.value();
    T& rx = result.value();

    T ax = abs(x);
    T sx = sin(x);
    T x2 = x * x;
    T dx;

    if (ax > root4_eps) {
        rx = sx / x;
        dx = (x * cos(x) - sx) / x2;
    } else {
        rx = 1;
        dx = -x / 3;
        if (ax > root1_eps) {
            rx -= x2 / 6;
            if (ax > root2_eps) {
                rx += (x2 * x2) / 120;
                dx += (x * x2) / 30;
            }
        }
    }

    result.push_forward(dx);

    return result;
}

template<typename T>
jet<T> exp(const jet<T>& d) {
    jet<T> result = d;
    result.value() = exp(d.value());
    result.push_forward(result.value());
    return result;
}

template<typename T>
jet<T> sqrt(const jet<T>& d) {
    jet<T> result = d;
    result.value() = sqrt(d.value());
    result.push_forward(T(0.5) / result.value());
    return result;
}

template<typename T>
jet<T> log(const jet<T>& d) {
    jet<T> result = d;
    result.value() = log(d.value());
    result.push_forward(1 / d.value());
    return result;
}

template<typename CharT, typename Traits, typename T>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& s, const jet<T>& d) {
    return (s << d.value());
}

template<typename T>
void make_variable(jet<T>& x) {
    x.make_variable();
}


template<typename T>
T partial(const jet<T>& y, const jet<T>& x) {
    return y.partial(x);
}

