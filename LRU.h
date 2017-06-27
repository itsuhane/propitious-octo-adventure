#pragma once

#include <vector>
#include <map>
#include <functional>

template <typename Key, typename Val>
class LRU {
public:
    typedef Key key_type;
    typedef Val value_type;
    typedef std::function<void(const key_type &key, value_type &value)> reader_fcn;
    typedef std::function<void(const key_type &key, const value_type &value)> writer_fcn;

private:
    struct Node {
        Node() {
            prev = next = -1;
        }
        std::ptrdiff_t prev;
        std::ptrdiff_t next;
        key_type key;
        value_type value;
    };

public:
    LRU(size_t capacity) {
        // capacity must be greater than 0!
        m_capacity = capacity;
        m_front = m_back = -1;
    }

    virtual ~LRU() {
        if (m_writer) {
            for (size_t i = 0; i < m_nodes.size(); ++i) {
                m_writer(m_nodes[i].key, m_nodes[i].value);
            }
        }
    }

    void cache(const key_type &key) {
        if (m_dict.count(key) == 0) {
            // we don't have it, add to cache
            if (m_nodes.size() >= m_capacity) {
                // already full, reuse last node
                std::ptrdiff_t pos = detach_back();
                m_dict.erase(m_nodes[pos].key);
                attach_front(pos);
                // store the old data before load the new
                if (m_writer) {
                    m_writer(m_nodes[m_front].key, m_nodes[m_front].value);
                }
            }
            else {
                // not full
                m_nodes.emplace_back(Node());
                attach_front(m_nodes.size() - 1);
            }
            // update node content
            m_nodes[m_front].key = key;
            if (m_reader) {
                m_reader(m_nodes[m_front].key, m_nodes[m_front].value);
            }
        }
        else {
            // we have it, just move to front
            std::ptrdiff_t pos = m_dict.at(key);
            detach(pos);
            attach_front(pos);
        }
        // update dict
        m_dict[key] = m_front;
    }

    value_type &get(const key_type &key) {
        cache(key);
        return m_nodes[m_front].value;
    }

    void set_reader(const reader_fcn &fcn = reader_fcn()) {
        m_reader = fcn;
    }

    void set_writer(const writer_fcn &fcn = writer_fcn()) {
        m_writer = fcn;
    }

private:
    // make the node at back detached
    std::ptrdiff_t detach_back() {
        std::ptrdiff_t ret = m_back;
        Node &node = m_nodes[m_back];

        m_back = node.prev;

        if (node.prev == -1) {
            // this is the only one
            m_front = -1;
        }
        else {
            m_nodes[node.prev].next = -1;
        }

        return ret;
    }

    void detach(std::ptrdiff_t pos) {
        Node &node = m_nodes[pos];
        if (node.prev == -1) { // is front
            m_front = node.next;
        }
        else {
            m_nodes[node.prev].next = node.next;
        }

        if (node.next == -1) { // is back
            m_back = node.prev;
        }
        else {
            m_nodes[node.next].prev = node.prev;
        }
    }

    void attach_front(std::ptrdiff_t pos) {
        Node &node = m_nodes[pos];
        node.prev = -1;
        node.next = m_front;

        if (m_front == -1) {
            // this is the only one, assert(pos==0)
            m_back = pos;
        }
        else {
            m_nodes[node.next].prev = pos;
        }

        m_front = pos;
    }

    void set_reader() {

    }

private:
    std::size_t m_capacity;
    std::ptrdiff_t m_front;
    std::ptrdiff_t m_back;
    std::vector<Node> m_nodes;
    std::map<key_type, std::ptrdiff_t> m_dict;

    reader_fcn m_reader;
    writer_fcn m_writer;
};

