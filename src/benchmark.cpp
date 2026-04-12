#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <string>
#include <iomanip>

#include "thread_pool.hpp"

using Clock = std::chrono::steady_clock;

enum class TaskSize {
    Tiny,
    Medium,
    Heavy
};

int computeTask(int x, TaskSize size) {
    int iterations = 0;

    switch (size) {
        case TaskSize::Tiny:   iterations = 1'000; break;
        case TaskSize::Medium: iterations = 100'000; break;
        case TaskSize::Heavy:  iterations = 1'000'000; break;
    }

    int v = x + 1;
    for (int i = 0; i < iterations; ++i) {
        v = (v * 31 + 7) % 1'000'003;
    }
    return v;
}

template <typename F>
long long measureMs(F&& func) {
    auto start = Clock::now();
    func();
    auto end = Clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

const char* toString(TaskSize size) {
    switch (size) {
        case TaskSize::Tiny: return "Tiny";
        case TaskSize::Medium: return "Medium";
        case TaskSize::Heavy: return "Heavy";
    }
    return "Unknown";
}

long long benchSingleThread(int numTasks, TaskSize size) {
    volatile long long sink = 0;
    return measureMs([&] {
        for (int i = 0; i < numTasks; ++i) {
            sink += computeTask(i, size);
        }
    });
}

long long benchThreadPerTask(int numTasks, TaskSize size) {
    volatile long long sink = 0;
    return measureMs([&] {
        std::vector<std::thread> threads;
        threads.reserve(numTasks);

        std::vector<int> results(numTasks, 0);

        for (int i = 0; i < numTasks; ++i) {
            threads.emplace_back([i, size, &results] {
                results[i] = computeTask(i, size);
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        for (int x : results) {
            sink += x;
        }
    });
}

long long benchThreadPool(int numTasks, TaskSize size, int numWorkers) {
    volatile long long sink = 0;
    return measureMs([&] {
        ThreadPool pool(numWorkers);
        std::vector<std::future<int>> futures;
        futures.reserve(numTasks);

        for (int i = 0; i < numTasks; ++i) {
            futures.push_back(pool.submit(computeTask, i, size));
        }

        for (auto& fut : futures) {
            sink += fut.get();
        }
    });
}

void runGranularityExperiment() {
    std::cout << "=== Experiment A: Task Granularity ===\n";

    struct Case {
        TaskSize size;
        int numTasks;
    };

    std::vector<Case> cases = {
        {TaskSize::Tiny,   1000},
        {TaskSize::Medium, 300},
        {TaskSize::Heavy,  50}
    };

    const int poolWorkers = 4;

    std::cout << std::left
              << std::setw(10) << "Task"
              << std::setw(12) << "NumTasks"
              << std::setw(16) << "Single(ms)"
              << std::setw(20) << "ThreadPerTask(ms)"
              << std::setw(16) << "ThreadPool(ms)"
              << "\n";

    for (const auto& c : cases) {
        long long singleMs = benchSingleThread(c.numTasks, c.size);
        long long threadPerTaskMs = benchThreadPerTask(c.numTasks, c.size);
        long long poolMs = benchThreadPool(c.numTasks, c.size, poolWorkers);

        std::cout << std::left
                  << std::setw(10) << toString(c.size)
                  << std::setw(12) << c.numTasks
                  << std::setw(16) << singleMs
                  << std::setw(20) << threadPerTaskMs
                  << std::setw(16) << poolMs
                  << "\n";
    }

    std::cout << "\n";
}

void runScalingExperiment() {
    std::cout << "=== Experiment B: Worker Scaling ===\n";

    const TaskSize size = TaskSize::Medium;
    const int numTasks = 500;
    std::vector<int> workerCounts = {1, 2, 4, 8};

    std::cout << "TaskSize=" << toString(size)
              << ", NumTasks=" << numTasks << "\n";

    std::cout << std::left
              << std::setw(12) << "Workers"
              << std::setw(16) << "ThreadPool(ms)"
              << "\n";

    for (int workers : workerCounts) {
        long long poolMs = benchThreadPool(numTasks, size, workers);

        std::cout << std::left
                  << std::setw(12) << workers
                  << std::setw(16) << poolMs
                  << "\n";
    }

    std::cout << "\n";
}

int main() {
    runGranularityExperiment();
    runScalingExperiment();
    return 0;
}