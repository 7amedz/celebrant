#pragma once

#include <utility>
#include <vector>

namespace celebrant {

template <typename T, typename E> class Expected {
  public:
    Expected(T value)
        : has_value_(true), value_(std::move(value)) {
    } // move: take ownership of the value instead of copying
    Expected(E error) : has_value_(false), error_(std::move(error)) {} //
    bool has_value() const {
        return has_value_;
    }
    const T& value() const {
        return value_;
    }
    const E& error() const {
        return error_;
    }

  private:
    bool has_value_;
    T value_{};
    E error_{};
};

} // namespace celebrant
