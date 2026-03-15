# High Performance ThreadPool Runtime for AI Inference

C++로 고정 worker 기반 ThreadPool Runtime을 직접 설계하고 구현한 프로젝트입니다.
단순 병렬 처리 구현을 넘어, 실제 AI inference workload에 적용하여 CPU 환경에서의 처리 효율 개선 가능성을 실험적으로 검증했습니다.

본 프로젝트는 AI 모델을 만드는 것이 아니라, AI 모델을 더 효율적으로 실행하기 위한 runtime 설계에 초점을 둡니다.

---

# Project Goal

AI inference를 CPU 환경에서 실행할 때 발생하는

낮은 코어 활용률, 스레드 생성 비용, 순차 실행으로 인한 성능 저하 문제를 해결하기 위해

Task 단위 병렬 실행 구조를 설계하는 것을 목표로 했습니다.

---

## 1️⃣ High Performance ThreadPool Runtime

설계 및 구현

AI inference 실행 성능을 개선하기 위해
고성능 ThreadPool 기반 runtime 구조를 직접 설계하고 구현했습니다.

* Fixed worker thread 기반 실행 구조 설계
* Shared task queue 기반 task scheduling 구현
* Condition variable 기반 대기/깨움 구조 설계
* Thread lifecycle 관리 및 graceful shutdown 구현
* Future 기반 비동기 결과 처리 구조 설계

이를 통해 단순한 ThreadPool 구현이 아닌,
AI inference 실행 환경을 고려한 runtime 수준의 병렬 처리 구조를 구축했습니다.

---

## 2️⃣ Execution Model Benchmark

3가지 execution model 비교 실험

* Single Thread
* Thread per Task
* ThreadPool

이를 통해

* task granularity 영향
* scheduling overhead 영향
* worker scaling 특성

을 분석했습니다.

---

## 3️⃣ AI Inference Runtime Extension

ThreadPool runtime을

실제 AI inference workload에 적용

* ONNX Runtime 기반 inference 실행
* batch inference 병렬 처리
* single vs parallel inference 비교

👉 AI runtime 관점 실험

---

### Runtime Benchmark Insight

* tiny task에서는 thread creation cost가 bottleneck
* threadpool은 scheduling overhead를 줄임
* worker scaling은 거의 linear하게 성능 향상

### AI Inference Result Example

Batch 1000 기준:

* Single-thread inference → 46 ms
* ThreadPool inference → 14 ms

👉 CPU batch inference throughput 개선 확인

---

# Key Insight

이 프로젝트의 핵심은 다음입니다.

* ThreadPool은 latency 개선보다 throughput 개선에 효과적
* runtime 설계는 task granularity를 반드시 고려해야 함
* AI inference도 runtime 구조에 따라 성능이 크게 달라짐

---

# Project Structure

```text
.
├── include/
│   └── thread_pool.hpp
├── src/
│   ├── main.cpp
│   ├── bench.cpp
│   └── onnx_mnist_demo.cpp
├── assets/
│   └── mnist.onnx
└── README.md
```

---
