#pragma once
#include <vector>
#include <functional>

// A naive implementation based on wikipedia
// may not good for serious application

template<typename S, typename V>
class RungeKutta {
public:
    RungeKutta() {
        setRK4();
    }

    V integrate(const std::function<V(const S&, const V&)> &f, const V &y0, const S &t0, const S &t1) {
        size_t s = stageNum();
        std::vector<V> k(s);
        S h = t1 - t0;
        V y1 = y0;
        k[0] = f(t0, y0);
        for (size_t i = 1; i < s; ++i) {
            S ti = t0 + h*node(i);
            V yi = y0;
            for (size_t j = 0; j < i; ++j) {
                S c = coefficient(i - 1, j);
                if (c != 0) {
                    yi += h*coefficient(i - 1, j)*k[j];
                }
            }
            k[i] = f(ti, yi);
        }
        for (size_t i = 0; i < s; ++i) {
            S w = weight(i);
            if (w != 0) {
                y1 += h*weight(i)*k[i];
            }
        }
        return y1;
    }

    void setForwardEuler() {
        setStageNum(1);
        setWeight(0, 1);
    }
    void setSecondOrder(const S &x) {
        setStageNum(2);
        setNode(1, x);
        setWeight(0, 1 - 0.5 / x);
        setWeight(1, 0.5 / x);
        setCoefficient(0, 0, x);
    }
    void setMidpoint() {
        setSecondOrder(0.5);
    }
    void setHeun() {
        setSecondOrder(1);
    }
    void setRalston() {
        setSecondOrder(2.0 / 3.0);
    }
    void setRK3() {
        setStageNum(3);
        setNode(1, 0.5);
        setNode(2, 1);
        setWeight(0, 1.0 / 6.0);
        setWeight(1, 2.0 / 3.0);
        setWeight(2, 1.0 / 6.0);
        setCoefficient(0, 0, 0.5);
        setCoefficient(1, 0, -1);
        setCoefficient(1, 1, 2);
    }
    void setRK4() {
        setStageNum(4);
        setNode(1, 0.5);
        setNode(2, 0.5);
        setNode(3, 1);
        setWeight(0, 1.0 / 6.0);
        setWeight(1, 1.0 / 3.0);
        setWeight(2, 1.0 / 3.0);
        setWeight(3, 1.0 / 6.0);
        setCoefficient(0, 0, 0.5);
        setCoefficient(1, 1, 0.5);
        setCoefficient(2, 2, 1);
    }

    void set38RK4() {
        setStageNum(4);
        setNode(1, 1.0 / 3.0);
        setNode(2, 2.0 / 3.0);
        setNode(3, 1);
        setWeight(0, 1.0 / 8.0);
        setWeight(1, 3.0 / 8.0);
        setWeight(2, 3.0 / 8.0);
        setWeight(3, 1.0 / 8.0);
        setCoefficient(0, 0, 1.0 / 3.0);
        setCoefficient(1, 0, -1.0 / 3.0);
        setCoefficient(1, 1, 1);
        setCoefficient(2, 0, 1);
        setCoefficient(2, 1, -1);
        setCoefficient(2, 2, 1);
    }

    void setStageNum(size_t num) {
        m_stages = num;
        m_nodes.clear();
        m_weights.clear();
        m_coefficients.clear();
        m_nodes.resize(num, 0);
        m_weights.resize(num, 0);
        m_coefficients.resize(num*(num - 1) / 2, 0);
    }

    size_t stageNum() const { return m_stages; }

    void setNode(size_t n, const S &v) { m_nodes[n] = v; }
    const S &node(size_t n) const { return m_nodes[n]; }

    void setWeight(size_t w, const S &v) { m_weights[w] = v; }
    const S &weight(size_t w) const { return m_weights[w]; }

    void setCoefficient(size_t i, size_t j, const S &v) { m_coefficients[(i + 1)*i / 2 + j] = v; }
    const S &coefficient(size_t i, size_t j) const { return m_coefficients[(i + 1)*i / 2 + j]; }

private:
    size_t m_stages;
    std::vector<S> m_nodes;
    std::vector<S> m_weights;
    std::vector<S> m_coefficients;
};
