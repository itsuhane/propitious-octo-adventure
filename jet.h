#pragma once

/*
    jet.h - dual number for automatic differentiation
    this is inspired by the Jet type in ceres-solver.

    this is a toy implementation, IT IS SLOW!
*/

#include <cmath>
#include <map>

template <typename T>
class jet {
public:
    typedef size_t guid_type;
    typedef T value_type;

    jet() : jet(0) {}
    jet(const value_type &x) : jet(x, 0) {}

    jet(const jet& d) = default;
    jet(jet&& d) : x(d.x), u(std::move(d.u)) {}

    static jet variable(const value_type &x) {
        return jet(x, new_id());
    }

    value_type value() const {
        return x;
    }

    value_type partial(const jet &d) const {
        if (u.count(d.id) > 0) {
            return u.at(d.id);
        }
        else {
            return 0;
        }
    }

    jet operator-() const {
        jet result = *this;
        result.x = -x;
        for (auto &p : result.u) {
            p.second = -p.second;
        }
        return result;
    }

    jet &operator+=(const jet &d) {
        id = 0;
        x += d.x;
        for (const auto &p : d.u) {
            u[p.first] += p.second;
        }
        return *this;
    }

    jet &operator-=(const jet &d) {
        id = 0;
        x -= d.x;
        for (const auto &p : d.u) {
            u[p.first] -= p.second;
        }
        return *this;
    }

    jet &operator*=(const jet &d) {
        id = 0;
        for (auto &p : u) {
            p.second *= d.x;
        }
        for (const auto &p : d.u) {
            u[p.first] += p.second*x;
        }
        x *= d.x;
        return *this;
    }

    jet &operator/=(const jet &d) {
        id = 0;
        x /= d.x;
        for (const auto &p : d.u) {
            u[p.first] -= p.second*x;
        }
        for (auto &p : u) {
            p.second /= d.x;
        }
        return *this;
    }

    jet operator+(const jet &d) const {
        jet result = *this;
        result += d;
        return result;
    }

    jet operator-(const jet &d) const {
        jet result = *this;
        result -= d;
        return result;
    }

    jet operator*(const jet &d) const {
        jet result = *this;
        result *= d;
        return result;
    }

    jet operator/(const jet &d) const {
        jet result = *this;
        result /= d;
        return result;
    }

    template<typename T> friend jet<T> operator+(const T& x, const jet<T> &d);
    template<typename T> friend jet<T> operator-(const T& x, const jet<T> &d);
    template<typename T> friend jet<T> operator*(const T& x, const jet<T> &d);
    template<typename T> friend jet<T> operator/(const T& x, const jet<T> &d);
    template<typename T> friend jet<T> abs(const jet<T> &d);
    template<typename T> friend jet<T> sin(const jet<T> &d);
    template<typename T> friend jet<T> cos(const jet<T> &d);
    template<typename T> friend jet<T> log(const jet<T> &d);

private:
    jet(const value_type &x, const guid_type &id) : x(x), id(id) {
        if (id != 0) {
            u[id] = 1;
        }
    }

    guid_type id;
    value_type x;
    std::map<guid_type, value_type> u;

    static guid_type new_id() {
        static guid_type current_id = 0;
        current_id++;
        //if (current_id == 0) {
        // TODO: id exhausted, throw exception
        //}
        return current_id;
    }
};

template<typename T>
jet<T> operator+(const T& x, const jet<T> &d) {
    return jet<T>(x) + d;
}

template<typename T>
jet<T> operator-(const T& x, const jet<T> &d) {
    return jet<T>(x) - d;
}

template<typename T>
jet<T> operator*(const T& x, const jet<T> &d) {
    return jet<T>(x) * d;
}

template<typename T>
jet<T> operator/(const T& x, const jet<T> &d) {
    return jet<T>(x) / d;
}

template<typename T>
jet<T> abs(const jet<T> &d) {
    return (d.x < 0) ? (-d) : d;
}

template<typename T>
jet<T> sin(const jet<T> &d) {
    jet<T> result = d;
    result.x = sin(d.x);
    T c = cos(d.x);
    for (auto &p : result.u) {
        p.second *= c;
    }
    return result;
}

template<typename T>
jet<T> cos(const jet<T> &d) {
    jet<T> result = d;
    result.x = cos(d.x);
    T s = -sin(d.x);
    for (auto &p : result.u) {
        p.second *= s;
    }
    return result;
}

template<typename T>
jet<T> log(const jet<T> &d) {
    jet<T> result = d;
    result.x = log(d.x);
    for (auto &p : result.u) {
        p.second /= d.x;
    }
    return result;
}
