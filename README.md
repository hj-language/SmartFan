# SmartFan
객체 추적 기반 스마트 선풍기

- 2022년도 2학기 임베디드시스템 1분반 1조 Apple Pi 팀
- 20200252 김원렬, 20200370 김혜진, 20200573 서준혁

<br/>

### 목차
[0.실행 방법](#실행-방법) <br/>
[1.기능](#기능) <br/> 
[2.모듈](#모듈) <br/>
[3.전체 시스템 구조](#전체-시스템-구조) <br/>
[4.역할 분담](#역할-분담) <br/>
[5.개발 일정](#개발-일정) <br/>
[6.문제점 및 해결 방안](#문제점-및-해결-방안) <br/>
[7.데모 영상](#데모-영상)

<br/>

## 실행 방법
- 컴파일
```bash
$ gcc stepmotor.c -o stepmotor -lwiringPi -lpthread -lrt
$ gcc dcmotor.c -o dcmotor -lwiringPi -lrt
$ gcc timer.c -o timer -lwiringPi -lpthread -lrt
$ gcc server.c -o server -lwiringPi -lpthread -lrt
```
- 실행
```bash
$ ./stepmotor &
$ sudo ./dcmotor &
$ sudo ./timer &
$ sudo ./server
```
- 실행 중인 모든 프로세스를 종료하고 싶다면 블루투스 통신으로 `q`를 입력한다.
- stepmotor 프로세스 실행 시 segmentation fault가 발생한다면 stepmotor_degree.txt 파일이 존재하는지 확인한다.

<br/>

## 기능
#### 본체 회전 모드 설정
- 사용자로부터 회전 모드를 입력받고, 입력 받은 값에 따라 다음 모드로 전환한다.
- 일반 회전 모드: 정해진 구간을 반복하여 회전한다.
- 객체 탐지 모드: 사용자를 객체로 인식하여 객체를 따라가면서 회전한다.
- 고정 모드: 선풍기 머리를 고정하여 회전하지 않는다.
#### 타이머 설정
- 사용자로부터 시간을 입력받는다.
- 입력 받은 시간이 지나면 자동으로 본체 회전과 팬 회전을 종료한다.
#### 바람 세기 설정
- 사용자로부터 바람 세기를 입력받는다. 
- 입력 받은 값에 따라 팬의 회전 세기를 변경한다.
#### 스위치 제어
- 회전 모드, 바람 세기를 스위치로 조절할 수 있다.
#### 스마트폰 통신
- 모든 통신은 스마트폰과 라즈베리파이를 블루투스로 연결하여 명령을 입력받는다.
#### 프로세스, 쓰레드 간 통신
- 서버 프로세스(server)
  - 스마트폰이나 스위치로부터 값을 입력받기 위해 대기한다.
  - 값이 들어오면, POSIX 메시지 큐를 이용하여 모드에 맞는 클라이언트 프로세스에게 메시지를 전달한다.
- 클라이언트 프로세스(stepmotor, dcmotor, timer)
  - 서버 프로세스로부터 값을 전달받기 위해 대기한다.
  - 전달받은 값에 맞는 기능을 실행한다.

<br/>

## 모듈
- Fan: Keyes140C04 모듈[L9110 모터 드라이버+DC 모터]
- 선풍기 본체 회전: 스텝 모터(28BYJ-48)+스텝 모터 드라이버(ULN2003A)
- 시간 계산: RTC 칩(DS3231)
- 객체 탐지: 초음파 센서(HC-SR04)
- 통신: UART 블루투스 모듈(HC-06)
- 3단계 세기 표현: LED 3개

<br/>

## 전체 시스템 구조
- 하드웨어 구조

  <img src="https://github.com/hj-language/SmartFan/blob/main/%ED%9A%8C%EB%A1%9C%EB%8F%84.png" alt="회로도" style="width:50%;" />
  
- 프로그램 구조
  
  <img src="https://github.com/hj-language/SmartFan/blob/main/%EC%A0%84%EC%B2%B4%EC%8B%9C%EC%8A%A4%ED%85%9C%EA%B5%AC%EC%A1%B0.png" alt="회로도" style="width:70%;" />
  
  - server 프로세스
    - 메인 쓰레드: 스마트폰에서 블루투스를 통해 전달되는 값을 읽어 해당하는 클라이언트 프로세스에게 메시지 전송
    - 스위치 제어 쓰레드: 스위치 입력 값이 바뀔 때 해당하는 클라이언트 프로세스에게 메시지 전송
  - timer 프로세스
    - 메인 쓰레드: server로부터 메시지를 받으면 시간을 초기화한 후 시간 계산 쓰레드 실행
    - 시간 계산 쓰레드: 5초마다 현재 시간을 읽어 정해진 시간이 지났는지 확인하고, <br/>지났다면 stepmotor에게는 고정 모드 메시지를, dcmotor에게는 세기 0 메시지를 전송
  - stepmotor 프로세스
    - 메인 쓰레드: server나 timer에게 메시지를 받으면 기존 실행중인 stepmotor의 쓰레드 종료 후 해당하는 모드의 쓰레드 실행
    - 자동 모드 쓰레드: 회전 모드가 바뀌기 전까지 30도~150도 사이로 무한 회전
    - 유도 모드 쓰레드: 거리 계산 쓰레드를 실행하고, 회전 모드가 바뀌기 전까지 객체를 찾아 객체를 따라가게끔 회전. 
    - 거리 계산 쓰레드: 초음파 센서로부터 거리 값을 읽어 공유변수에 저장
  - dcmotor 프로세스
    - 메인 쓰레드: server나 timer에게 메시지를 받으면 해당 세기로 회전
    
- 예상 형상도
  <div>
    <img src="https://github.com/hj-language/SmartFan/blob/main/%EB%AA%A8%EB%8D%B8%EB%A7%81%EB%8F%84%EC%95%88.png" alt="도안1" style="width:30%;" />
    <img src="https://github.com/hj-language/SmartFan/blob/main/%EB%AA%A8%EB%8D%B8%EB%A7%81%EB%8F%84%EC%95%882.png" alt="도안2" style="width:50%;float:left;" />
  </div>

  - 3D CAD를 이용하여 만든 형상도이다.
  - 위 형상을 이용하면 DC 모터가 초음파 센서에 미치는 영향도 없으며 팬을 보호할 수 있어 안전성도 높일 수 있을 것이다.

<br/>

## 통신 명세
- 통신은 `m1`, `s3`, `t30`과 같이 알파벳+숫자를 붙여 입력한다.
  - `m1`: 본체 회전 모드를 일반 회전 모드로 변경
  - `s3`: 팬 회전 강도를 강풍으로 변경
  - `t30`: 타이머 30분 설정
- 알파벳 m + 숫자: 본체 회전 모드 변경. (0-고정, 1-일반 회전, 2-객체 유도)
- 알파벳 s + 숫자: 팬 회전 강도 변경. (0-종료, 1-미풍, 2-약풍, 3-강풍)
- 알파벳 t + 숫자: 타이머 설정. (1분~300분)
- 유효하지 않은 값이 입력되면 무시한다.

<br/>

## 역할 분담
| 학번 | 이름 | 역할 |
|--------|-----|-------------------------------------------------------------|
|20200252|김원렬|🔡회로 구성<br/>👨‍💻객체 탐지 기능, 팬 회전 기능 구현<br/>🙋🏻‍♂구현 내용 시연|
|20200370|김혜진|👩‍👦‍👦프로젝트 총괄, 회의 진행 독려<br/>👩‍💻몸체 회전 기능, 스위치 제어 기능 구현, 멀티프로세싱&멀티쓰레딩<br/>📝보고서 작성|
|20200573|서준혁|🔡회로 구성<br/>👨‍💻블루투스 통신 기능, 타이머 기능 구현<br/>🙋🏻‍♂최종 발표|

<br/>

## 개발 일정
| 예상 기간 | 실제 기간 | 기능 | 진행 |
|----------|-----------|------|-----|
|11/28~12/4|11/28~12/4|제작 계획 수립, 프로젝트 기획|✔|
|12/5~12/9|11/28~12/12|회로 구성, 통신 인터페이스 명세, 회전/세기 조정 기능 구현|✔|
|12/10~12/13|12/13~12/15|객체 탐지 기능 구현, 블루투스 통신 시도|✔|
|12/13~12/15|12/14~12/15|테스트 및 디버깅|✔|
|12/16~12/20|12/5~12/20|보고서(README) 작성|✔|
|12/16, 12/20|12/16, 12/20|결과 및 최종 데모 발표|✔|

<br/>

## 문제점 및 해결 방안
- 초음파 센서가 측정한 거리값 중 불규칙적으로 비정상값이 측정되는 문제
  - 초음파 센서가 측정한 거리값이 너무 작거나 크면 무시하도록 수정하였다.
- 서버 프로세스와 dcmotor 프로세스가 메시지 큐를 공유하지 못하는 문제
  - DC모터는 PWM을 이용해야 하므로 dcmotor 프로세스를 root 권한으로 실행하기 때문에, 메시지 큐를 만들 때 root 권한으로 생성한다.
  - 서버 프로세스가 일반 사용자 권한으로 실행되면 root 권한으로 만든 메시지 큐를 열지 못해 permission denied 에러가 발생했다.
  - 두 프로그램의 권한을 통일하여 해결했다.

<br/>

## 데모 영상
[![데모 영상](https://img.youtube.com/vi/-LbV7Xpw9Mo/0.jpg)](https://www.youtube.com/watch?v=-LbV7Xpw9Mo) <br/>
(클릭하면 유튜브 영상으로 이동)

<br/>

## 참고자료
- 초음파 센서의 이해 https://www.digikey.kr/ko/articles/understanding-ultrasonic-sensors
- 임베디드시스템 교안
