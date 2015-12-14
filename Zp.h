#pragma once
#include <cstdint>
#include <ostream>
#include <exception>

template<std::int64_t N>
class Zp {
	template<std::int64_t N>
	friend std::ostream& operator<<(std::ostream& o, const Zp<N>& z);
	typedef std::int64_t ZType;
public:
	Zp() : Zp(0, false) {}
	Zp(ZType v) : Zp(v, false) { modulo(); }

	Zp operator-() const {
		return Zp(N - v, false);
	}

	Zp& operator+=(const Zp& z) {
		v += (z.v - N);
		modulo();
		return *this;
	}

	Zp operator+(const Zp& z) const {
		return Zp(v + (z.v - N));
	}

	Zp& operator-=(const Zp& z) {
		v -= z.v;
		modulo();
		return *this;
	}

	Zp operator-(const Zp& z) const {
		return *this + (-z);
	}

	Zp& operator*=(const Zp& z) {
		v *= z.v;
		modulo();
		return *this;
	}

	Zp operator*(const Zp& z) const {
		return Zp(v*z.v);
	}

	Zp& operator/=(const Zp& z) {
		v *= z.inverse().v;
		modulo();
		return *this;
	}

	Zp operator/(const Zp& z) const {
		return *this * z.inverse();
	}

	Zp inverse() const {
		if (v == 0) {
			throw std::exception("Division by zero.");
		}
		ZType r0 = N, r1 = v, r2;
		ZType t0 = 0, t1 = 1, t2;
		ZType q;
		while (r1 > 0) {
			q = r0 / r1;
			r2 = r0 - q*r1;
			t2 = t0 - q*t1;
			r0 = r1;
			r1 = r2;
			t0 = t1;
			t1 = t2;
		}
		return Zp(t0 >= 0 ? t0 : t0 + N, false);
	}

	bool operator==(const Zp &z) const {
		return v == z.v;
	}

	bool operator!=(const Zp& z) const {
		return !(*this == z);
	}

	bool operator>(const Zp&z) const { return true; }
	bool operator>=(const Zp&z) const { return true; }
	bool operator<(const Zp&z) const { return true; }
	bool operator<=(const Zp&z) const { return true; }

private:
	ZType v;
	Zp(ZType v, bool) : v(v) {}

	void modulo() {
		v = v % N;
		v = (v >= 0 ? v : v + N);
	}
};

template<std::int64_t N>
std::ostream& operator<<(std::ostream& o, const Zp<N>& z) {
	o << z.v;
	return o;
}
