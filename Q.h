#pragma once
#include <cstdint>
#include <ostream>

class Q {
	friend std::ostream& operator<<(std::ostream& o, const Q& q);
	typedef std::int64_t ZType;
public:
	Q() : Q(0, 1, false) {}
	Q(ZType v) : Q(v, 1, false) {}
	Q(ZType up, ZType down) : Q(up, down, false) { simplify(); }

	Q operator-() const {
		return Q(-up, down, false);
	}

	Q operator+(const Q& q) const {
		ZType n = gcd(down, q.down);
		ZType s1 = q.down / n;
		ZType s2 = down / n;
		return Q(up*s1 + q.up*s2, s1*down);
	}

	Q operator-(const Q& q) const {
		return *this + (-q);
	}

	Q operator*(const Q& q) const {
		ZType s1 = gcd(std::abs(up), q.down);
		ZType s2 = gcd(std::abs(q.up), down);
		return Q((up / s1)*(q.up / s2), (down / s2)*(q.down / s1), false);
	}

	Q operator/(const Q& q) const {
		return *this * q.invert();
	}

	Q invert() const {
		return up > 0 ? Q(down, up, false) : Q(-down, -up, false);
	}

	bool operator==(const Q& q) const {
		return (up == q.up) && (down == q.down);
	}

	bool operator!=(const Q& q) const {
		return !(*this == q);
	}

	bool operator<(const Q&q) const {
		ZType nup = gcd(std::abs(up), std::abs(q.up));
		ZType ndown = gcd(down, q.down);
		return (q.down/ndown)*(up/nup) < (down/ndown)*(q.up/nup);
	}

	bool operator>(const Q& q) const {
		return q < *this;
	}

	bool operator<=(const Q& q) const {
		return !(q < *this);
	}

	bool operator>=(const Q& q) const {
		return !(*this < q);
	}

private:
	ZType up;
	ZType down;

	Q(ZType up, ZType down, bool) : up(up), down(down) {}

	ZType gcd(ZType a, ZType b) const {
		ZType c;
		if (a < b) std::swap(a, b);
		while (b > 0) {
			c = a % b;
			a = b;
			b = c;
		}
		return a;
	}

	void simplify() {
		if (down == 0) {
			throw std::exception("Division by zero.");
		}

		if (down < 0) {
			up = -up;
			down = -down;
		}

		ZType n = gcd(std::abs(up), down);
		up /= n;
		down /= n;
	}
};

inline std::ostream& operator<<(std::ostream& o, const Q& q) {
	o << '(' << q.up << '/' << q.down << ')';
	return o;
}
