#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include <ostream>
#include <sstream>

namespace symbolic {
	class Variable {
		friend class Monomial;
		template<typename F> friend class Polynomial;
		friend struct std::hash<Variable>;
		friend std::ostream& operator<<(std::ostream& o, const Variable& v);
	public:
		Variable() : id(new_id()) {}
		Variable(const std::string& name) : id(new_id(name)) {}

		bool operator==(const Variable& v) const {
			return id == v.id;
		}

		bool operator!=(const Variable& v) const {
			return !(*this == v);
		}

		void set_name(const std::string &name) const {
			id_name_map()[id] = name;
		}

		std::string get_name() const {
			return id_name_map()[id];
		}

		std::string to_string() const {
			return get_name();
		}
	private:
		static std::unordered_map<size_t, std::string> &id_name_map() {
			static std::unordered_map<size_t, std::string> the_map;
			return the_map;
		}

		static size_t new_id() {
			size_t id = id_name_map().size();
			std::stringstream ss;
			ss << "{nameless-variable-" << id << '}';
			id_name_map()[id] = ss.str();
			return id;
		}

		static size_t new_id(const std::string &name) {
			size_t id = id_name_map().size();
			id_name_map()[id] = name;
			return id;
		}

		const size_t id;
	};

	std::ostream& operator<<(std::ostream& o, const Variable& v) {
		o << v.to_string();
		return o;
	}
}

namespace std {
	template<>
	struct hash<symbolic::Variable> {
		typedef symbolic::Variable argument_type;
		typedef size_t return_type;
		return_type operator() (argument_type const &v) const {
			return v.id;
		}
	};
}

namespace symbolic {
	class Monomial {
		template<typename F> friend class Polynomial;
		friend struct std::hash<Monomial>;
		friend std::ostream& operator<<(std::ostream& o, const Monomial& m);
	public:
		Monomial() : Monomial(std::unordered_map<Variable, size_t>()) {}
		Monomial(const Variable &v) : Monomial(v, 1) {}
		Monomial(const Variable &v, size_t order_v) : Monomial(std::unordered_map<Variable, size_t>({ {v, order_v} })) {}
		Monomial(const std::unordered_map<Variable, size_t> &variables) : Monomial(clean_up(variables)) {}

		Monomial operator*(const Monomial& m) const {
			std::map<Variable, size_t, InternalVariableOrder> result_vt = vt;
			for (auto & t : m.vt) {
				result_vt[t.first] += t.second;
			}
			return Monomial(result_vt);
		}

		bool operator== (const Monomial& m) const {
			return hash == m.hash;
		}

		bool operator!= (const Monomial& m) const {
			return !(*this == m);
		}

		bool is_constant() const {
			return vt.size() == 0;
		}

		std::string to_string() const {
			std::stringstream ss;
			if (is_constant()) {
				ss << '1';
			}
			else {
				for (auto &t : vt) {
					ss << t.first;
					if (t.second > 1) {
						ss << '^' << t.second;
					}
				}
			}
			return ss.str();
		}

	private:
		struct InternalVariableOrder {
			bool operator() (const Variable& a, const Variable& b) const {
				return a.id < b.id;
			}
		};

		Monomial(const std::map<Variable, size_t, InternalVariableOrder> &vt) : vt(vt), hash(hash_fcn(vt)), order(count_order(vt)) {}

		static std::map<Variable, size_t, InternalVariableOrder> clean_up(const std::unordered_map<Variable, size_t>& vt) {
			std::map<Variable, size_t, InternalVariableOrder> result_vt;
			for (auto &t : vt) {
				if (t.second != 0) {
					result_vt[t.first] = t.second;
				}
			}
			return result_vt;
		}

		static std::size_t hash_fcn(const std::map<Variable, size_t, InternalVariableOrder>& vt) {
			size_t seed = 0xdeadbeef;
			for (auto& i : vt) {
				seed ^= i.first.id + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= i.second + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			}
			return seed;
		}

		static size_t count_order(const std::map<Variable, size_t, InternalVariableOrder>& vt) {
			size_t o = 0;
			for (auto& i : vt) {
				o += i.second;
			}
			return o;
		}

		const std::map<Variable, size_t, InternalVariableOrder> vt;
		const size_t hash;
		const size_t order;
	};

	std::ostream& operator<<(std::ostream& o, const Monomial& m) {
		o << m.to_string();
		return o;
	}
}

namespace std {
	template<>
	struct hash<symbolic::Monomial> {
		typedef symbolic::Monomial argument_type;
		typedef size_t return_type;
		return_type operator() (argument_type const &v) const {
			return v.hash;
		}
	};
}

namespace symbolic {
	template<typename F>
	class Polynomial {
		template<typename F> friend std::ostream& operator<<(std::ostream& o, const Polynomial<F>& m);
	public:
		typedef F FieldType;

		Polynomial() : Polynomial(std::unordered_map<Monomial, FieldType>()) {}
		Polynomial(const FieldType& c) : Polynomial(Monomial(), c) {}
		Polynomial(const Variable& v) : Polynomial(Monomial(v)) {}
		Polynomial(const Monomial& m) : Polynomial(m, 1) {}
		Polynomial(const Monomial& m, const FieldType& coeff_m) : Polynomial(std::unordered_map<Monomial, FieldType>({ {m, coeff_m} })) {}
		Polynomial(const std::unordered_map<Monomial, FieldType>& mt) {
			for (auto & t : mt) {
				if (t.second != FieldType(0)) {
					this->mt[t.first] = t.second;
				}
			}
		}

		Polynomial operator-() const {
			Polynomial result(mt);
			for (auto& t : result.mt) {
				t.second = -t.second;
			}
			return result;
		}

		Polynomial& operator+=(const Polynomial& p) {
			for (auto &t : p.mt) {
				mt[t.first] += t.second;
			}
			clean_up();
			return *this;
		}

		Polynomial operator+(const Polynomial& p) const {
			Polynomial result(*this);
			return result+=p;
		}

		Polynomial& operator-=(const Polynomial& p) {
			for (auto &t : p.mt) {
				mt[t.first] -= t.second;
			}
			clean_up();
			return *this;
		}

		Polynomial operator-(const Polynomial& p) const {
			Polynomial result(*this);
			return result -= p;
		}

		Polynomial operator*(const Polynomial& p) const {
			Polynomial result;
			for (auto &a : mt) {
				for (auto &b : p.mt) {
					result.mt[a.first*b.first] += a.second*b.second;
				}
			}
			result.clean_up();
			return result;
		}

		Polynomial& operator*=(const Polynomial& p) {
			*this = *this*p;
			return *this;
		}

		bool operator==(const Polynomial& p) const {
			if (mt.size() != p.mt.size()) return false;
			for (auto & t : mt) {
				if (p.mt.count(t.first) == 0) return false;
				if (p.mt.at(t.first) != t.second) return false;
			}
			return true;
		}

		bool operator!=(const Polynomial& p) const {
			return !(*this == p);
		}

		std::string to_string() const {
			std::stringstream ss;
			if (mt.size() == 0) {
				ss << '0';
			}
			else {
				bool is_first = true;
				for (auto & t : mt) {
					if (is_first) {
						is_first = false;
					}
					else {
						if (t.second > 0) {
							ss << '+';
						}
					}
					if (t.first.is_constant()) {
						ss << t.second;
					}
					else {
						if (t.second != 1) {
							ss << t.second;
						}
						ss << t.first;
					}
				}
			}
			return ss.str();
		}

	private:
		struct InternalMonomialOrder {
			bool operator() (const Monomial &a, const Monomial &b) const {
				if (a.order != b.order) return a.order>b.order;
				auto at = a.vt.begin(), bt = b.vt.begin();
				for (; at != a.vt.end() && bt != b.vt.end(); at++, bt++) {
					if (at->first != bt->first) return at->first.id < bt->first.id;
					if (at->second != bt->second) return at->second > bt->second;
				}
				return bt != b.vt.end();
			}
		};

		Polynomial(const std::map<Monomial, FieldType, InternalMonomialOrder>& mt) : mt(mt) {}

		void clean_up() {
			for (auto it = mt.begin(); it != mt.end();) {
				if (it->second==0) {
					it = mt.erase(it);
				}
				else {
					++it;
				}
			}
		}

		std::map<Monomial, FieldType, InternalMonomialOrder> mt;
	};

	template<typename F>
	std::ostream& operator<<(std::ostream& o, const Polynomial<F>& m) {
		o << m.to_string();
		return o;
	}
}
