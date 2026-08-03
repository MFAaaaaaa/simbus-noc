#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <exception>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace simbus {

inline uint32_t align_up(uint32_t value, uint32_t align) {
    if (align == 0) {
        throw std::invalid_argument("align must not be zero");
    }
    return ((value + align - 1) / align) * align;
}

inline void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename... Args>
inline std::string str_join(Args&&... args) {
    std::ostringstream os;
    (os << ... << args);
    return os.str();
}

}  // namespace simbus
