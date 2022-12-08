#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <sys/ioctl.h>

/*
    최종 수정: 2022-12-08 17:13
    timer 클라이언트는
    - 서버로부터 데이터 받기
    입력 받은 값 만큼 타이머를 실행한다.
    종료 시간이 되면 POSIX 메시지 큐를 이용하여 
    세기 담당, 회전 담당 프로세스에게 전송한다.
*/

#define SLAVE_ADDR_01 0x68 
static const char* I2C_DEV = "/dev/i2c-1";
int i2c_fd;

/* 메시지 큐 관련 변수 */
mode_t mq_timer, mq_strength;
struct mq_attr attr;
const char* mq_timer_name = "/timer";  
const char* mq_strength_name = "/strength";
char buf[BUFSIZ];
int n;

/* 함수 선언 */
int init();
int readReg(int i2c_fd, int value);

int main() 
{
    int value = 1;
    int isInit = 0;
    int isQuit = 0;

    while (!isQuit) {
        n = mq_receive(mq_timer, buf, sizeof(buf), NULL);

        value = atoi(buf);

        if (value > 0 && isInit == 0)
            if ((isInit = initReg()) == -1)
                return;

        if (value > 0 && isInit == 1) {
            if (readReg(i2c_fd, value)) {
                printf("타이머 종료 !\n");
                isInit = 0;
                break;
            }
        }
    }
    
    return 0;
}

int init()
{
    /* wiringPi setup */
    if (wiringPiSetup() < 0) {
        printf("wiringPiSetup() is failed\n");
        return -1;
    }

    /* I2C 연결 설정 */
    i2c_fd = wiringPiI2CSetupInterface(I2C_DEV, SLAVE_ADDR_01);
    wiringPiI2CWriteReg8(i2c_fd, 0x00, 0x00);
    wiringPiI2CWriteReg8(i2c_fd, 0x01, 0x00);
    wiringPiI2CWriteReg8(i2c_fd, 0x02, 0x00);

    /* 메시지 큐 속성 초기화 */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = BUFSIZ;
    attr.mq_curmsgs = 0;
    mq_timer = mq_open(mq_timer_name, O_CREAT | O_RDONLY, 0644, &attr);
    mq_strength = mq_open(mq_strength_name, O_WRONLY);
    return 1;
}

int readReg(int i2c_fd, int value)
{
    int min = wiringPiI2CReadReg8(i2c_fd, 0x01);
    int hour = wiringPiI2CReadReg8(i2c_fd, 0x02);

    int target = hour * 60 + min;

    // 타이머 끝났을땐 1 리턴
    if (target > value) return 1;

    delay(60000); // 1분마다 확인하도록 delay

    // 안 끝났으면 0 리턴
    return 0;
}