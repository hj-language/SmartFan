#include <stdio.h>
#include <unistd.h>
#include <mqueue.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <stdlib.h>

/*
    timer 클라이언트는
    - 서버로부터 데이터 받기
    입력 받은 값 만큼 타이머를 실행한다.
    종료 시간이 되면 POSIX 메시지 큐를 이용하여 
    세기 담당, 회전 담당 프로세스에게 전송한다.
*/

#define SLAVE_ADDR_01 0x68 
static const char* I2C_DEV = "/dev/i2c-1";
int i2c_fd;

/* 공유 변수 */
int isQuit;

/* 메시지 큐 관련 변수 */
mqd_t mq_timer, mq_strength, mq_rotate;
struct mq_attr attr;
const char* mq_timer_name = "/timer";  
const char* mq_strength_name = "/strength";
const char* mq_rotate_name = "/rotate";
char buf[BUFSIZ];
int n;

/* 함수 선언 */
int init();
int readReg(int, int);
void* setTimer(void *);
void timerInit();

void main() 
{
    if (init() == -1) return;

    int value;
    pthread_t thread = 0;
    int threadReturnValue;

    while (1) {
        n = mq_receive(mq_timer, buf, sizeof(buf), NULL);
        value = atoi(buf);
	printf("Timer: Receive from server value %d, %s\n", value, buf);

        if (value == 999) {
        	isQuit = 1;
		// 기존 쓰레드 종료
		if (thread != 0) threadReturnValue = pthread_join(thread, NULL); 
        	break;
	}
	
        // 기존 쓰레드 종료
        if (thread != 0) {
        	isQuit = 1;
        	threadReturnValue = pthread_join(thread, NULL);
        } 

        // 타이머 초기화
        timerInit();
    	isQuit = 0;

        // 새 쓰레드 실행
        threadReturnValue = pthread_create(&thread, NULL, setTimer, (void *)value);
    }

    mq_close(mq_timer);
    mq_close(mq_rotate);
    mq_close(mq_strength);
    mq_unlink("/timer");
}

int init() {
    /* wiringPi setup */
    if (wiringPiSetup() < 0) {
        printf("wiringPiSetup() is failed\n");
        return -1;
    }

    /* I2C 연결 설정 */
    i2c_fd = wiringPiI2CSetupInterface(I2C_DEV, SLAVE_ADDR_01);
    
    /* 메시지 큐 속성 초기화 */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = BUFSIZ;
    attr.mq_curmsgs = 0;
    mq_timer = mq_open(mq_timer_name, O_CREAT | O_RDONLY, 0644, &attr);
    mq_strength = mq_open(mq_strength_name, O_WRONLY);
    mq_rotate = mq_open(mq_rotate_name, O_WRONLY);
    return 1;
}

void timerInit() {
    wiringPiI2CWriteReg8(i2c_fd, 0x00, 0x00);
    wiringPiI2CWriteReg8(i2c_fd, 0x01, 0x00);
    wiringPiI2CWriteReg8(i2c_fd, 0x02, 0x00);
};

int readReg(int i2c_fd, int value) {
    int min = wiringPiI2CReadReg8(i2c_fd, 0x01);
    int hour = wiringPiI2CReadReg8(i2c_fd, 0x02);

    int target = hour * 60 + min;

    // 타이머 끝났을땐 1 리턴
    if (target > value) return 1;

    delay(60000); // 1분마다 확인하도록 delay

    // 안 끝났으면 0 리턴
    return 0;
}

void *setTimer(void *arg) {
    int value = (int)arg;
    int min, hour, target;
    printf("Timer: Set time %d\n", value);
    while (!isQuit) {
        min = wiringPiI2CReadReg8(i2c_fd, 0x01);
        hour = wiringPiI2CReadReg8(i2c_fd, 0x02);
        target = hour * 60 + min;
        printf("Timer: target: %d, value: %d\n", target, value);

        // 타이머 끝났을 때
        if (target >= value) {
        	printf("Timer: Send to rotate value 0\n");
    		mq_send(mq_rotate, "0", 2, 0);
    		printf("Timer: Send to strength value 0\n");
    		mq_send(mq_strength, "0", 2, 0);
        	break;
        }
        delay(5000); // 5초마다 확인
    }
}