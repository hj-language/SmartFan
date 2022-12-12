#include <stdio.h>
#include <unistd.h>
#include <mqueue.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringSerial.h>

#define BAUD_RATE 9600
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
int isQuit = 1;

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
unsigned char serialRead(const int fd);
void serialWrite(const int fd, const unsigned char c);
void serialWriteBytes(const int fd, const char* s);
// 스위치 값을 받는 함수
void InputFromSwitch();
// 클라이언트에게 메시지 큐를 통해 데이터를 전송하는 함수
void SendToClient(int value);

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
                readValue = serialRead(fd_serial);
                /////////////////// readValue에는 1~3 길이의 숫자가 들어오도록 해주세용
                /////////////////// 문자열을 buf에 써서 보낼 거임
                sprintf(buf, "%d", readValue);
                mq_send(mq_rotate, buf, strlen(buf), 0);
            }
            else if (mode == 's' && serialDataAvail(fd_serial)) {
                readValue = serialRead(fd_serial);
                sprintf(buf, "%d", readValue);
                mq_send(mq_strength, buf, strlen(buf), 0);
            }
            else if (mode == 't' && serialDataAvail(fd_serial)) {
                readValue = serialRead(fd_serial);
                sprintf(buf, "%d", readValue);
                mq_send(mq_timer, buf, strlen(buf), 0);
            }
            // 개발자 모드 - q가 들어오면 프로세스 종료
            else if (mode == 'q') {
                isQuit = 1;
                mq_send(mq_rotate, "999", 4, 0);
                mq_send(mq_strength, "999", 4, 0);
            }
        }
        delay(10);
    }
    threadReturnValue = pthread_join(switchThread, NULL);
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

    return 1;
}

unsigned char serialRead(const int fd)
{
   unsigned char x;

   if (read(fd, &x, 1) != 1)
   {
      return -1;
   }
      
   return x;
}

void serialWrite(const int fd, const unsigned char c)
{
   write(fd, &c, 1);
}

void serialWriteBytes(const int fd, const char* s)
{
   write(fd, s, strlen(s));
}

void InputFromSwitch() {
    int str_mode = 0;
    int rot_mode = 0;
    int rot_val = 0;
    int str_val = 0;
    int str_flag = 0;
    int rot_flag = 0;
    char buf[BUFSIZ];

    while (!isQuit){
    	str_val = digitalRead(STR_IN);
    	if(str_val == LOW && str_flag == 0) str_flag = 1;
	
    	if (str_flag == 1) {
    		str_mode = (str_mode + 1) % 4;
            sprintf(buf, "%d", str_mode);
            mq_send(mq_strength, buf, strlen(buf), 0);
            str_flag = 0;
            delay(200);
    	}
    	
    	rot_val = digitalRead(ROT_IN);
        if (rot_flag == 0 && rot_val == LOW) rot_flag = 1;
    	
    	if (rot_flag == 1) {
    	    rot_mode = (rot_mode + 1) % 3;
    	    sprintf(buf, "%d", rot_mode);
            mq_send(mq_rotate, buf, strlen(buf), 0);
            rot_flag = 0;
            delay(200);
    	}
    }
}