# SmartFan
객체 추적 기반 스마트 선풍기

- 2022년도 2학기 임베디드시스템 1분반 1조 Apple Pi 팀
- 20200252 김원렬, 20200370 김혜진, 20200573 서준혁

### 목차
[1.기능](#기능) <br/> 
[2.모듈](#모듈) <br/>
[3.전체 시스템 구조](#전체-시스템-구조) <br/>
[4.역할 분담](#역할-분담) <br/>
[5.개발 일정](#개발-일정) <br/>
[6.데모 영상](#데모-영상)

<br/>

## 기능
#### 본체 회전 모드 설정
- 사용자로부터 회전 모드를 입력받고, 입력 받은 값에 따라 다음 모드로 전환한다.
- 일반 회전 모드: 정해진 구간을 반복하여 회전한다.
- 객체 유도 모드: 사용자를 객체로 인식하여 객체를 따라가면서 회전한다.
- 고정 모드: 선풍기 머리를 고정하여 회전하지 않는다.
#### 타이머 설정
- 사용자로부터 시간을 입력받는다. (1분 ~ 60분)
- 입력 받은 시간이 지나면 자동으로 본체 회전과 팬 회전을 종료한다.
#### 바람 세기 설정
- 사용자로부터 바람 세기를 입력받는다. 
- 입력 받은 값에 따라 팬의 회전 세기를 변경한다.
#### 스마트폰 통신
- 모든 통신은 스마트폰과 라즈베리파이를 블루투스로 연결하여 명령을 입력받는다.
#### 프로세스, 쓰레드 간 통신
- 서버 프로세스
  - 스마트폰으로부터 값을 입력받기 위해 대기한다.
  - 값이 들어오면, POSIX 메시지 큐를 이용하여 모드에 맞는 클라이언트 프로세스에게 메시지를 전달한다.
- 클라이언트 프로세스
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
- 추후 회로도 그림, 실물 회로 이미지 업로드 예정

<br/>

## 통신 명세
- 통신은 `m1`, `s3`, `t30`과 같이 알파벳+숫자를 붙여 입력한다.
  - `m1`: 본체 회전 모드를 일반 회전 모드로 변경
  - `s3`: 팬 회전 강도를 강풍으로 변경
  - `t30`: 타이머 30분 설정
- 알파벳 m + 숫자: 본체 회전 모드 변경. (0-고정, 1-일반 회전, 2-객체 유도)
- 알파벳 s + 숫자: 팬 회전 강도 변경. (0-종료, 1-미풍, 2-약풍, 3-강풍)
- 알파벳 t + 숫자: 타이머 설정. (1분~60분)
- 유효하지 않은 값이 입력되면 무시한다.

<br/>

## 역할 분담
| 학번 | 이름 | 역할 |
|--------|-----|-------------------------------------------------------------|
|20200252|김원렬|📝회의 내용 기록<br/>🔡회로 구성<br/>👨‍💻객체 탐지 기능 구현<br/>🙋🏻‍♂구현 내용 시연|
|20200370|김혜진|👩‍👦‍👦프로젝트 총괄, 회의 진행 독려<br/>👩‍💻회전, 세기 조정 기능 구현<br/>📝보고서 작성|
|20200573|서준혁|🔡회로 구성<br/>👨‍💻통신 기능, 멀티프로세싱 구현<br/>🙋🏻‍♂최종 발표|

<br/>

## 개발 일정
| 기간 | 기능 | 진행 |
|------|-----|------|
|11/28~12/4|제작 계획 수립, 프로젝트 기획|✔|
|12/5~12/9|회로 구성, 통신 인터페이스 명세, 회전/세기 조정 기능 구현|✔|
|12/10~12/13|객체 탐지 기능 구현, 통신 시도||
|12/13~12/15|통신 및 테스트, 디버깅||
|12/16~12/20|보고서 작성||
|12/16, 12/20|결과 및 최종 데모 발표||

<br/>

## 데모 영상
- 추후 유튜브 링크 삽입 예정
