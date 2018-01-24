#pragma once

// damn std::vector

struct boolean {
    boolean() = default;
    boolean(bool value) : value(value) {}
    operator bool &() { return value; }
    operator const bool&() const { return value; }
private:
    bool value;
};
