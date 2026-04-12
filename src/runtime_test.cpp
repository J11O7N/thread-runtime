#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <numeric>
#include <stdexcept>

#include "thread_pool.hpp"

int add(int a, int b) {
    return a + b;
}

int heavyWork(int n) {
    int sum = 0;
    for (int i = 0; i < n; ++i) {
        sum += (i % 7);
    }
    return sum;
}

int main() {
    try {
        ThreadPool pool(4);

        // 1) 기본 submit / future 검증
        auto f1 = pool.submit(add, 3, 5);
        auto f2 = pool.submit([](int x) { return x * x; }, 12);

        std::cout << "[Basic]\n";
        std::cout << "add(3,5) = " << f1.get() << "\n";
        std::cout << "square(12) = " << f2.get() << "\n\n";

        // 2) 여러 task 동시에 제출
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 10; ++i) {
            futures.push_back(pool.submit([i] {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                return i * 10;
            }));
        }

        std::cout << "[Batch Tasks]\n";
        for (auto& fut : futures) {
            std::cout << fut.get() << " ";
        }
        std::cout << "\n\n";

        // 3) 무거운 계산 task 검증
        std::cout << "[Heavy Work]\n";
        auto f3 = pool.submit(heavyWork, 1'000'000);
        std::cout << "heavyWork result = " << f3.get() << "\n\n";

        // 4) 예외 전달 검증
        std::cout << "[Exception Propagation]\n";
        auto f4 = pool.submit([]() -> int {
            throw std::runtime_error("task failed");
        });

        try {
            std::cout << f4.get() << "\n";
        } catch (const std::exception& e) {
            std::cout << "caught exception from task: " << e.what() << "\n";
        }

        std::cout << "\nAll checks passed.\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}