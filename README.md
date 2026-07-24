# CPU Scheduling Simulator

C로 구현한 tick 기반 CPU 스케줄링 시뮬레이터입니다. 무작위로 생성한 CPU-bound/IO-bound 프로세스를 대상으로 단일·다중 CPU와 여러 Ready Queue 구성을 조합해 실행할 수 있습니다.

시뮬레이션이 끝나면 프로세스별 상태 체류 시간, CPU 사용률, CPU 간트 차트를 터미널에 출력하고 CPU/IO 자원의 전체 결과를 `result.txt`에 저장합니다. 같은 프로세스 집합에 여러 스케줄링 정책을 적용해 결과를 비교할 수도 있습니다.

## 주요 기능

### 1. 프로세스 무작위 생성

실행할 때마다 3~5개의 프로세스를 생성합니다.

- PID: 1000~9999에서 중복 없이 선택
- 도착 시간: 0~5 tick
- 우선순위: 0~9
  - 숫자가 작을수록 높은 우선순위입니다.
- 프로세스 유형: CPU-bound 70%, IO-bound 30% 확률
- Burst 구조: `CPU -> IO -> CPU -> ... -> CPU`
  - 항상 CPU Burst로 시작하고 끝납니다.
  - IO Burst마다 IO 장치 0~4 중 하나를 무작위로 사용합니다.

프로세스 유형별 기본 생성 범위는 다음과 같습니다.

| 유형 | CPU Burst | IO Burst | IO Cycle 수 |
| --- | ---: | ---: | ---: |
| CPU-bound | 5~10 tick | 5~10 tick | 0~2회 |
| IO-bound | 3~8 tick | 8~13 tick | 1~3회 |

생성된 프로세스 정보는 PID, 도착 시간, 우선순위, 유형, Burst 순서와 함께 터미널에 표시됩니다. `IO시간(IO 장치 번호)` 형식으로 출력되므로, 예를 들어 `10(2)`는 IO 장치 2에서 10 tick 동안 실행되는 Burst를 뜻합니다.

### 2. CPU 및 Ready Queue 구성

다섯 가지 실행 환경을 지원합니다.

| 번호 | 구성 | CPU | Ready Queue | 특징 |
| ---: | --- | ---: | ---: | --- |
| 0 | Single Core / Single Queue | 1 | 1 | 하나의 정책을 선택 |
| 1 | Single Core / Multilevel Queue | 1 | 5 | 우선순위별 고정 tier와 내장 정책 사용 |
| 2 | Single Core / Multilevel Feedback Queue | 1 | 5 | MLQ에 aging과 하위 tier 이동을 추가 |
| 3 | Multi Core / Common Queue | 5 | 1 | 모든 CPU가 하나의 Ready Queue를 공유 |
| 4 | Multi Core / Own Queue | 5 | 5 | CPU별 정책과 Ready Queue를 개별 설정 |

멀티코어 개별 큐 구성에서는 새 프로세스와 IO 완료 프로세스를 누적 실행 시간이 가장 짧은 CPU의 큐에 배정합니다. Time Quantum 만료로 돌아온 프로세스는 기존 CPU의 Ready Queue로 복귀합니다.

### 3. 스케줄링 알고리즘

Single Queue, Common Queue, Own Queue 구성에서 다음 정책을 선택할 수 있습니다. Own Queue에서는 CPU별로 각각 다른 정책을 지정할 수 있습니다.

| 번호 | 알고리즘 | 동작 |
| ---: | --- | --- |
| 0 | FCFS | Ready Queue 도착 시간이 빠른 순서로 실행 |
| 1 | SJF | 남은 CPU Burst가 짧은 순서로 실행 |
| 2 | Priority | 우선순위 값이 작은 프로세스부터 실행 |
| 3 | Round Robin | FCFS 순서로 실행하며 Time Quantum 만료 시 재대기 |
| 4 | Preemptive SJF | 더 짧은 CPU Burst를 가진 프로세스가 준비되면 선점 |
| 5 | Preemptive Priority | 더 높은 우선순위의 프로세스가 준비되면 선점 |
| 6 | Priority + Aging | 대기 중인 프로세스의 우선순위를 주기적으로 높임 |
| 7 | Preemptive Priority + Aging | Aging을 적용하고 더 높은 우선순위가 준비되면 선점 |

동률인 경우 PID가 작은 프로세스를 먼저 선택합니다. Round Robin은 같은 시각에 Ready Queue로 들어온 프로세스 중 새로 도착하거나 IO를 마친 프로세스를 CPU에서 반환된 프로세스보다 먼저 배치합니다.

- Round Robin Time Quantum 입력 범위: 1~20 tick
- Aging Interval 입력 범위: 5~20 tick
- Aging 동작: 지정한 간격만큼 Ready Queue에서 대기할 때마다 우선순위 값을 1 감소

### 4. Multilevel Queue

우선순위 범위 0~9를 2개씩 나누어 5개 tier로 구성합니다. tier 번호가 작을수록 먼저 스케줄링됩니다.

| Tier | 우선순위 | 큐 내부 정책 | Time Quantum |
| ---: | --- | --- | ---: |
| 0 | 0~1 | Round Robin + FCFS | 2 |
| 1 | 2~3 | Round Robin + SJF | 4 |
| 2 | 4~5 | Round Robin + SJF | 8 |
| 3 | 6~7 | FCFS | 없음 |
| 4 | 8~9 | FCFS | 없음 |

상위 tier에 실행 가능한 프로세스가 있으면 하위 tier보다 먼저 선택합니다. Round Robin tier에서는 Time Quantum을 사용하며, MLQ에서는 프로세스가 원래 우선순위의 tier를 유지합니다.

### 5. Multilevel Feedback Queue

MLQ와 동일한 5개 tier 및 큐 내부 정책을 사용하면서 다음 피드백 동작을 추가합니다.

- Time Quantum을 모두 사용한 프로세스를 한 단계 낮은 tier로 이동
- Ready Queue에서 10 tick을 대기할 때마다 우선순위 값을 1 감소
- Aging 결과 우선순위 구간이 바뀌면 상위 tier로 이동
- 마지막 tier는 FCFS로 실행되며 Time Quantum을 사용하지 않음

### 6. CPU/IO Burst 및 상태 전이

시뮬레이터는 최대 10,000 tick 동안 각 프로세스와 자원의 상태를 기록합니다.

```text
NOT_GENERATED
      |
      | 도착 시간
      v
    READY <---------- IO_RUNNING
      |                  ^
      | CPU 배정         | IO 장치 배정
      v                  |
 CPU_RUNNING -------> WAITING
      |
      | 마지막 CPU Burst 완료
      v
 TERMINATED
```

- CPU Burst가 끝난 프로세스는 해당 IO 장치의 Waiting Queue로 이동합니다.
- 각 IO 장치는 하나의 프로세스만 실행하며, Waiting Queue는 FCFS 방식으로 처리됩니다.
- IO Burst 완료 후 프로세스는 Ready Queue로 돌아갑니다.
- 선점되거나 Time Quantum이 끝난 프로세스도 남은 CPU Burst를 유지한 채 Ready Queue로 돌아갑니다.
- 모든 프로세스가 종료되면 최대 시간에 도달하기 전이라도 시뮬레이션을 끝냅니다.

## 빌드 및 실행

### 요구 사항

- GCC
- GNU Make
- POSIX 환경(Linux 등)

`clock_gettime()`과 `<unistd.h>`를 사용하므로 현재 구현은 POSIX 환경을 기준으로 합니다.

### 빌드

```bash
make
```

생성되는 실행 파일:

```text
cpu_scheduling_simulator
```

### 실행

```bash
./cpu_scheduling_simulator
```

프로그램 실행 순서는 다음과 같습니다.

1. 무작위 프로세스 집합 생성
2. CPU/Ready Queue 구성 선택
3. 필요한 경우 알고리즘, Time Quantum, Aging Interval 입력
4. 시뮬레이션 결과 확인
5. 같은 프로세스에 다른 알고리즘을 적용하거나 새 프로세스 집합 생성

모든 숫자 입력은 허용 범위를 검사하며, 숫자가 아니거나 범위를 벗어난 값은 다시 입력받습니다.

### 정리

오브젝트 파일만 삭제:

```bash
make obj_clean
```

오브젝트 파일과 실행 파일 삭제:

```bash
make clean
```

## 출력 결과

### 터미널

터미널에는 ANSI 색상을 사용해 다음 내용을 출력합니다.

- 생성된 PCB 정보
- 프로세스별 CPU 실행 시간
- 프로세스별 IO 실행 시간
- Ready Queue 대기 시간
- IO Waiting Queue 대기 시간
- 각 항목의 프로세스당 평균
- CPU별 사용률
- CPU별 간트 차트와 IDLE 구간

CPU 사용률은 해당 CPU의 실행 tick을 전체 시뮬레이션 시간으로 나눈 값입니다.

### `result.txt`

각 시뮬레이션이 끝나면 프로젝트 루트의 `result.txt`에 다음 내용을 저장합니다.

- 생성된 프로세스 정보와 전체 Burst
- 프로세스별/평균 상태 시간
- CPU별 사용률과 간트 차트
- IO 장치별 사용률과 간트 차트

`result.txt`는 시뮬레이션할 때마다 새로 작성되므로 이전 결과는 덮어써집니다.

## 설정 변경

주요 상수는 `config.h`에서 변경할 수 있습니다.

| 상수 | 기본값 | 의미 |
| --- | ---: | --- |
| `MIN_PROCESS_COUNT` / `MAX_PROCESS_COUNT` | 3 / 5 | 생성할 프로세스 수 |
| `MAX_PRIORITY` | 10 | 우선순위 단계 수 |
| `MIN_ARRIVAL` / `MAX_ARRIVAL` | 0 / 5 | 도착 시간 범위 |
| `PORTION_OF_CPU_BOUND` | 70 | CPU-bound 생성 비율(%) |
| `MAX_CPU_COUNT` | 5 | 멀티코어 CPU 수 |
| `MAX_TIER_COUNT` | 5 | MLQ/MLFQ tier 수 |
| `MAX_IO_WQ_COUNT` | 5 | IO 장치와 Waiting Queue 수 |
| `AGING_INTERVAL` | 10 | MLFQ 기본 aging 간격 |
| `MAX_TIME_LINE` | 10000 | 최대 시뮬레이션 tick |

CPU-bound/IO-bound 프로세스의 CPU·IO Burst 및 IO Cycle 범위도 같은 파일에서 조정할 수 있습니다.

## 파일 구성

| 파일 | 역할 |
| --- | --- |
| `main.c` | 입력 검증, 메뉴, 반복 실행, 결과 파일 생성 |
| `config.c`, `config.h` | 자료구조와 상수, 프로세스 생성, 실행 환경 초기화 |
| `simulation.c`, `simulation.h` | tick 진행, 상태 전이, 선점, 자원 및 큐 스케줄링 |
| `cmp_func.c`, `cmp_func.h` | FCFS, SJF, Priority 및 RR용 정렬 기준 |
| `graph.c`, `graph.h` | 통계 계산, CPU/IO 사용률과 간트 차트 출력 |
| `random_distribution.c`, `random_distribution.h` | 균등·기하 분포 난수 함수 |
| `mem_alloc.c`, `mem_alloc.h` | PCB, 프로세스, CPU, IO, Queue 메모리 할당·해제 |
| `color.h` | 터미널 ANSI 색상 코드 |
| `Makefile` | 빌드 및 정리 명령 |

## 구현 참고 사항

- 시간 단위는 실제 밀리초가 아닌 논리적인 `tick`입니다.
- 컨텍스트 스위칭 비용은 별도로 계산하지 않습니다.
- SJF와 Preemptive SJF는 다음 CPU Burst의 실제 길이를 이미 알고 있다고 가정합니다.
- 통계의 `READY_QUEUE`와 `WAIT_QUEUE`는 각각 Ready Queue 및 IO 장치 앞 Waiting Queue에서 보낸 시간입니다.
- 난수 seed는 단조 시계의 나노초 값을 사용하므로 실행할 때마다 프로세스 구성이 달라집니다.
- 현재 자동화된 테스트 코드는 포함되어 있지 않습니다.
