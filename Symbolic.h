#pragma once

#include <map>
#include <unordered_map>
#include <string>
#include <sstream>
#include <ostream>

namespace symbolic {
	class Variable {
	public:
		Variable(std::string const &name = std::string()) : m_id(new_id()) {
			set_name(name);
		}

		bool operator==(Variable const&v) const {
			return m_id == v.m_id;
		}

		bool operator!=(Variable const&v) const {
			return !(*this == v);
		}

		void set_name(std::string const &name) const {
			id_name_map()[m_id] = name;
		}

		std::string get_name() const {
			return id_name_map()[m_id];
		}

		std::string to_string() const {
			return get_name();
		}

	private:
		size_t const m_id;

	private:
		static std::unordered_map<size_t, std::string> &id_name_map() {
			static std::unordered_map<size_t, std::string> the_map;
			return the_map;
		}

		static size_t new_id() {
			size_t id{ id_name_map().size() };
			id_name_map().emplace(id, std::string());
			return id;
		}

		friend struct std::hash < Variable >;

	public:
		struct LexicographicalOrder {
			bool operator() (Variable const &a, Variable const &b) const {
				return a.m_id < b.m_id;
			}
		};
	};

	std::ostream &operator<<(std::ostream &o, Variable const& v) {
		return o << v.to_string();
	}
}

namespace std {
	template<>
	struct hash < ::symbolic::Variable > {
		typedef ::symbolic::Variable argument_type;
		typedef size_t result_type;
		result_type operator()(argument_type const &v) const {
			return std::hash<size_t>()(v.m_id);
		}
	};
}

namespace symbolic {
	class Term {
	public:
		Term() : Term(MapType{ {} }) {}

		bool operator==(Term const & t) const {
			return m_hash == t.m_hash;
		}

		bool operator!=(Term const &t) const {
			return !(*this == t);
		}

		size_t degree() const {
			return m_degree;
		}

		std::string to_string() const {
			std::stringstream ss;
			if (m_degree == 0) {
				ss << '1';
			}
			else {
				for (auto & f : m_factors) {
					ss << f.first;
					if (f.second > 1) {
						ss << '^' << f.second;
					}
				}
			}
			return ss.str();
		}

		static Term constantTerm() {
			static Term t;
			return t;
		}

		static Term fromVariable(Variable const &v) {
			return Term({ { v, 1 } });
		}

	private:
		typedef std::map<Variable, size_t, Variable::LexicographicalOrder> MapType;
		const size_t m_hash;
		const size_t m_degree;
		const MapType m_factors;

	private:
		Term(MapType const &f) : m_hash(calc_hash(f)), m_degree(calc_degree(f)), m_factors(f) {}

	private:
		static size_t calc_degree(MapType const &m) {
			size_t degree = 0;
			for (auto & v : m) {
				degree += v.second;
			}
			return degree;
		}

		static size_t calc_hash(MapType const &m) {
			std::hash<Variable> h;
			size_t seed = 0xdeadbeef;
			for (auto& i : m) {
				seed ^= h(i.first) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= i.second + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			}
			return seed;
		}

		friend Term lcm(Term const &a, Term const &b);
		friend struct std::hash < Term >;
		template <typename A, typename B>
		friend struct Operator;
	public:
		struct LexicographicalOrder {

			bool operator() (Term const &a, Term const &b) const {
				Variable::LexicographicalOrder less;
				auto at = a.m_factors.begin();
				auto bt = b.m_factors.begin();
				for (; at != a.m_factors.end() && bt != b.m_factors.end(); ++at, ++bt) {
					if (at->first != bt->first) return less(at->first, bt->first);
					if (at->second != bt->second) return at->second > bt->second;
				}
				return at != a.m_factors.end();
			}
		};

		struct ReverseLexicographicalOrder {
			bool operator() (Term const &a, Term const &b) const {
				Variable::LexicographicalOrder less;
				auto at = a.m_factors.begin();
				auto bt = b.m_factors.begin();
				for (; at != a.m_factors.end() && bt != b.m_factors.end(); ++at, ++bt) {
					if (at->first != bt->first) return less(bt->first, at->first);
					if (at->second != bt->second) return at->second > bt->second;
				}
				return at != a.m_factors.end();
			}
		};

		struct GradedLexicographicalOrder {
			bool operator() (Term const &a, Term const &b) const {
				if (a.m_degree != b.m_degree) return a.m_degree > b.m_degree;
				return LexicographicalOrder()(a, b);
			}
		};

		struct GradedReverseLexicographicalOrder {
			bool operator() (Term const &a, Term const &b) const {
				if (a.m_degree != b.m_degree) return a.m_degree > b.m_degree;
				return ReverseLexicographicalOrder()(b, a);
			}
		};
	};

	std::ostream &operator<<(std::ostream &o, Term const& v) {
		return o << v.to_string();
	}

	Term lcm(Term const &a, Term const &b) {
		Term::MapType f{ a.m_factors };
		for (auto & t : b.m_factors) {
			f[t.first] = std::max(f[t.first], t.second);
		}
		return Term(f);
	}
}

namespace std {
	template<>
	struct hash < ::symbolic::Term > {
		typedef ::symbolic::Term argument_type;
		typedef size_t result_type;
		result_type operator()(argument_type const &t) const {
			return std::hash<size_t>()(t.m_hash);
		}
	};
}

namespace symbolic {
	typedef long UnknownField;

	template < typename F, typename Ord = Term::GradedLexicographicalOrder >
	class Polynomial {
	public:
		typedef F FieldType;
		typedef Ord Order;

		Polynomial() = default;

		Polynomial operator-() const {
			Polynomial q{ *this };
			for (auto &m : q.m_monomials) {
				m.second = -m.second;
			}
			return q;
		}

		Polynomial& operator+=(Polynomial const &p) {
			for (auto &m : p.m_monomials) {
				m_monomials[m.first] += m.second;
			}
			clean_up();
			return *this;
		}

		Polynomial& operator-=(Polynomial const &p) {
			for (auto &m : p.m_monomials) {
				m_monomials[m.first] -= m.second;
			}
			clean_up();
			return *this;
		}

		Polynomial& operator*=(Polynomial const &p) {
			MapType monomials;
			monomials.swap(m_monomials);
			for (auto &a : monomials) {
				for (auto& b : p.m_monomials) {
					m_monomials[a.first*b.first] += a.second*b.second;
				}
			}
			clean_up();
			return *this;
		}

		std::string to_string() const {
			std::stringstream ss;
			if (m_monomials.size() == 0) {
				ss << '0';
			}
			else {
				bool first = true;
				for (auto &m : m_monomials) {
					if (first) {
						first = false;
					}
					else {
						if (m.second > FieldType(0)) {
							ss << '+';
						}
					}
					if (m.first.degree() == 0) {
						ss << m.second;
					}
					else {
						ss << m.second << m.first;
					}
				}
			}
			return ss.str();
		}

		static Polynomial fromConstant(FieldType const &c) {
			return Polynomial({ { Term::constantTerm(), c } });
		}

		static Polynomial fromTerm(Term const &t) {
			return Polynomial({ { t, 1 } });
		}

		static Polynomial fromUnknownField(Polynomial < UnknownField, Ord > const &p) {
			Polynomial result;
			for (auto &m : p.m_monomials) {
				result.m_monomials[m.first] = FieldType(m.second);
			}
			return result;
		}
	private:
		typedef typename std::map < Term, FieldType, Order > MapType;
		MapType m_monomials;

		Polynomial(MapType const &monomials) : m_monomials(monomials) {}

		void clean_up() {
			for (auto it = m_monomials.begin(); it != m_monomials.end();) {
				if (it->second == FieldType(0)) {
					it = m_monomials.erase(it);
				}
				else {
					++it;
				}
			}
		}

		template <typename F, typename Ord>
		friend class Polynomial;
	};
}

namespace symbolic {
	template < typename S >
	struct is_symbolic {
		static const bool value = false;
	};

	template < >
	struct is_symbolic < Variable > {
		static const bool value = true;
	};
	template < >
	struct is_symbolic < Term > {
		static const bool value = true;
	};
	template < typename F >
	struct is_symbolic < Polynomial<F> > {
		static const bool value = true;
	};

	template < typename A, typename B >
	struct Operator {};

	template < typename F >
	struct Operator < Polynomial<F>, Polynomial<F> > {
		typedef typename Polynomial<F> sum_type;
		typedef typename Polynomial<F> prod_type;

		static sum_type add(Polynomial<F> const &a, Polynomial<F> const &b) {
			Polynomial<F> c{ a };
			return c += b;
		}

		static sum_type sub(Polynomial<F> const &a, Polynomial<F> const &b) {
			Polynomial<F> c{ a };
			return c -= b;
		}

		static prod_type mult(Polynomial<F> const &a, Polynomial<F> const &b) {
			Polynomial<F> c{ a };
			return c *= b;
		}
	};

	template < typename F >
	struct Operator < Polynomial<F>, F > : public Operator < Polynomial<F>, Polynomial<F> >{
		static sum_type add(Polynomial<F> const &a, F const &b) {
			return Operator<Polynomial<F>, Polynomial<F>>::add(a, Polynomial<F>::fromConstant(b));
		}
		static sum_type sub(Polynomial<F> const &a, F const &b) {
			return Operator<Polynomial<F>, Polynomial<F>>::sub(a, Polynomial<F>::fromConstant(b));
		}
		static prod_type mult(Polynomial<F> const &a, F const &b) {
			return Operator<Polynomial<F>, Polynomial<F>>::mult(a, Polynomial<F>::fromConstant(b));
		}
	};

	template < typename F >
	struct Operator < F, Polynomial<F> > : public Operator < Polynomial<F>, Polynomial<F> >{
		static sum_type add(F const &a, Polynomial<F> const &b) {
			return Operator<Polynomial<F>, Polynomial<F>>::add(Polynomial<F>::fromConstant(a), b);
		}
		static sum_type sub(F const &a, Polynomial<F> const &b) {
			return Operator<Polynomial<F>, Polynomial<F>>::sub(Polynomial<F>::fromConstant(a), b);
		}
		static sum_type mult(F const &a, Polynomial<F> const &b) {
			return Operator<Polynomial<F>, Polynomial<F>>::mult(Polynomial<F>::fromConstant(a), b);
		}
	};

	template < typename F >
	struct Operator < Polynomial<F>, Term > : public Operator < Polynomial<F>, Polynomial<F> >{
		static sum_type add(Polynomial<F> const &a, Term const &b) {
			return Operator<Polynomial<F>, Polynomial<F>>::add(a, Polynomial<F>::fromTerm(b));
		}
		static sum_type sub(Polynomial<F> const &a, Term const &b) {
			return Operator<Polynomial<F>, Polynomial<F>>::sub(a, Polynomial<F>::fromTerm(b));
		}
		static prod_type mult(Polynomial<F> const &a, Term const &b) {
			return Operator<Polynomial<F>, Polynomial<F>>::mult(a, Polynomial<F>::fromTerm(b));
		}
	};

	template < typename F >
	struct Operator < Term, Polynomial<F> > : public Operator < Polynomial<F>, Polynomial<F> >{
		static sum_type add(Term const &a, Polynomial<F> const &b) {
			return Operator<Polynomial<F>, Polynomial<F>>::add(Polynomial<F>::fromTerm(a), b);
		}
		static sum_type sub(Term const &a, Polynomial<F> const &b) {
			return Operator<Polynomial<F>, Polynomial<F>>::sub(Polynomial<F>::fromTerm(a), b);
		}
		static prod_type mult(Term const &a, Polynomial<F> const &b) {
			return Operator<Polynomial<F>, Polynomial<F>>::mult(Polynomial<F>::fromTerm(a), b);
		}
	};

	template < typename F >
	struct Operator < Polynomial<F>, Variable > : public Operator < Polynomial<F>, Term >{
		static sum_type add(Polynomial<F> const &a, Variable const &b) {
			return Operator<Polynomial<F>, Term>::add(a, Term::fromVariable(b));
		}
		static sum_type sub(Polynomial<F> const &a, Variable const &b) {
			return Operator<Polynomial<F>, Term>::sub(a, Term::fromVariable(b));
		}
		static prod_type mult(Polynomial<F> const &a, Variable const &b) {
			return Operator<Polynomial<F>, Term>::mult(a, Term::fromVariable(b));
		}
	};

	template < typename F >
	struct Operator < Variable, Polynomial<F> > : public Operator < Term, Polynomial<F> >{
		static sum_type add(Variable const &a, Polynomial<F> const &b) {
			return Operator<Term, Polynomial<F>>::add(Term::fromVariable(a), b);
		}
		static sum_type sub(Variable const &a, Polynomial<F> const &b) {
			return Operator<Term, Polynomial<F>>::sub(Term::fromVariable(a), b);
		}
		static prod_type mult(Variable const &a, Polynomial<F> const &b) {
			return Operator<Term, Polynomial<F>>::mult(Term::fromVariable(a), b);
		}
	};

	template < >
	struct Operator < Term, Term > : public Operator < Polynomial<UnknownField>, Term >{
		typedef Term prod_type;

		static sum_type add(Term const &a, Term const &b) {
			return Operator<Polynomial<UnknownField>, Term>::add(Polynomial<UnknownField>::fromTerm(a), b);
		}

		static sum_type sub(Term const &a, Term const &b) {
			return Operator<Polynomial<UnknownField>, Term>::sub(Polynomial<UnknownField>::fromTerm(a), b);
		}

		static prod_type mult(Term const &a, Term const &b) {
			Term::MapType factors{ a.m_factors };
			for (auto &f : b.m_factors) {
				factors[f.first] += f.second;
			}
			return Term(factors);
		}
	};

	template < >
	struct Operator < Term, Variable > : public Operator < Term, Term >{
		static sum_type add(Term const &a, Variable const &b) {
			return Operator<Term, Term>::add(a, Term::fromVariable(b));
		}
		static sum_type sub(Term const &a, Variable const &b) {
			return Operator<Term, Term>::sub(a, Term::fromVariable(b));
		}
		static prod_type mult(Term const &a, Variable const &b) {
			return Operator<Term, Term>::mult(a, Term::fromVariable(b));
		}
	};

	template < >
	struct Operator < Variable, Term > : public Operator < Term, Term >{
		static sum_type add(Variable const &a, Term const &b) {
			return Operator<Term, Term>::add(Term::fromVariable(a), b);
		}
		static sum_type sub(Variable const &a, Term const &b) {
			return Operator<Term, Term>::sub(Term::fromVariable(a), b);
		}
		static prod_type mult(Variable const &a, Term const &b) {
			return Operator<Term, Term>::mult(Term::fromVariable(a), b);
		}
	};

	template < >
	struct Operator < Variable, Variable > : public Operator < Term, Variable >{
		static sum_type add(Variable const &a, Variable const &b) {
			return Operator<Term, Variable>::add(Term::fromVariable(a), b);
		}
		static sum_type sub(Variable const &a, Variable const &b) {
			return Operator<Term, Variable>::sub(Term::fromVariable(a), b);
		}
		static prod_type mult(Variable const &a, Variable const &b) {
			return Operator<Term, Variable>::mult(Term::fromVariable(a), b);
		}
	};

	template < typename A, typename B >
	typename Operator<A, B>::sum_type operator+(A const &a, B const &b) {
		return Operator<A, B>::add(a, b);
	}

	template < typename A, typename B >
	typename Operator<A, B>::sum_type operator-(A const &a, B const &b) {
		return Operator<A, B>::sub(a, b);
	}

	template < typename A, typename B >
	typename Operator<A, B>::prod_type operator*(A const &a, B const &b) {
		return Operator<A, B>::mult(a, b);
	}

	template < typename A >
	typename std::enable_if<!std::is_same<A, UnknownField>::value, Polynomial<A>>::type operator+(Polynomial<A> const &a, Polynomial<UnknownField> const &b) {
		return a + Polynomial<A>::fromUnknownField(b);
	}

	template < typename B >
	typename std::enable_if<!std::is_same<B, UnknownField>::value, Polynomial<B>>::type operator+(Polynomial<UnknownField> const &a, Polynomial<B> const &b) {
		return b + a;
	}

	template < typename A >
	typename std::enable_if<!std::is_same<A, UnknownField>::value, Polynomial<A>>::type operator-(Polynomial<A> const &a, Polynomial<UnknownField> const &b) {
		return a - Polynomial<A>::fromUnknownField(b);
	}

	template < typename B >
	typename std::enable_if<!std::is_same<B, UnknownField>::value, Polynomial<B>>::type operator-(Polynomial<UnknownField> const &a, Polynomial<B> const &b) {
		return Polynomial<B>::fromUnknownField(a) - b;
	}

	template < typename A >
	typename std::enable_if<!std::is_same<A, UnknownField>::value, Polynomial<A>>::type operator*(Polynomial<A> const &a, Polynomial<UnknownField> const &b) {
		return a*Polynomial<A>::fromUnknownField(b);
	}

	template < typename B >
	typename std::enable_if<!std::is_same<B, UnknownField>::value, Polynomial<B>>::type operator*(Polynomial<UnknownField> const &a, Polynomial<B> const &b) {
		return b*a;
	}

	template < typename F >
	typename std::enable_if<!std::is_same<F, UnknownField>::value && !is_symbolic<F>::value, Polynomial<F>>::type operator+(Polynomial<UnknownField> const &a, F const &b) {
		return Polynomial<F>::fromUnknownField(a) + b;
	}

	template < typename F >
	typename std::enable_if<!std::is_same<F, UnknownField>::value && !is_symbolic<F>::value, Polynomial<F>>::type operator+(F const &a, Polynomial<UnknownField> const &b) {
		return b + a;
	}

	template < typename F >
	typename std::enable_if<!std::is_same<F, UnknownField>::value && !is_symbolic<F>::value, Polynomial<F>>::type operator-(Polynomial<UnknownField> const &a, F const &b) {
		return Polynomial<F>::fromUnknownField(a) - b;
	}

	template < typename F >
	typename std::enable_if<!std::is_same<F, UnknownField>::value && !is_symbolic<F>::value, Polynomial<F>>::type operator-(F const &a, Polynomial<UnknownField> const &b) {
		return a - Polynomial<F>::fromUnknownField(b);
	}

	template < typename F >
	typename std::enable_if<!std::is_same<F, UnknownField>::value && !is_symbolic<F>::value, Polynomial<F>>::type operator*(Polynomial<UnknownField> const &a, F const &b) {
		return Polynomial<F>::fromUnknownField(a)*b;
	}

	template < typename F >
	typename std::enable_if<!std::is_same<F, UnknownField>::value && !is_symbolic<F>::value, Polynomial<F>>::type operator*(F const &a, Polynomial<UnknownField> const &b) {
		return b*a;
	}

	template < typename F >
	typename std::enable_if<!is_symbolic<F>::value, Polynomial<F>>::type operator+(F const &a, Term const &b) {
		return Polynomial<F>::fromConstant(a) + b;
	}

	template < typename F >
	typename std::enable_if<!is_symbolic<F>::value, Polynomial<F>>::type operator+(F const &a, Variable const &b) {
		return a + Term::fromVariable(b);
	}

	template < typename F >
	typename std::enable_if<!is_symbolic<F>::value, Polynomial<F>>::type operator+(Term const &a, F const &b) {
		return b + a;
	}

	template < typename F >
	typename std::enable_if<!is_symbolic<F>::value, Polynomial<F>>::type operator+(Variable const &a, F const &b) {
		return b + a;
	}

	template < typename F >
	typename std::enable_if<!is_symbolic<F>::value, Polynomial<F>>::type operator-(F const &a, Term const &b) {
		return Polynomial<F>::fromConstant(a) - b;
	}

	template < typename F >
	typename std::enable_if<!is_symbolic<F>::value, Polynomial<F>>::type operator-(F const &a, Variable const &b) {
		return a - Term::fromVariable(b);
	}

	template < typename F >
	typename std::enable_if<!is_symbolic<F>::value, Polynomial<F>>::type operator-(Term const &a, F const &b) {
		return Polynomial<F>::fromTerm(a) - b;
	}

	template < typename F >
	typename std::enable_if<!is_symbolic<F>::value, Polynomial<F>>::type operator-(Variable const &a, F const &b) {
		return Term::fromVariable(a) - b;
	}

	template < typename F >
	typename std::enable_if<!is_symbolic<F>::value, Polynomial<F>>::type operator*(F const &a, Term const &b) {
		return Polynomial<F>::fromConstant(a)*b;
	}

	template < typename F >
	typename std::enable_if<!is_symbolic<F>::value, Polynomial<F>>::type operator*(F const &a, Variable const &b) {
		return a*Term::fromVariable(b);
	}

	template < typename F >
	typename std::enable_if<!is_symbolic<F>::value, Polynomial<F>>::type operator*(Term const &a, F const &b) {
		return b*a;
	}

	template < typename F >
	typename std::enable_if<!is_symbolic<F>::value, Polynomial<F>>::type operator*(Variable const &a, F const &b) {
		return b*a;
	}

	template < typename F >
	typename std::enable_if<is_symbolic<F>::value, F>::type operator+(F const &a) {
		return a;
	}

	Polynomial<UnknownField> operator-(Term const&v) {
		return -Polynomial<UnknownField>::fromTerm(v);
	}

	Polynomial<UnknownField> operator-(Variable const&v) {
		return -Term::fromVariable(v);
	}
}
