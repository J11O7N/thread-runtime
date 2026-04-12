#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "httplib.h"
#include "thread_pool.hpp"
#include "onnxruntime_cxx_api.h"

using Clock = std::chrono::steady_clock;

struct Metrics {
    std::atomic<long long> total_requests{0};
    std::atomic<long long> total_inference_ms{0};
};

class MnistOnnxRuntime {
public:
    explicit MnistOnnxRuntime(const std::string& model_path)
        : env_(ORT_LOGGING_LEVEL_WARNING, "mnist-runtime"),
          session_(nullptr),
          allocator_() {
        Ort::SessionOptions options;
        options.SetIntraOpNumThreads(1);
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

        session_ = Ort::Session(env_, model_path.c_str(), options);

        auto input_name_alloc = session_.GetInputNameAllocated(0, allocator_);
        auto output_name_alloc = session_.GetOutputNameAllocated(0, allocator_);

        input_name_ = input_name_alloc.get();
        output_name_ = output_name_alloc.get();

        input_names_.push_back(input_name_.c_str());
        output_names_.push_back(output_name_.c_str());
    }

    int predict(const std::vector<float>& input) {
        if (input.size() != 28 * 28) {
            throw std::runtime_error("input size must be 784");
        }

        std::array<int64_t, 4> shape{1, 1, 28, 28};

        Ort::MemoryInfo memory_info =
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info,
            const_cast<float*>(input.data()),
            input.size(),
            shape.data(),
            shape.size());

        auto output_tensors = session_.Run(
            Ort::RunOptions{nullptr},
            input_names_.data(),
            &input_tensor,
            1,
            output_names_.data(),
            1);

        float* scores = output_tensors[0].GetTensorMutableData<float>();

        int best_idx = 0;
        for (int i = 1; i < 10; ++i) {
            if (scores[i] > scores[best_idx]) {
                best_idx = i;
            }
        }
        return best_idx;
    }

private:
    Ort::Env env_;
    Ort::Session session_;
    Ort::AllocatorWithDefaultOptions allocator_;

    std::string input_name_;
    std::string output_name_;
    std::vector<const char*> input_names_;
    std::vector<const char*> output_names_;
};

std::vector<float> parse_csv_784(const std::string& body) {
    std::vector<float> values;
    values.reserve(784);

    std::stringstream ss(body);
    std::string token;

    while (std::getline(ss, token, ',')) {
        if (!token.empty()) {
            values.push_back(std::stof(token));
        }
    }

    if (values.size() != 784) {
        throw std::runtime_error("request body must contain 784 comma-separated float values");
    }

    return values;
}

int main() {
    try {
        ThreadPool pool(4);
        MnistOnnxRuntime runtime("assets/mnist.onnx");
        Metrics metrics;

        httplib::Server server;

        server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("{\"status\":\"ok\"}", "application/json");
        });

        server.Get("/metrics", [&metrics](const httplib::Request&, httplib::Response& res) {
            long long count = metrics.total_requests.load();
            long long total_ms = metrics.total_inference_ms.load();
            double avg_ms = (count == 0) ? 0.0 : static_cast<double>(total_ms) / count;

            std::ostringstream oss;
            oss << "{"
                << "\"total_requests\":" << count << ","
                << "\"total_inference_ms\":" << total_ms << ","
                << "\"avg_inference_ms\":" << avg_ms
                << "}";

            res.set_content(oss.str(), "application/json");
        });

        server.Post("/predict", [&pool, &runtime, &metrics](const httplib::Request& req,
                                                            httplib::Response& res) {
            try {
                std::vector<float> input = parse_csv_784(req.body);
                auto start = Clock::now();

                auto fut = pool.submit([&runtime, input]() {
                    return runtime.predict(input);
                });

                int prediction = fut.get();

                auto end = Clock::now();
                auto elapsed_ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                metrics.total_requests.fetch_add(1);
                metrics.total_inference_ms.fetch_add(elapsed_ms);

                std::ostringstream oss;
                oss << "{"
                    << "\"prediction\":" << prediction << ","
                    << "\"inference_ms\":" << elapsed_ms
                    << "}";

                res.set_content(oss.str(), "application/json");
            } catch (const std::exception& e) {
                res.status = 400;

                std::ostringstream oss;
                oss << "{"
                    << "\"error\":\"" << e.what() << "\""
                    << "}";

                res.set_content(oss.str(), "application/json");
            }
        });

        std::cout << "Starting inference server...\n";

        if (!server.listen("127.0.0.1", 8081)) {
            std::cerr << "Failed to start server on 127.0.0.1:8081\n";
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}