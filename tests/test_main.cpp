#include "test_common.h"

#include <iostream>

namespace simbus::test {

void register_routetable_tests(std::vector<TestCase>& tests);
void register_basic_transfer_tests(std::vector<TestCase>& tests);
void register_congestion_tests(std::vector<TestCase>& tests);
void register_stress_tests(std::vector<TestCase>& tests);

}  // namespace simbus::test

int main() {
    using namespace simbus::test;

    std::vector<TestCase> tests;
    register_routetable_tests(tests);
    register_basic_transfer_tests(tests);
    register_congestion_tests(tests);
    register_stress_tests(tests);

    try {
        for (const auto& test : tests) {
            test.fn();
            std::cout << "[PASS] " << test.name << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "[SUMMARY] " << tests.size() << " tests passed\n";
    return EXIT_SUCCESS;
}
