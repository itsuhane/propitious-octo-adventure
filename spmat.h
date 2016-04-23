#pragma once

// A sparse matrix class using CRS scheme.
// I wrote this some years ago.

#include <vector>
#include <tuple>
#include <algorithm>
#include <numeric>
#include <ostream>
#include <iostream>

template<typename T>
class spmat {
public:
	typedef T elem_type;
	friend std::ostream &operator<<(std::ostream &os, const spmat &m);

public:
	spmat(size_t nrow, size_t ncol) {
		m_cols.push_back(ncol);
		m_row_start.resize(nrow + 1, 0);
	}

	spmat(size_t nrow, size_t ncol, const std::vector<size_t> &rows, const std::vector<size_t> &cols, const std::vector<elem_type> &vals) {
		using namespace std;
		typedef tuple<size_t, size_t, elem_type> E;
		vector<E> elements;
		if (vals.size() != rows.size() || vals.size() != cols.size()) {
			throw "input error";
		}
		for (size_t i = 0; i < vals.size(); ++i) {
			if (rows[i] >= nrow || cols[i]>=ncol) {
				throw "index out of bound";
			}
			if (vals[i] != elem_type(0)) { // fp zero check should be used here.
				elements.emplace_back(rows[i], cols[i], vals[i]);
			}
		}

		init(nrow, ncol, elements);
	}

	size_t nrow() const {
		return m_row_start.size() - 1;
	}

	size_t ncol() const {
		return m_cols.back();
	}

	elem_type at(size_t i, size_t j) const {
		using namespace std;
		if (i >= nrow() || j >= ncol()) {
			throw "index out of bound";
		}
		if (m_row_start[i] < m_row_start[i + 1]) {
			size_t ci = distance(m_cols.begin(), lower_bound(m_cols.begin() + m_row_start[i], m_cols.begin() + m_row_start[i + 1], j));
			if (ci < m_row_start[i+1] && m_cols[ci] == j) {
				return m_vals[ci];
			}
			else {
				return elem_type(0);
			}
		}
		else {
			return elem_type(0);
		}
	}

	void put(size_t i, size_t j, const elem_type &val) {
		using namespace std;
		if (i >= nrow() || j >= ncol()) {
			throw "index out of bound";
		}
		if (val == elem_type(0)) {
			return;
		}
		if (m_row_start[i] < m_row_start[i + 1]) {
			size_t ci = distance(m_cols.begin(), lower_bound(m_cols.begin() + m_row_start[i], m_cols.begin() + m_row_start[i + 1], j));
			if (ci < m_row_start[i + 1] && m_cols[ci] == j) {
				m_vals[ci] = val;
			}
			else {
				m_vals.insert(m_vals.begin() + ci, val);
				m_cols.insert(m_cols.begin() + ci, j);
				for (size_t ri = i + 1; ri < m_row_start.size(); ++ri) {
					m_row_start[ri]++;
				}
			}
		}
		else {
			size_t insert_pos = m_row_start[i + 1];
			m_vals.insert(m_vals.begin() + insert_pos, val);
			m_cols.insert(m_cols.begin() + insert_pos, j);
			for (size_t ri = i + 1; ri < m_row_start.size(); ++ri) {
				m_row_start[ri]++;
			}
		}
	}

	spmat operator- () const {
		spmat result(*this);
		for (auto &v : result.m_vals) {
			v = -v;
		}
		return result;
	}

	spmat operator+ (const spmat &m) const {
		using namespace std;
		typedef tuple<size_t, size_t, elem_type> E;
		size_t nr = nrow();
		size_t nc = ncol();
		if (nr != m.nrow() || nc != m.ncol()) {
			throw "incompatible";
		}

		vector<E> elements;
		for (size_t r = 0; r < nr; ++r) {
			size_t ci = m_row_start[r], cj = m.m_row_start[r];
			while (ci < m_row_start[r + 1] && cj < m.m_row_start[r + 1]) {
				if (m_cols[ci] < m.m_cols[cj]) {
					elements.emplace_back(r, m_cols[ci], m_vals[ci]);
					ci++;
				}
				else if (m_cols[ci] > m.m_cols[cj]) {
					elements.emplace_back(r, m.m_cols[cj], m.m_vals[cj]);
					cj++;
				}
				else {
					elements.emplace_back(r, m_cols[ci], m_vals[ci] + m.m_vals[cj]);
					ci++;
					cj++;
				}
			}
			while (ci < m_row_start[r + 1]) {
				elements.emplace_back(r, m_cols[ci], m_vals[ci]);
				ci++;
			}
			while (cj < m.m_row_start[r + 1]) {
				elements.emplace_back(r, m.m_cols[cj], m.m_vals[cj]);
				cj++;
			}
		}
		return spmat(nr, nc, elements);
	}

	spmat operator* (const spmat &m) const {
		using namespace std;
		typedef tuple<size_t, size_t, elem_type> E;

		size_t nr = nrow();
		size_t nk = ncol(); // == m.nrow();
		size_t nc = m.ncol();

		if (nk != m.nrow()) {
			throw "incompatible";
		}

		const spmat t = m.transpose(); // we transpose m to get a CSC representation for faster multiplication

		vector<E> elements;

		for (size_t i = 0; i < nr; ++i) {
			for (size_t j = 0; j < nc; ++j) {
				size_t ci = m_row_start[i];
				size_t cj = t.m_row_start[j];
				elem_type sum(0);
				while (ci < m_row_start[i + 1] && cj < t.m_row_start[j + 1]) {
					if (m_cols[ci] < t.m_cols[cj]) {
						ci++;
					}
					else if (m_cols[ci] > t.m_cols[cj]) {
						cj++;
					}
					else {
						sum += m_vals[ci] * t.m_vals[cj];
						ci++;
						cj++;
					}
				}
				if (sum != elem_type(0)) {
					E element;
					get<0>(element) = i;
					get<1>(element) = j;
					get<2>(element) = sum;
					elements.push_back(element);
				}
			}
		}

		return spmat(nr, nc, elements);
	}

	spmat transpose() const {
		using namespace std;
		typedef tuple<size_t, size_t, elem_type> E;
		vector<E> elements(m_vals.size());
		for (size_t r = 0; r < nrow(); ++r) {
			for (size_t ci = m_row_start[r]; ci < m_row_start[r + 1]; ++ci) {
				E &element = elements[ci];
				get<0>(element) = m_cols[ci];
				get<1>(element) = r;
				get<2>(element) = m_vals[ci];
			}
		}
		return spmat(ncol(), nrow(), elements);
	}

	std::vector<elem_type> operator* (const std::vector<elem_type> &v) const {
		using namespace std;
		if (ncol() != v.size()) {
			throw "incompatible";
		}
		vector<elem_type> result(nrow());
		multInPlace(v, result);
		return result;
	}

	std::vector<elem_type> solveJ(const std::vector<elem_type> &b, size_t verbose = 0, const elem_type &threshold = elem_type(1.e-6), size_t max_iter = 1000000) const {
		using namespace std;
		if (nrow() != b.size()) {
			throw "incompatible";
		}
		if (nrow() != ncol()) {
			throw "non-square";
		}
		size_t v_count = 0;
		std::vector<elem_type> x = b;
		std::vector<elem_type> x_new(b.size(), elem_type(0));

		elem_type diff;
		size_t n_iter = 0;
		do {
			diff = elem_type(0);
			for (size_t r = 0; r < nrow(); ++r) {
				x_new[r] = b[r];
				elem_type aii(0);
				for (size_t ci = m_row_start[r]; ci < m_row_start[r + 1]; ++ci) {
					if (m_cols[ci] != r) {
						x_new[r] -= m_vals[ci] * x[m_cols[ci]];
					}
					else {
						aii = m_vals[ci];
					}
				}
				if (aii == elem_type(0)) {
					throw "zero diagonal";
				}
				x_new[r] /= aii;
				diff = max(diff, abs(x_new[r] - x[r]));
			}
			x_new.swap(x);
			n_iter++;
			if (verbose) {
				v_count++;
				if (v_count == verbose) {
					cout << "Method: Jacobi, Iter " << n_iter << ", |x-x'|_inf = " << diff << endl;
					v_count = 0;
				}
			}
			if (n_iter >= max_iter) {
				throw "not converging";
			}
		} while (diff>threshold);
		return x;
	}

	std::vector<elem_type> solveGS(const std::vector<elem_type> &b, size_t verbose = 0, const elem_type &threshold = elem_type(1.e-6), size_t max_iter = 1000000) const {
		return solveSOR(b, elem_type(1), verbose, threshold, max_iter);
	}

	std::vector<elem_type> solveSOR(const std::vector<elem_type> &b, const elem_type &lambda = elem_type(1.67), size_t verbose = 0, const elem_type &threshold = elem_type(1.e-6), size_t max_iter = 1000000) const {
		using namespace std;
		if (nrow() != b.size()) {
			throw "incompatible";
		}
		if (nrow() != ncol()) {
			throw "non-square";
		}
		if (lambda < elem_type(1.0) || lambda >= elem_type(2.0)) {
			throw "wrong lambda";
		}
		size_t v_count = 0;
		std::vector<elem_type> x = b;
		elem_type diff;
		size_t n_iter = 0;
		do {
			diff = elem_type(0);
			for (size_t r = 0; r < nrow(); ++r) {
				elem_type x_new = b[r];
				elem_type aii(0);
				for (size_t ci = m_row_start[r]; ci < m_row_start[r + 1]; ++ci) {
					if (m_cols[ci] != r) {
						x_new -= m_vals[ci] * x[m_cols[ci]];
					}
					else {
						aii = m_vals[ci];
					}
				}
				if (aii == elem_type(0)) {
					throw "zero diagonal";
				}
				x_new = lambda*x_new/aii + (elem_type(1) - lambda)*x[r];
				diff = max(diff, abs(x_new - x[r]));
				x[r] = x_new;
			}
			n_iter++;
			if (verbose) {
				v_count++;
				if (v_count == verbose) {
					if (lambda == elem_type(1)) {
						cout << "Method: Gauss-Seidel, Iter ";
					}
					else {
						cout << "Method: SOR(" << lambda << "), Iter ";
					}
					cout << n_iter << ", |x-x'|_inf = " << diff << endl;
					v_count = 0;
				}
			}
			if (n_iter >= max_iter) {
				throw "not converging";
			}
		} while (diff>threshold);
		return x;
	}

	std::vector<elem_type> solveCG(const std::vector<elem_type> &b, size_t verbose = 0, const elem_type &threshold = elem_type(1.e-6), size_t max_iter = 1000000) const {
		using namespace std;
		if (nrow() != b.size()) {
			throw "incompatible";
		}
		if (nrow() != ncol()) {
			throw "non-square";
		}
		size_t v_count = 0;
		vector<elem_type> x(b.size(), elem_type(0));
		vector<elem_type> r = b;
		vector<elem_type> p = r;
		vector<elem_type> Ap(b.size());
		elem_type rr_old = inner_product(r.begin(), r.end(), r.begin(), elem_type(0));
		for (size_t n_iter = 0; n_iter < max_iter; ++n_iter) {
			multInPlace(p, Ap);
			elem_type pAp = inner_product(p.begin(), p.end(), Ap.begin(), elem_type(0));
			elem_type alpha = rr_old / pAp;
			for (size_t i = 0; i < x.size(); ++i) {
				x[i] += alpha*p[i];
				r[i] -= alpha*Ap[i];
			}
			elem_type rr_new = inner_product(r.begin(), r.end(), r.begin(), elem_type(0));
			if (verbose) {
				v_count++;
				if (v_count == verbose) {
					cout << "Method: Conjugate Gradient, Iter " << n_iter+1 << ", r^2 = " << rr_new << endl;
					v_count = 0;
				}
			}
			if (sqrt(rr_new) < threshold) {
				return x;
			}
			elem_type beta = rr_new / rr_old;
			for (size_t i = 0; i < p.size(); ++i) {
				p[i] = beta*p[i] + r[i];
			}
			rr_old = rr_new;
		}
		throw "not converging";
	}

private:
	spmat(size_t nrow, size_t ncol, std::vector<std::tuple<size_t, size_t, elem_type>> &elements) {
		init(nrow, ncol, elements);
	}

	void init(size_t nrow, size_t ncol, std::vector<std::tuple<size_t, size_t, elem_type>> &elements) { // WARNING: will change elements
		using namespace std;
		typedef tuple<size_t, size_t, elem_type> E;
		sort(elements.begin(), elements.end(), [](const E &a, const E &b)->bool {
			if (get<0>(a) < get<0>(b)) {
				return true;
			}
			else if (get<0>(a) > get<0>(b)){
				return false;
			}
			else {
				return  get<1>(a) < get<1>(b);;
			}
		});

		size_t elem_num = distance(elements.begin(), unique(elements.begin(), elements.end()));

		m_vals.resize(elem_num);
		m_cols.resize(elem_num + 1);
		m_row_start.resize(nrow + 1);

		size_t next_row = 0;
		for (size_t i = 0; i < elem_num; ++i) {
			size_t curr_row = get<0>(elements[i]);
			while (next_row <= curr_row) {
				next_row++;
				m_row_start[next_row] = m_row_start[next_row - 1];
			}
			size_t &next_row_start = m_row_start[next_row];
			m_cols[next_row_start] = get<1>(elements[i]);
			m_vals[next_row_start] = get<2>(elements[i]);
			next_row_start++;
		}
		while (next_row < nrow) {
			next_row++;
			m_row_start[next_row] = m_row_start[next_row - 1];
		}

		m_cols.back() = ncol;
	}

	// result = (*this) * v, no dimension check.
	void multInPlace(const std::vector<elem_type> &v, std::vector<elem_type> &result) const {
		using namespace std;
		size_t nr = nrow();
		for (size_t r = 0; r < nr; ++r) {
			result[r] = elem_type(0);
			for (size_t ci = m_row_start[r]; ci < m_row_start[r + 1]; ++ci) {
				result[r] += m_vals[ci] * v[m_cols[ci]];
			}
		}
	}

	std::vector<elem_type> m_vals;
	std::vector<size_t> m_cols; // we store the col counts in its very end.
	std::vector<size_t> m_row_start; // we store the non-zero element count in its very end.
};

std::ostream &operator<<(std::ostream &os, const spmat &m) {
	for (size_t r = 0; r < m.nrow(); ++r) {
		if (r > 0) {
			os << std::endl;
		}
		if (m.ncol() > 0) {
			os << m.at(r, 0);
		}
		for (size_t c = 1; c < m.ncol(); ++c) {
			os << ", " << m.at(r, c);
		}
	}
	return os;
}
