#include <stdio.h>
#include <unistd.h>
#include <mqueue.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

#define BAUD_RATE 115200
#define ROT_IN 7
#define STR_IN 8

/*
    서버는
    - switch로부터 입력 받기
    - 블루투스를 통해 스마트폰으로부터 입력 받기
    입력 받은 값은 POSIX 메시지 큐를 이용하여 
    클라이언트 프로세스에게 전송한다.
*/

static const char* UART2_DEV = "/dev/ttyAMA1";

/* 공유 변수 */
pthread_mutex_t mutex_strength = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_rotate = PTHREAD_MUTEX_INITIALIZER;
int isQuit = 0;
int strength = 0;
int rotate = 0;

/* 메시지 큐 관련 변수 */
mqd_t mq_timer, mq_rotate, mq_strength;
static const char* mq_timer_name = "/timer"; // 메시지 큐끼리 공유할 이름
static const char* mq_rotate_name = "/rotate";
static const char* mq_strength_name = "/strength";
struct mq_attr attr;
char buf[BUFSIZ];

/* UART 통신 관련 변수 */
int fd_serial;

/* 함수 선언 */
int init();
unsigned char serialRead(const int fd);
int serialReadBytes(const int fd);
void serialWrite(const int fd, const unsigned char c);
void serialWriteBytes(const int fd, const char* s);
// 스위치 값을 받는 함수
void *InputFromSwitch();
// 클라이언트에게 메시지 큐를 통해 데이터를 전송하는 함수
void SendToClient(int value);
int catchRangeException(char mode, int value);

// main함수에서는 switch, 스마트폰으로부터 입력받는 쓰레드를 만들어 실행한다.
void main(void) {
    if (init() == -1) return;
    
    int readValue, mode;
    char buf[BUFSIZ];

    // switch 제어 쓰레드 생성 및 실행
    pthread_t switchThread;
    int threadReturnValue = pthread_create(&switchThread, NULL, InputFromSwitch, NULL);
    if (threadReturnValue < 0) return;

    while (!isQuit) {
        if (serialDataAvail(fd_serial)) {
            mode = serialRead(fd_serial);
            if (mode == 'm' && serialDataAvail(fd_serial)) {
                // mode 인자 경우의 수 : 0 ~ 2 (1 Byte)
                readValue = serialRead(fd_serial) - '0';
                if (catchRangeException(mode, readValue)) continue;
                pthread_mutex_lock(&mutex_rotate);
                rotate = readValue;
                pthread_mutex_unlock(&mutex_rotate);
                sprintf(buf, "%d", readValue);
                mq_send(mq_rotate, buf, strlen(buf), 0);
            }
            else if (mode == 's' && serialDataAvail(fd_serial)) {
                // strength 인자 경우의 수 : 0 ~ 3 (1 Byte)
                readValue = serialRead(fd_serial) - '0';
                if (catchRangeException(mode, readValue)) continue;
                pthread_mutex_lock(&mutex_strength);
                strength = readValue;
                pthread_mutex_unlock(&mutex_strength);
                sprintf(buf, "%d", readValue);
                mq_send(mq_strength, buf, strlen(buf), 0);
            }
            else if (mode == 't' && serialDataAvail(fd_serial)) {
                // time 인자 경우의 수 : 0 ~ 300 (1 ~ 3 Bytes)
                readValue = serialReadBytes(fd_serial);
                if (catchRangeException(mode, readValue)) continue;
                sprintf(buf, "%d", readValue);
                mq_send(mq_timer, buf, strlen(buf), 0);
            }
            // 개발자 모드 - q가 들어오면 프로세스 종료
            else if (mode == 'q') {
                mq_send(mq_rotate, "9", 2, 0);
                mq_send(mq_strength, "9", 2, 0);
                mq_send(mq_timer, "999", 4, 0);
                isQuit = 1;
            }
            memset(buf, 0, sizeof(buf));
        }
        delay(10);
    }
    threadReturnValue = pthread_join(switchThread, NULL);
    pthread_mutex_destroy(&mutex_strength);
    pthread_mutex_destroy(&mutex_rotate);
    /* 메시지 큐 닫기 */
    mq_close(mq_timer);
    mq_close(mq_rotate);
    mq_close(mq_strength);
}

int init() {
    /* wiringPi setup */
    if (wiringPiSetupGpio() < 0) {
        printf("Unable to setup wiringPi.\n");
        return -1;
    }

    /* UART 통신 설정 */
    if ((fd_serial = serialOpen(UART2_DEV, BAUD_RATE)) < 0) {
      printf("Unable to open serial device.\n");
      return -1;
    }

    /* pin mode 설정 */
    pinMode(STR_IN, INPUT);
    pinMode(ROT_IN, INPUT);

    /* 메시지 큐 열기 */
    mq_timer = mq_open(mq_timer_name, O_WRONLY);
    mq_rotate = mq_open(mq_rotate_name, O_WRONLY);
    mq_strength = mq_open(mq_strength_name, O_WRONLY);
    printf("ERROR NO:%d\n", errno);
    return 1;
}

unsigned char serialRead(const int fd) {
    unsigned char x;

    if (read(fd, &x, 1) != 1)
        return -1;
      
    return x;
}

int serialReadBytes(const int fd) {
    unsigned char bytes[3];

    for (int i = 0; i < 3; i++)
        bytes[i] = -1;

    int size = 0;

    while (serialDataAvail(fd))
        bytes[size++] = serialRead(fd);

    return atoi(bytes);
}

void serialWrite(const int fd, const unsigned char c) {
   write(fd, &c, 1);
}

void serialWriteBytes(const int fd, const char* s) {
   write(fd, s, strlen(s));
}

void *InputFromSwitch() {
    int rot_val = 0;
    int str_val = 0;
    int str_flag = 0;
    int rot_flag = 0;
    char buf[BUFSIZ];

    while (!isQuit){
    	str_val = digitalRead(STR_IN);
    	if(str_val == LOW && str_flag == 0) str_flag = 1;
	
    	if (str_flag == 1) {
            pthread_mutex_lock(&mutex_strength);
            strength = (strength + 1) % 4;
            pthread_mutex_unlock(&mutex_strength);
            sprintf(buf, "%d", strength);
            mq_send(mq_strength, buf, strlen(buf), 0);
            str_flag = 0;
            delay(400);
    	}
    	
    	rot_val = digitalRead(ROT_IN);
        if (rot_flag == 0 && rot_val == LOW) rot_flag = 1;
    	
    	if (rot_flag == 1) {
            pthread_mutex_lock(&mutex_rotate);
            rotate = (rotate + 1) % 3;
            pthread_mutex_unlock(&mutex_rotate);
    	    sprintf(buf, "%d", rotate);
            mq_send(mq_rotate, buf, strlen(buf), 0);
            rot_flag = 0;
            delay(400);
    	}
    }
}

int catchRangeException(char mode, int value) {
    // 1 (예외) 또는 0 (정상) 리턴
    switch (mode) {
        case 'm':
            if (value < 0 || value > 2 || value == 9) return 1;
            else return 0;
            break;
        case 's':
            if (value < 0 || value > 3 || value == 9) return 1;
            else return 0;
            break;
        case 't':
            if (value < 0 || value > 300 || value == 999) return 1;
            else return 0;
            break;
        default:
            return 1;
    }
}
