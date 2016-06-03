#pragma once

#include <cmath>
#include <array>
#include <vector>
#include <limits>
#include <type_traits>

template <size_t Dimension>
class fast_marching {
    typedef double value_type;
    typedef char state_type;
    typedef size_t voxel_index;
    typedef size_t heap_index;
    typedef std::array<size_t, Dimension> space_index;
    static_assert(std::is_unsigned<size_t>::value, "space_index must use unsigned type!");

public:
    static const size_t dimension = Dimension;

    size_t get_size(size_t idim) const {
        return m_sizes[idim];
    }

    void set_size(size_t idim, size_t size) {
        m_sizes[idim] = size;
    }

    value_type get_band_threshold() const {
        return m_band_threshold;
    }

    void set_band_threshold(double t) {
        m_band_threshold = t;
    }

    void reset() {
        voxel_reset();
        heap_reset();
    }

    void march() {
        while (heap_size() > 0) {
            voxel_index v = heap_pop();
            m_voxel_states[v] = voxel_state_accepted;
            if (voxel_value(v) < m_band_threshold) {
                space_index s;
                voxel_to_space_decompose(v, s);
                voxel_update_neighbors(s);
            }
        }
    }

    void set_init_voxel(const space_index &s, const value_type &value) {
        voxel_index v = space_to_voxel(s);
        m_voxel_values[v] = value;
        m_voxel_states[v] = voxel_state_accepted;
        voxel_update_neighbors(s);
    }

private:
    static const heap_index heap_nil = heap_index(-1);
    /*constexpr*/ const value_type voxel_value_inf = std::numeric_limits<value_type>::max();
    static const state_type voxel_state_unknown = 0;
    static const state_type voxel_state_accepted = 1;

    bool space_is_inside(const space_index &s) const {
        // [unroll]
        for (size_t d = 0; d < dimension; ++d) {
            // we use unsigned type, so negative coordinates are wrapped around
            if (s[d] >= m_sizes[d]) {
                return false;
            }
        }
        return true;
    }

    void voxel_reset() {
        size_t c = voxel_count();
        m_voxel_values.swap(std::vector<value_type>(c, voxel_value_inf));
        m_voxel_states.swap(std::vector<state_type>(c, voxel_state_unknown));
    }

    size_t voxel_count() const {
        size_t c = 1;
        for (size_t d = 0; d < dimension; ++d) {
            c *= m_sizes[d];
        }
        return c;
    }

    const value_type &voxel_value(const voxel_index &v) const {
        return m_voxel_values[v];
    }

    const state_type &voxel_state(const voxel_index &v) const {
        return m_voxel_states[v];
    }

    void voxel_set_value(const voxel_index &v, const value_type &value) {
        value_type value_old = voxel_value(v);
        m_voxel_values[v] = value;
        heap_index h = voxel_to_heap(v);
        if (h != heap_nil) {
            if (value < value_old) {
                heap_decrease(h);
            }
            else if (value > value_old) {
                heap_increase(h);
            }
        }
    }

    value_type voxel_value_solve(const voxel_index &v, space_index s) const {
        int n = 0;
        value_type sval = 0.0;
        value_type sqval = 0.0;

        for (size_t d = 0; d < dimension; ++d) {
            value_type val = voxel_value_inf;
            bool has_val = false;
            s[d]++;
            if (space_is_inside(s)) {
                voxel_index v = space_to_voxel(s);
                if (voxel_state(v) == voxel_state_accepted) {
                    val = std::min(val, voxel_value(v));
                    has_val = true;
                }
            }
            s[d] -= 2;
            if (space_is_inside(s)) {
                voxel_index v = space_to_voxel(s);
                if (voxel_state(v) == voxel_state_accepted) {
                    val = std::min(val, voxel_value(v));
                    has_val = true;
                }
            }
            s[d]++;

            if (has_val) {
                n++;
                sval += val;
                sqval += val*val;
            }
        }

        return (sval + sqrt(sval*sval - n*(sqval - 1))) / n;
    }

    void voxel_update_neighbors(const space_index &s) {
        space_index ns = s;
        // [unroll]
        for (size_t td = 0; td < dimension*2; ++td) {
            if (td < dimension) {
                ns[td%dimension]++;
            }
            else {
                ns[td%dimension]--;
            }
            if (space_is_inside(ns)) {
                voxel_index v = space_to_voxel(ns);
                if (voxel_state(v) != voxel_state_accepted) {
                    double value = voxel_value_solve(v, ns);
                    if (voxel_to_heap(v) == heap_nil) {
                        m_voxel_values[v] = value;
                        heap_push(v);
                    }
                    else {
                        voxel_set_value(v, value);
                    }
                }
            }
            if (td < dimension) {
                ns[td%dimension]--;
            }
            else {
                ns[td%dimension]++;
            }
        }
    }

    static heap_index heap_parent(const heap_index &h) {
        return (h - 1) / 2;
    }

    static heap_index heap_left(const heap_index &h) {
        return h * 2 + 1;
    }

    static heap_index heap_right(const heap_index &h) {
        return h * 2 + 2;
    }

    const voxel_index &heap_to_voxel(const heap_index &h) const {
        return m_heap_perms[h];
    }

    const heap_index &voxel_to_heap(const voxel_index &v) const {
        return m_heap_brefs[v];
    }

    voxel_index space_to_voxel(const space_index &s) const {
        voxel_index v = s[0];
        // [unroll]
        for (size_t d = 1; d < dimension; ++d) {
            v = v*m_sizes[d] + s[d];
        }
        return v;
    }

    void voxel_to_space_decompose(voxel_index v, space_index &s) const {
        // [unroll]
        for (size_t d = dimension - 1; d > 0; --d) {
            s[d] = v % m_sizes[d];
            v /= m_sizes[d];
        }
        s[0] = v;
    }

    void heap_reset() {
        m_heap_brefs.swap(std::vector<heap_index>(voxel_count(), heap_nil));
        m_heap_perms.clear();
    }

    size_t heap_size() const {
        return m_heap_perms.size();
    }

    void heap_swap(const heap_index &h1, const heap_index &h2) {
        std::swap(m_heap_brefs[heap_to_voxel(h1)], m_heap_brefs[heap_to_voxel(h2)]);
        std::swap(m_heap_perms[h1], m_heap_perms[h2]);
    }

    void heap_push(const voxel_index &v) {
        heap_index h = heap_size();

        m_heap_perms.push_back(v);
        m_heap_brefs[v] = h;

        heap_decrease(h);
    }

    voxel_index heap_pop() {
        voxel_index v = heap_to_voxel(0);
        heap_swap(0, heap_size() - 1);

        m_heap_perms.pop_back();
        m_heap_brefs[v] = heap_nil;

        heap_increase(0);

        return v;
    }

    void heap_decrease(heap_index h) {
        heap_index parent;
        while (h > 0 && voxel_value(heap_to_voxel(h)) < voxel_value(heap_to_voxel(parent = heap_parent(h)))) {
            heap_swap(h, parent);
            h = parent;
        }
    }
    
    void heap_increase(heap_index h) {
        size_t target, left, right;
        while (h < heap_size()) {
            target = h;
            left = heap_left(h);
            right = heap_right(h);
            if (left < heap_size() && voxel_value(heap_to_voxel(left)) < voxel_value(heap_to_voxel(target))) {
                target = left;
            }
            if (right < heap_size() && voxel_value(heap_to_voxel(right)) < voxel_value(heap_to_voxel(target))) {
                target = right;
            }
            if (h != target) {
                heap_swap(h, target);
                h = target;
            }
            else {
                break;
            }
        }
    }

    std::array<size_t, Dimension> m_sizes;
    
    std::vector<value_type> m_voxel_values;
    std::vector<state_type> m_voxel_states;

    std::vector<heap_index> m_heap_brefs;
    std::vector<voxel_index> m_heap_perms;

    value_type m_band_threshold = std::numeric_limits<value_type>::max();
};