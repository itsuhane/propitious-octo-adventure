#pragma once

// damn std::vector

struct boolean {
    boolean() = default;
    boolean(bool value) : value(value) {}
    operator bool&() { return value; }
    operator const bool&() const { return value; }
    bool* operator&() { return &value; }
    const bool* operator&() const { return &value; }
private:
    bool value;
};
