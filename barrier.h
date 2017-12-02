#pragma once
#include <mutex>
#include <condition_variable>

class barrier {
public:
    explicit barrier(std::size_t thread_count) :
        m_thread_count(thread_count),
        m_current_remaining(thread_count),
        m_reuse_switch(false) {
    }

    void sync() {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool current_switch = m_reuse_switch;
        m_current_remaining--;
        if (m_current_remaining == 0) {
            // when all sync() met, flip this switch
            // in this case we have clear signal about
            // sync() success. So spurious wake-ups are
            // avoided.
            // also, the next run starts with different
            // value, allowing reuse of the barrier.
            m_reuse_switch = !m_reuse_switch;
            m_current_remaining = m_thread_count;
            m_condition.notify_all();
        }
        else {
            m_condition.wait(lock, [this, current_switch] {
                return current_switch != m_reuse_switch;
            });
        }
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::size_t m_thread_count;
    std::size_t m_current_remaining;
    bool m_reuse_switch;
};
