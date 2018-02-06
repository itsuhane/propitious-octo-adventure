#pragma once

#include <vector>
#include <map>
#include <functional>

template <typename K, typename V, typename Cmp = std::less<V>>
class priority_map {
    typedef size_t heap_index_type;
public:
    typedef K key_type;
    typedef V value_type;

    size_t size() const {
        return heap_size();
    }

    size_t count(key_type key) const {
        return m_data.count(key);
    }

    void put(key_type key, const value_type &value) {
        if (count(key) > 0) {
            bool cmp = value_compare(m_data.at(key).value, value);
            m_data.at(key).value = value;
            if (cmp) {
                heap_sift_up(m_data.at(key).heap_index);
            }
            else {
                heap_sift_down(m_data.at(key).heap_index);
            }
        }
        else {
            data_item &new_item = m_data[key];
            new_item.value = value;
            new_item.heap_index = heap_size();
            m_heap.push_back(key);
            heap_sift_up(heap_size() - 1);
        }
    }

    value_type at(key_type key) const {
        return m_data.at(key).value;
    }

    key_type top_key() const {
        return m_heap[0];
    }

    value_type top_value() const {
        return m_data.at(m_heap[0]).value;
    }

    void pop() {
        if (heap_size() > 0) {
            key_type kt = top_key();
            heap_swap(0, heap_size() - 1);
            m_data.erase(kt);
            m_heap.pop_back();
            heap_sift_down(0);
        }
    }

private:
    void heap_sift_up(heap_index_type h) {
        heap_index_type hp;
        while (h > 0 && heap_compare(hp = heap_parent(h), h)) {
            heap_swap(h, hp);
            h = hp;
        }
    }

    void heap_sift_down(heap_index_type h) {
        while (h < heap_size()) {
            heap_index_type ht = h;
            heap_index_type hl = heap_left(h);
            heap_index_type hr = heap_right(h);
            if (hl < heap_size() && heap_compare(ht, hl)) ht = hl;
            if (hr < heap_size() && heap_compare(ht, hr)) ht = hr;
            if (ht == h) break;
            heap_swap(h, ht);
            h = ht;
        }
    }

    void heap_swap(heap_index_type ha, heap_index_type hb) {
        if (ha != hb) {
            std::swap(m_data.at(m_heap[ha]).heap_index, m_data.at(m_heap[hb]).heap_index);
            std::swap(m_heap[ha], m_heap[hb]);
        }
    }

    bool value_compare(const value_type &va, const value_type &vb) const {
        return m_cmp(va, vb);
    }

    bool heap_compare(heap_index_type ha, heap_index_type hb) const {
        return value_compare(m_data.at(m_heap[ha]).value, m_data.at(m_heap[hb]).value);
    }

    size_t heap_size() const {
        return m_heap.size();
    }

    static heap_index_type heap_parent(heap_index_type h) {
        return (h - 1) / 2;
    }

    static heap_index_type heap_left(heap_index_type h) {
        return h * 2 + 1;
    }

    static heap_index_type heap_right(heap_index_type h) {
        return h * 2 + 2;
    }

private:
    struct data_item {
        value_type value;
        heap_index_type heap_index;
    };
    std::map<key_type, data_item> m_data;
    std::vector<key_type> m_heap;

    Cmp m_cmp;
};
