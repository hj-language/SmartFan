#include <stdio.h>
#include <wiringPi.h>
#define TRIG 15
#define ECHO 18 

/*
    최종 수정 : 2022-12-14 10:30
    ultrasonic 클라이언트는
    - 서버로부터 작동 시그널 받기
    일단 초음파 센서 탐지값 출력
*/

float getDistance()
{
    int start, end;
	float distance;
    float m_distance = 0; // 거리 탐지 최소값

	pinMode(ECHO, INPUT);
	pinMode(TRIG, OUTPUT);

    while (1) 
    {
		digitalWrite(TRIG, 0);
		delay(200);
		
		digitalWrite(TRIG, 1);
		delayMicroseconds(10);
		digitalWrite(TRIG, 0);

		while (digitalRead(ECHO) == 0)
			start = micros();
		while (digitalRead(ECHO) == 1)
			end = micros();
		distance = (float)(end - start) / 29. / 2. * 1000.;

        if (m_distance > distance)
        {
            m_distance = distance;
        }

		printf("distance(mm): %f\n", distance);
	}
}