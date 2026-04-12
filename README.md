
# ThreadPool 기반 AI 추론 서버

## 프로젝트 개요

병렬 처리 구조를 단순히 구현하는 데서 끝나지 않고, 실제 의미 있는 작업에 적용하여 그 효과를 검증하고자 프로젝트를 진행했습니다.

특히 인공지능 모델 추론은 동일한 연산이 반복되는 구조이기 때문에, 작업 단위 병렬 실행 구조를 적용하기에 적합하다고 판단했습니다. 이에 따라 ThreadPool 기반 병렬 처리 구조를 직접 구현하고, 이를 ONNX 기반 AI 모델 추론 과정에 적용하여 성능 차이를 확인했습니다.

또한 내부 실행 코드 수준을 넘어, 외부 요청을 받아 처리하는 서버 형태로 확장하여 실제 서비스 환경과 유사한 구조를 구성했습니다.

---

## 프로젝트 목표

- ThreadPool 기반 병렬 실행 구조 설계 및 구현
- AI 모델 추론 과정에 병렬 처리 적용
- 실행 구조에 따른 성능 차이 검증
- 서버 형태로 확장하여 실제 사용 환경 구현

---

## 시스템 구조

```
Client Request (curl)
        ↓
HTTP Server (httplib)
        ↓
ThreadPool (작업 분배)
        ↓
ONNX Runtime (모델 추론)
        ↓
Response 반환 (JSON)
```

---

## 주요 기능

### 1. ThreadPool 구현

- 작업 큐 기반 병렬 실행 구조
- Worker Thread 재사용
- 조건 변수를 활용한 효율적인 동기화
- future를 통한 비동기 결과 처리

---

### 2. AI 모델 추론

- ONNX Runtime을 활용한 MNIST 모델 실행
- 입력 데이터 → 텐서 변환 → 모델 추론 → 결과 반환
- 가장 높은 값을 가지는 클래스 선택

---

### 3. HTTP 서버

- `/health` : 서버 상태 확인
- `/metrics` : 처리 요청 수 및 평균 실행 시간 확인
- `/predict` : 입력 데이터를 받아 AI 추론 수행

---

### 4. 성능 비교 실험

세 가지 실행 구조를 비교했습니다.

- 단일 실행 방식 (Single Thread)
- 작업마다 스레드 생성 방식 (Thread per Task)
- ThreadPool 기반 병렬 처리 방식

### 실험 결과

| Task | NumTasks | Single(ms) | Thread-per-task(ms) | ThreadPool(ms) |
| --- | --- | --- | --- | --- |
| Tiny | 1000 | 10 | 32 | 2 |
| Medium | 300 | 215 | 28 | 54 |
| Heavy | 50 | 358 | 54 | 93 |

### 분석

- 작은 작업이 많은 경우 → ThreadPool이 가장 효율적
- 작업이 무거운 경우 → 스레드 생성 비용 영향 감소
- 작업 크기에 따라 최적 실행 구조가 달라짐

---

## ⚙️ 실행 방법

### 1. 컴파일

```
g++ -std=c++17 -O2 -pthread src/inference_server.cpp \
-Iinclude \
-Ionnxruntime/include \
-Lonnxruntime/lib -lonnxruntime \
-o inference_server
```

---

### 2. 실행 (Mac)

```
export DYLD_LIBRARY_PATH=$(pwd)/onnxruntime/lib:$DYLD_LIBRARY_PATH
./inference_server
```

---

## 발생 문제 및 해결

- ONNX Runtime 연동 과정에서 라이브러리 경로 설정 문제 발생
    
    → 환경 변수 설정으로 해결
    
- 서버 실행 시 포트 충돌 문제 발생
    
    → 포트 변경으로 해결
    
- 병렬 구조를 실제 작업에 적용하는 과정에서 실행 흐름 이해 필요
    
    → 작업 단위 구조로 재설계
    

---

## 배운 점

- 병렬 처리는 단순히 스레드를 늘리는 것이 아니라, 작업 구조에 따라 효율이 달라짐
- 실행 구조와 실제 작업 특성 간의 관계를 고려해야 함
- 인공지능 모델은 단순 사용보다 실행 환경 설계가 중요함