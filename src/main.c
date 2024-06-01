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

void set_dice(int*);
// void set_roll_cnt(char);
void set_lcd_bot(int);
void set_turn_score(int);
void roll_dice(int*);
void calc_score(int*, int*);
int turn(int*);
void cleanup_resources();

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
    0b10100001, // d
    0b11111111 //출력 없음
};
unsigned char dot_buffer[8] = {
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
};
char clcd_top[17] = {
    " P1 000  P2 000 "
};
char clcd_bot[18][17] = {
    "      Ones      ",
    "      Twos      ",
    "     Threes     ",
    "      Fours     ",
    "      Fives     ",
    "      Sixes     ",
    "   Full House   ",
    " Four of a Kind ",
    "Little Straight ",
    "  Big Straight  ",
    "     Choice     ",
    "      Yacht     ",
    "  P1 turn Roll  ",
    "  P2 turn Roll  ",
    "    P1 win!!    ",
    "    P2 win!!    ",
    "      draw      ",
    "  pick or roll  "
};

int main() {
    srand(time(NULL));
    atexit(cleanup_resources);

    if((tactsw = open( tact, O_RDWR )) < 0){
        printf("tact open error\n");
        exit(1);
    }

    if((dipsw = open( dip, O_RDWR )) < 0){
        printf("dip open error\n");
        exit(1);
    }

    if((clcds = open(clcd, O_RDWR)) < 0 ) {
        printf("clcd open error\n");
        exit(1);
    }
    
    if((dot_mtx = open(dot, O_RDWR)) < 0 ){
        printf("dot open error\n");
        exit(1);
    }

    if((fnds = open(fnd, O_RDWR)) < 0 ) {
        printf("fnd open error\n");
        exit(1);
    }
    
    // 합산 스코어
    int score[2] = {0, 0};
    //사용 족보 바이너리로 바꾸어서 플래그로 사용(0~4095) 사용시 1
    int score_category[2] = {0, 0};
    int r; // round : 12라운드
    int t; // 0: p1턴/ 1 : p2턴

    for(r = 0; r < 12; r++) {
        for(t = 0; t < 2; t++) {
            // P0 turn Roll
            set_lcd_bot(12 + t);
            score[t] += turn(&score_category[t]);
            
            // clcd 윗줄 수정
            clcd_top[4 + t * 8] = (score[t] / 100) + '0';
            clcd_top[5 + t * 8] = (score[t] / 10 % 10) + '0';
            clcd_top[6 + t * 8] = (score[t] % 10) + '0';
        }
    }

    //p1 win
    if(score[0] > score[1]) {
        set_lcd_bot(14);
    }
    //p2 win
    else if(score[1] > score[0]) {
        set_lcd_bot(15);
    }
    //draw
    else {
        set_lcd_bot(16);
    }
    
    close(tactsw);
    close(dipsw);
    close(clcds);
    close(dot_mtx);
    close(fnds);
    return 0;
}

int turn(int* category) {
    write(dot_mtx, &dot_buffer, sizeof(dot_buffer));
    

    int score[12] = {0};
    int roll_count = 0;
    unsigned char dip_input = 0;
    unsigned char tact_input = 0;
    unsigned char before_input = -1;
    int dice[5] = {0, 0, 0, 0, 0};

    while(1) {
        usleep(10000); // 0.01 초 쉬기

        if(roll_count < 3){
            read(dipsw, &dip_input, sizeof(dip_input));
            if(dip_input & 128) {
                roll_dice(dice);
                calc_score(score, dice);
                set_lcd_bot(17);
                roll_count++;
                dip_input = 0;
                continue;
            }
        }
        
        // 주사위 안굴렸으면 dip만 읽어야함
        if(roll_count == 0) continue;
        usleep(10000); // 0.01 초 쉬기

        read(tactsw, &tact_input, sizeof(tact_input));
        // tact 누름
        while(0 < tact_input && tact_input < 13){
            unsigned char temp = 0;
            usleep(10000); // 0.01 초 쉬기
            read(tactsw, &temp, sizeof(temp));
            // tact 뗌
            if(!temp){
                //tact 눌렀을 때 이벤트 처리
                // 점수 출력 부분
                if(tact_input != before_input) {
                    set_lcd_bot(tact_input-1);
                    printf("[dot] %d %d %d %d %d\n",dice[0],dice[1],dice[2],dice[3],dice[4]);
                    
                    if((1 << (tact_input - 1)) & *category) {
                        set_turn_score(100);
                    }
                    else {
                        set_turn_score(score[tact_input-1]);
                        before_input = tact_input;
                    }
                }
                // 점수 확정 부분
                else {
                    //TODO category에 사용한 족보 넣기
                    *category |= 1 << (tact_input - 1);
                    return score[tact_input];
                }
                break;
            }
        }
    }
}

void roll_dice(int* dice) {
    unsigned char hold;
    int i, j;
    
    read(dipsw, &hold, sizeof(hold));

    for(i = 0; i < 5; i++) {
        if(!(hold & (1 << i))){
            dice[i] = rand() % 6 + 1;
        }
    }

    while(1) {
        usleep(10000); // 0.01 초 쉬기
        read(dipsw, &hold, sizeof(hold));
        // 딥스위치 맨 오른쪽 내렸을때
        if(!(hold & 128)) {
            break;
        }
    }
    set_dice(dice);
}

void calc_score(int* score, int* dice) { 
    // 1부터 6까지의 주사위 눈의 빈도를 저장하기 위한 배열, 0번 인덱스는 사용하지 않음
    int counts[7] = {0};
    int i, j;

    for(i = 1;i < 7; i++) {
        int cnt = 0;

    	for(j = 0; j < 5; j++) {
            if(dice[j] == i) {
                cnt++;
            }
            //counts[dice[i]]++; // 주사위 빈도 계산
        }
        counts[i] = cnt;
        score[i]=cnt*i;//ones ~ sixs
    }

    // Full House: 3개가 같고 2개가 같을 때
    bool three_of_a_kind = false, pair = false;
    for(i = 1; i <= 6; i++) {
        if(counts[i] == 3) {
            three_of_a_kind = true;
        }
        if(counts[i] == 2) {
            pair = true;
        }
    }

    if(three_of_a_kind && pair) {
        score[7] = 25; // Full House 점수는 25점으로 가정
    } 

	else {
        score[7] = 0;
    }

    // Four of a Kind: 같은 눈 4개
    for(i = 1; i <= 6; i++) {
        if (counts[i] >= 4) {
            score[8] = counts[i] * i; // Four of a Kind 점수는 해당 눈의 값 * 4
        }
    }

    // Little Straight: 1-2-3-4-5
    if(counts[1] == 1 && counts[2] == 1 && counts[3] == 1 && counts[4] == 1 && counts[5] == 1) {
        score[9] = 30; // Little Straight 점수는 30점
    } 
    else {
        score[9] = 0;
    }

    // Big Straight: 2-3-4-5-6
    if (counts[2] == 1 && counts[3] == 1 && counts[4] == 1 && counts[5] == 1 && counts[6] == 1) {
        score[10] = 30; // Big Straight 점수는 30점
    } 
    else {
        score[10] = 0;
    }

    // Choice: 모든 주사위의 합
    for(i = 1; i <= 6; i++) {
        score[11] += counts[i] * i;
    }

    // Yacht: 모든 주사위가 같을 때
    for(i = 1; i <= 6; i++) {
        if (counts[i] == 5) {
            score[12] = 50; // Yacht 점수는 50점
        }
    }
    return;
}

void set_dice(int* dice) {
    int i, j;

    // 열
    for(i = 0; i < 5; i++) { 
        // 행
        for(j = 7; j > 1 ; j--) {
            if(8 - j <= dice[i]) {
                dot_buffer[j] |= (128 >> i);  // 해당 비트를 켭니다.
            } 
            else {
                dot_buffer[j] &= ~(128 >> i); // 해당 비트를 끕니다.
            }
        }
    }

    write(dot_mtx, &dot_buffer, sizeof(dot_buffer));
    printf("[dot] %d %d %d %d %d\n",dice[0],dice[1],dice[2],dice[3],dice[4]);
    return;
}

// void set_roll_cnt(char roll_cnt) {
//     dot_buffer[0] &= 0x00;
//     switch (roll_cnt) {
//         case 3:
//             dot_buffer[0] |= 0x07; // 0000 0111
//             break;
//         case 2:
//             dot_buffer[0] |= 0x03; // 0000 0011
//             break;
//         case 1:
//             dot_buffer[0] |= 0x01; // 0000 0001
//             break;
//     }

//     write(dot_mtx,&dot_buffer, sizeof(dot_buffer));
//     return;
// }

void set_lcd_bot(int line) {

    char buffer[33];  // 32글자를 위한 버퍼
     snprintf(buffer, sizeof(buffer), "%s%s", clcd_top, clcd_bot[line]);

    printf("[CLCD] %s\n",buffer);
    if (write(clcds, buffer, 32) != 32) {
        perror("write error");
    }
    close(clcd_fd);
    return;
}

void set_turn_score(int score) {
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

    if (write(fnds, buffer, sizeof(buffer)) != sizeof(buffer)) {
        perror("write error");
    }
    printf("[fnd] %d\n",score);
    close(fnds);
    return;
}

void cleanup_resources() {
    if (tactsw > 0) {
        close(tactsw);
        printf("Tact switch file descriptor closed.\n");
    }
    if (dipsw > 0) {
        close(dipsw);
        printf("Dip switch file descriptor closed.\n");
    }
    if (leds > 0) {
        close(leds);
        printf("LED file descriptor closed.\n");
    }
    if (dot_mtx > 0) {
        close(dot_mtx);
        printf("Dot matrix file descriptor closed.\n");
    }
    if (clcds > 0) {
        close(clcds);
        printf("Character LCD file descriptor closed.\n");
    }
    if (fnds > 0) {
        close(fnds);
        printf("FND file descriptor closed.\n");
    }
}
