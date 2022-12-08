#include <wiringPi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <pthread.h>
#define FIXED 0
#define AUTO 1
#define DERIVED 2
#define CW 0
#define CCW 

/* 
    최종 수정: 2022-12-08 17:06
    rotate 클라이언트는
    - server로부터 모드 입력 받기
    모드에 따라 쓰레드를 생성 및 종료하는 과정을 반복한다.
*/

int pin_arr[4] = {12, 16, 20, 21};
int one_two_phase[8][4] = {
	{1,0,0,0},
	{1,1,0,0},
	{0,1,0,0},
	{0,1,1,0},
	{0,0,1,0},
	{0,0,1,1},
	{0,0,0,1},
	{1,0,0,1}
};

/* 공유 변수 */
int degree; // 현재 각도 저장
int mode;   // 회전 모드 저장 
int dir;    // 회전 방향 저장

/* 메시지 큐 관련 변수 */
mode_t mq_rotate;
struct mq_attr attr;
const char* mq_rotate_name = "/rotate";  
char buf[BUFSIZ];
int n;

/* 함수 선언 */
void init();
void Rotate_Auto();     // 자동 회전 모드
void Rotate_Derived();  // 유도 회전 모드
void one_two_Phase_Rotate_Angle(float angle, int dir);

int main()
{
	if (init() == -1) return;
	
    pthread_t thread = 0;     // tid 저장 변수
    int threadReturnValue;    // 쓰레드 반환값 저장 변수
    int isQuit = 0;               // 종료 여부 저장
    
    while (!isQuit) {
        n = mq_receive(mq_rotate, buf, sizeof(buf), NULL);
        if ((mode = atoi(buf[0])) == FIXED) {           // 고정 모드
        	mode = FIXED;
            // 기존 쓰레드 종료
            threadReturnValue = pthread_join(thread, NULL); 
            if (threadReturnValue < 0) return;
            thread = 0;
        } else if ((mode = atoi(buf[0])) == AUTO) {     // 자동 회전 모드
            // 기존 쓰레드 종료
            if (thread != 0) threadReturnValue = pthread_join(thread, NULL); 
            // 새 쓰레드 실행
            threadReturnValue = pthread_create(&thread, NULL, Rotate_Auto, NULL);
            if (threadReturnValue < 0) return;
        } else if ((mode = atoi(buf[0])) == DERIVED) {  // 객체 유도 모드
            // 기존 쓰레드 종료
            if (thread != 0) threadReturnValue = pthread_join(thread, NULL); 
            // 새 쓰레드 실행
            threadReturnValue = pthread_create(&thread, NULL, Rotate_Derived, NULL);
            if (threadReturnValue < 0) return;
        }
        if ((mode = atoi(buf[0])) == 999) {
        	isQuit = 1;
        }
        memset(buf, 0, sizeof(buf));
    }
	
	FILE* fp = fopen("stepmotor_degree.txt", "wt");
	fprintf(fp, "%d", degree);
	fclose(fp);
	return 0;
}

void one_two_Phase_Rotate_Angle(float angle, int dir) {	
	// 4096 step이면 360도
	// x*11.38step이면 x도 
	
	for(int i = 0; i < angle * 11.38; i++){
		if (dir == 1) { // CW
			for(int j = 0; j < 4; j++){
				digitalWrite(pin_arr[j], one_two_phase[i%8][j]);
				delay(1);
			}	
		}
		else { // CCW
			for(int j = 0; j < 4; j++){
				digitalWrite(pin_arr[j], one_two_phase[7-i%8][j]);
				delay(1);
			}
		}
	}
}

int init()
{
	/* wiringPi setup */
    if (wiringPiSetupGpio() < 0) {
        printf("Unable to setup wiringPi.\n");
        return -1;
    }

	/* 저장된 degree 불러오기 */
	FILE* fp = fopen("stepmotor_degree.txt", "rt");
	fscanf(fp, "%d", &degree);
	fclose(fp);
	
    /* 스텝 모터 핀 모드를 OUTPUT으로 설정 */
	for(int i = 0 ; i < 4; i++)
		pinMode(pin_arr[i], OUTPUT);

    /* 메시지 큐 속성 초기화 */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = BUFSIZ;
    attr.mq_curmsgs = 0;
    mq_rotate = mq_open(mq_rotate_name, O_CREAT | O_RDONLY, 0644, &attr);

	return 0;
}

void Rotate_Auto() {
	// 11.38 step이 돼야 1도.
	float step = 0;
	int i = 0;
	while (mode == AUTO) {
		if (dir == CCW) {
			for(int j = 0; j < 4; j++){
				digitalWrite(pin_arr[j], one_two_phase[i%8][j]);
				delay(1);
			}
			step++;
			if (step - 11.38 > 0) {
				degree++;
				step = 0;
			}
			if (degree == 150) dir = CW;
		}
		else if (dir == CW) {
			for(int j = 0; j < 4; j++){
				digitalWrite(pin_arr[j], one_two_phase[7-i%8][j]);
				delay(1);
			}
			step++;
			if (step - 11.38 > 0) {
				degree--;
				step = 0;
			}
			if (degree == 30) dir = CCW;
		}
		i++;
		if (i == 10000) {
			return;
		}
	}
}

void Rotate_Derived() {

}