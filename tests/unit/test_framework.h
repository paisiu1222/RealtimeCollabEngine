#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace test {

struct TestResult {
    std::string name;
    bool passed;
    std::string message;
    double duration_ms;
};

class TestRunner {
public:
    static TestRunner& getInstance() {
        static TestRunner instance;
        return instance;
    }
    
    void addTest(const std::string& name, std::function<void()> testFunc) {
        tests.push_back({name, testFunc});
    }
    
    void runAll() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Running Tests" << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        int totalTests = 0;
        int passedTests = 0;
        int failedTests = 0;
        
        for (auto& test : tests) {
            TestResult result;
            result.name = test.name;
            
            auto start = std::chrono::high_resolution_clock::now();
            
            try {
                test.func();
                result.passed = true;
                result.message = "PASSED";
                passedTests++;
            } catch (const std::exception& e) {
                result.passed = false;
                result.message = std::string("FAILED: ") + e.what();
                failedTests++;
            } catch (...) {
                result.passed = false;
                result.message = "FAILED: Unknown exception";
                failedTests++;
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
            
            totalTests++;
            results.push_back(result);
            
            // 打印测试结果
            std::cout << "[" << (result.passed ? "✓" : "✗") << "] " 
                     << result.name << " (" << result.duration_ms << " ms)" << std::endl;
            if (!result.passed) {
                std::cout << "    Error: " << result.message << std::endl;
            }
        }
        
        // 打印总结
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Test Summary" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total:  " << totalTests << std::endl;
        std::cout << "Passed: " << passedTests << std::endl;
        std::cout << "Failed: " << failedTests << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        if (failedTests > 0) {
            exit(1);
        }
    }
    
private:
    TestRunner() = default;
    ~TestRunner() = default;
    
    struct TestCase {
        std::string name;
        std::function<void()> func;
    };
    
    std::vector<TestCase> tests;
    std::vector<TestResult> results;
};

// 断言宏
#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        throw std::runtime_error("Assertion failed: " #condition); \
    }

#define ASSERT_FALSE(condition) \
    if (condition) { \
        throw std::runtime_error("Assertion failed: expected false but got true"); \
    }

#define ASSERT_EQ(expected, actual) \
    if (!((expected) == (actual))) { \
        throw std::runtime_error("Assertion failed: values are not equal"); \
    }

#define ASSERT_NE(a, b) \
    if ((a) == (b)) { \
        throw std::runtime_error("Assertion failed: values are equal"); \
    }

#define ASSERT_GT(a, b) \
    if ((a) <= (b)) { \
        throw std::runtime_error("Assertion failed: " #a " <= " #b); \
    }

#define ASSERT_LT(a, b) \
    if ((a) >= (b)) { \
        throw std::runtime_error("Assertion failed: " #a " >= " #b); \
    }

#define ASSERT_STREQ(expected, actual) \
    if (std::string(expected) != std::string(actual)) { \
        throw std::runtime_error(std::string("String assertion failed: \"") + expected + "\" != \"" + actual + "\""); \
    }

#define ASSERT_NOT_EMPTY(str) \
    if (std::string(str).empty()) { \
        throw std::runtime_error("Assertion failed: string is empty"); \
    }

#define ASSERT_GE(a, b) \
    if ((a) < (b)) { \
        throw std::runtime_error("Assertion failed: " #a " < " #b); \
    }

#define ASSERT_LE(a, b) \
    if ((a) > (b)) { \
        throw std::runtime_error("Assertion failed: " #a " > " #b); \
    }

// 注册测试宏
#define TEST(suite_name, test_name) \
    void suite_name##_##test_name(); \
    struct suite_name##_##test_name##_Registrar { \
        suite_name##_##test_name##_Registrar() { \
            test::TestRunner::getInstance().addTest(#suite_name "." #test_name, suite_name##_##test_name); \
        } \
    } suite_name##_##test_name##_registrar; \
    void suite_name##_##test_name()

} // namespace test
