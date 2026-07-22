#pragma once

#include <stdexcept>
#include <string>

namespace sts::test {

inline void require(bool condition, const char* expression, const char* file, int line) {
    if (!condition) {
        throw std::runtime_error(std::string(file) + ":" + std::to_string(line) + " requirement failed: " + expression);
    }
}

} // namespace sts::test

#define REQUIRE(expression) ::sts::test::require((expression), #expression, __FILE__, __LINE__)
