# CPU scheduling simulator 개발 과정

## 2026 - 05 - 07
프로세스를 랜덤생성하는 로직을 짜보고자 한다.

프로세스의 구성은 이렇다고 가정한다.

1. 프로세스는 CPU Burst와 I/O Burst가 번갈아가면서 일어난다고 가정한다. 이 때, Ready Queue에 대기하고 있는 프로세스는 CPU Burst를 하기 위해서 Ready Queue에 대기하고 있는 것이기 때문에 시뮬레이터가 Ready Queue에 저장하기 위해 무작위로 생성하는 프로세스는 항상 I/O Burst가 아닌 CPU Burst를 먼저 진행하도록 설계되어야 한다.

2. I/O Burst는 running 상태인 프로세스가 I/O Device의 처리를 하고 그 처리의 결과를 받아서 CPU에 다시 돌아와서 처리하여야 하므로, I/O Burst가 발생하면 항상 그 뒤에는 CPU Burst도 발생하여야 한다. 그러므로 모든 프로세스의 마지막은 CPU Burst로 마무리 되어야 한다.

3. 1번과 2번 조건으로 인하여 모든 프로세스는 CPU(시작) -> I/O -> ... -> I/O -> CPU(끝) 의 순서로 Burst가 발생하기 때문에, 홀수 개(`2*k + 1, k>=0 인 정수`)의 Burst 구간이 발생한다.

4. 각 CPU Burst 구간은 평소엔 짧게 (`1~10 ms`) 어떨 땐 매우 길게 (`90 ~ 150 ms`) I/O Burst 구간은 길게 (`50~100 ms`) 발생한다고 가정한다.


## 2026 - 05 - 11
프로세스를 본격적으로 생성하기 전 미리 프로세스 생성 데이터를 마련하는 코드를 작성했다.

우선 define 커맨드를 통해 프로세스 생성과 관련된 하이퍼파라미터를 미리 선언한다.
```
#define max_Process_count 20
#define min_pid 1000
#define max_pid 3000
#define max_k 3 // max_burst_interval_size is equal to 2 * max_k + 1
#define max_arrival 500
#define CPU_min 1
#define CPU_max 10
#define long_CPU_min 90
#define long_CPU_max 150
#define IO_min 50
#define IO_max 100
#define max_priority 20

#define max_time_unit 2000
```

process_random_generator() 함수를 선언하여 위의 하이퍼파라미터 범위 내에서 프로세스 생성 데이터를 뽑아내는 로직을 구성하고 PCB에 저장시켜 놓았다.

## 2026 - 05 - 21
다른 과제하다 보니 어느새 10일 남짓, 계속 달려야 한다. 힘내보자.

uniform_dist, geeometric_dist를 새로 정의하여 이산확률분포를 통해 랜덤변수를 뽑는 방식으로 변경했다.

파일을 여러개 분할하여 저장하였고, Makefile 파일을 통해 컴파일 및 링크를 위한 환경을 마련했다.