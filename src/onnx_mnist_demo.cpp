#include <iostream>
#include <vector>
#include <chrono>
#include "thread_pool.hpp"
#include <onnxruntime_cxx_api.h>

using Clock = std::chrono::high_resolution_clock;

std::vector<float> dummy_image() {
    return std::vector<float>(784, 0.1f);
}

int infer(Ort::Session& session, const std::vector<float>& input) {
    Ort::AllocatorWithDefaultOptions allocator;

    std::array<int64_t, 4> shape{1, 1, 28, 28};

    Ort::Value tensor = Ort::Value::CreateTensor<float>(
        allocator.GetInfo(),
        const_cast<float*>(input.data()),
        input.size(),
        shape.data(),
        shape.size()
    );

    auto input_name = session.GetInputNameAllocated(0, allocator);
    auto output_name = session.GetOutputNameAllocated(0, allocator);

    const char* input_names[] = { input_name.get() };
    const char* output_names[] = { output_name.get() };

    auto output = session.Run(
        Ort::RunOptions{nullptr},
        input_names,
        &tensor,
        1,
        output_names,
        1
    );

    float* scores = output[0].GetTensorMutableData<float>();

    int best = 0;
    for (int i = 1; i < 10; ++i) {
        if (scores[i] > scores[best]) best = i;
    }

    return best;
}

int main() {
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "mnist");
    Ort::SessionOptions opts;
    opts.SetIntraOpNumThreads(1);

    Ort::Session session(env, "assets/mnist.onnx", opts);
    for (int N : {50, 100, 500, 1000}) {
        std::vector<std::vector<float>> images(N, dummy_image());

        auto t1 = Clock::now();
        for (auto& img : images)
            infer(session, img);
        auto t2 = Clock::now();

        ThreadPool pool(4);

        auto t3 = Clock::now();
        std::vector<std::future<int>> futs;
        for (auto& img : images)
            futs.push_back(pool.submit(infer, std::ref(session), img));
        for (auto& f : futs) f.get();
        auto t4 = Clock::now();

        std::cout << "Batch " << N << '\n';
        std::cout << "Single inference: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
              << " ms\n";

        std::cout << "ThreadPool inference: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count()
              << " ms\n";
    }
}