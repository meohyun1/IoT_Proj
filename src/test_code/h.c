#include <stdio.h>          	// 입출력 관련 
#include <stdlib.h>         	// 문자열 변환, 메모리 관련 
#include <string.h>       	    // 문자열 처리 
#include <time.h>         	    // 시간 관련
#include <stdbool.h>
#include<unistd.h>       		// POSIX 운영체제 API에 대한 액세스 제공 
#include<fcntl.h>			    // 타겟시스템 입출력 장치 관련 
#include<sys/types.h>    		// 시스템에서 사용하는 자료형 정보 
#include<sys/ioctl.h>    		// 하드웨어의 제어와 상태 정보 
#include<sys/stat.h>     		// 파일의 상태에 대한 정보 



// Target System
#define fnd "/dev/fnd"			// 7-Segment FND 
#define dot "/dev/dot"			// Dot Matrix
#define tact "/dev/tactsw"    	// Tact Switch
#define led "/dev/led"			// LED 
#define dip "/dev/dipsw"		// Dip Switch
#define clcd "/dev/clcd"		// Character LCD

int tactsw;
int dipsw;
int leds;
int dot_mtx;
int clcds;
int fnds;

unsigned char fnd_output[15] = {
    0b11000000, // 0
    0b11111001, // 1
    0b10100100, // 2
    0b10110000, // 3
    0b10011001, // 4
    0b10010010, // 5
    0b10000010, // 6
    0b11111000, // 7
    0b10000000, // 8
    0b10010000, // 9
    0b11000001, // U
    0b10010010, // S
    0b10000110, // E
    0b10100001,  // d
    0b11111111 //출력 없음
};
void set_turn_score(int score) {
    if((fnds = open(fnd, O_RDWR)) < 0 ){
        printf("fnd open error\n");
        exit(1);
    }

    unsigned char buffer[4];

    //턴당 점수 50 이상 불가능 USEd에 사용
    if(score>50) {
        buffer[0] = fnd_output[10];
        buffer[1] = fnd_output[11];
        buffer[2] = fnd_output[12];
        buffer[3] = fnd_output[13];
    }
    else {
        int tens = (score / 10) % 10;
        int ones = score % 10;
        buffer[0] = fnd_output[14];
        buffer[1] = fnd_output[tens];
        buffer[2] = fnd_output[ones];
        buffer[3] = fnd_output[14];
    }

    write(fnds, &buffer, sizeof(buffer));
    sleep(5);
    close(fnds);
    return;
}

int main()
{
    set_turn_score(33);
    sleep(2);
    set_turn_score(100);
	return 0;
}
