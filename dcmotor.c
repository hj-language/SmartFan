#include <wiringPi.h>
#include <stdio.h>
#include <mqueue.h>

#define PWM0 18
#define PWM1 19
#define GPIO23 23
#define GPIO24 24
#define GPIO25 25
#define GPIO8 8

#define FREQ 1000
#define RANGE 100

/*
    dcmotor 클라이언트는
    - server로부터 세기(0~3) 입력 받기
    - timer로부터 종료 세기(0) 입력 받기
    세기에 따라 motor_Rotate의 인자를 바꿔서 실행한다.
*/

/* 메시지 큐 관련 변수 */
mqd_t mq_strength;
struct mq_attr attr;
const char* mq_strength_name = "/strength";
char buf[BUFSIZ];
int n;

/* 함수 선언 */
int init();
void motor_Rotate(int);

void main() {
    if (init() == -1) return;
    
    int strength, isQuit = 0;
    
    while (1) {
        n = mq_receive(mq_strength, buf, sizeof(buf), NULL);
        printf("Receive from server value %d\n", buf[0]-'0');
        if (buf[0]-'0' == 9) break;
    	motor_Rotate(buf[0]-'0');
    }
    motor_Rotate(0);                // 마지막에 세기 0 주고 종료
    mq_close(mq_strength);
    mq_unlink(mq_strength_name);
}

int init() {
    /* wiringPi setup */
    if (wiringPiSetupGpio() < 0) {
        printf("Unable to setup wiringPi.\n");
        return -1;
    }

    /* pin mode 설정 */
    pinMode(PWM0, PWM_OUTPUT);
    pinMode(PWM1, PWM_OUTPUT);
    pinMode(GPIO23, OUTPUT);
    pinMode(GPIO24, OUTPUT);
    pinMode(GPIO25, OUTPUT);
    pinMode(GPIO8, INPUT);

    /* pwm 설정 */
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(RANGE);
	pwmSetClock(19200000 / (FREQ * RANGE));

    /* 메시지 큐 속성 초기화 */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = BUFSIZ;
    attr.mq_curmsgs = 0;
    mq_strength = mq_open(mq_strength_name, O_CREAT | O_RDONLY, 0644, &attr);
    
    return 0;
}

void motor_Rotate(int strength) {
    printf("Strength value %d\n", strength);
	switch(strength) {
		case 0:
			pwmWrite(PWM0, 0);
			digitalWrite(GPIO23, 1);
			digitalWrite(GPIO24, 1);
			digitalWrite(GPIO25, 1);
			break;
		case 1:
			pwmWrite(PWM0, 40);
			digitalWrite(GPIO23, 0);
			digitalWrite(GPIO24, 1);
			digitalWrite(GPIO25, 1);
			break;
		case 2:
			pwmWrite(PWM0, 70);
			digitalWrite(GPIO23, 0);
			digitalWrite(GPIO24, 0);
			digitalWrite(GPIO25, 1);	
			break;
		case 3:
			pwmWrite(PWM0, 100);
			digitalWrite(GPIO23, 0);
			digitalWrite(GPIO24, 0);
			digitalWrite(GPIO25, 0);
			break;
		default:
			printf("error: incorrect strength");
			return;	
	}
	pwmWrite(PWM1, 0);
}
