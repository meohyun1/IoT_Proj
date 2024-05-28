#include <stdio.h>          	// 입출력 관련 
#include <stdlib.h>         	// 문자열 변환, 메모리 관련 
#include <string.h>       	    // 문자열 처리 
#include <time.h>         	    // 시간 관련 
//#include <cstddef>
#include <stdbool.h>

//#include "hardware_io.h"
#include "logic_test.h"

void set_dice(int* dice);
void set_roll_cnt(char roll_cnt);
void set_lcd_bot(int line);
void set_turn_score(int score);

char fnd_output[15] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
    0b00000000,  //출력 없음
    0b00111110, // U
    0b01101101, // S
    0b01111001, // E
    0b01011110 // d
};
char dot_buffer[8] = {
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
char clcd_bot[17][17] = {
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
    "      draw      "
};

int turn(int);
int roll_calc_score(int*);

int main() {
    srand(time(NULL));

    // 합산 스코어
    int score[2] = {0,0};

    //사용 족보 바이너리로 바꾸어서 플래그로 사용(0~4095) 사용시 1
    int score_category[2] = {0,0};

    //12라운드
    for(int r=0;r<12;r++) {
        //0 : p1턴/ 1 : p2턴
        for(int t=0;t<2;t++) {
            set_lcd_bot(12+t);
            score[t] += turn(score_category[t]);
            //clcd_output[0] 갱신
            char temp_score = score[t];
            snprintf(temp_score, 4, "%03d", score[t]);
            memcpy(clcd_top + 3 + t*8, temp_score, 3);
        }
        unsigned char ledStatus = 0;
        if (r > 8) r = 8; // LED가 8개를 초과하는 경우, 최대 8개만 켬
        for (int i = 0; i < r; i++) {
            ledStatus |= (1 << i);  // 해당 LED 비트를 켬
        }
        set_led(ledStatus);
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
    return 0;
}

int turn(int category) {
    int score[12] = {0};
    int roll_count = 0;
    int dip_input = 0;
    int tact_input = 0;
    int before_input = 0;

    while(1) {
        //입력 및 주사위 굴리기
        while(1) {
            usleep(10000); //0.01 초 쉬기
            //TODO tactsw입력 tact_input으로
            get_tact_switch(&tact_input);
            if(!tact_input && roll_count < 3) { //tactsw 입력 없고 3번 이하로 굴렸을 때
            get_dip_switch(&dip_input);
                if(dip_input & 1) {// 딥스위치 맨 오른쪽 올렸을 때
                    roll_calc_score(score);
                    roll_count++;
                    tact_input = 1;
                    break;
                }
            }
            //아직 한번도 주사위 안굴렸으면 점수출력X
            else if(roll_count != 0) {
                break;
            }
        }
        
        if(tact_input == before_input) {
            category |= 1 << (tact_input-1);
            return score;
        }

        if((1 << (tact_input - 1)) & category) {
            //사용한 족보는 다시 사용 못하도록

            set_turn_score(100);
            continue;
        }
        else{
            //TODO lcd 족보 이름출력
            set_lcd_bot(tact_input-1);
            set_turn_score(score[tact_input]);
            //TODO category에 사용한 족보 넣기
            before_input = tact_input;
        }
    }
}

int roll_calc_score(int*score){
    int dice[5] = {0,0,0,0,0};
    int hold;
    get_dip_switch(&hold);
    for(int i=0;i<5;i++){
        if(!(hold & (1<<(i+3)))){
            dice[i] = rand()%6+1;
        }
    }

    // 1부터 6까지의 주사위 눈의 빈도를 저장하기 위한 배열, 0번 인덱스는 사용하지 않음
    int counts[7] = {0}; 
    for(int i=1;i<7;i++){
        int cnt = 0;
        for(int j=0;j<5;j++){
            if(dice[j]==i){
                cnt++;
            }
            counts[dice[i]]++; // 주사위 빈도 계산
        }
        score[i]=cnt*i;//ones ~ sixs
    }

    // Full House: 3개가 같고 2개가 같을 때
    bool three_of_a_kind = false, pair = false;
    for (int i = 1; i <= 6; i++) {
        if (counts[i] == 3) three_of_a_kind = true;
        if (counts[i] == 2) pair = true;
    }
    if (three_of_a_kind && pair) {
        score[7] = 25; // Full House 점수는 25점으로 가정
    } else {
        score[7] = 0;
    }

    // Four of a Kind: 같은 눈 4개
    for (int i = 1; i <= 6; i++) {
        if (counts[i] >= 4) {
            score[8] = counts[i] * i; // Four of a Kind 점수는 해당 눈의 값 * 4
        }
    }

    // Little Straight: 1-2-3-4-5
    if (counts[1] == 1 && counts[2] == 1 && counts[3] == 1 && counts[4] == 1 && counts[5] == 1) {
        score[9] = 30; // Little Straight 점수는 30점
    } else {
        score[9] = 0;
    }

    // Big Straight: 2-3-4-5-6
    if (counts[2] == 1 && counts[3] == 1 && counts[4] == 1 && counts[5] == 1 && counts[6] == 1) {
        score[10] = 30; // Big Straight 점수는 30점
    } else {
        score[10] = 0;
    }

    // Choice: 모든 주사위의 합
    int sum = 0;
    for (int i = 1; i <= 6; i++) {
        sum += counts[i] * i;
    }
    score[11] = sum;

    // Yacht: 모든 주사위가 같을 때
    for (int i = 1; i <= 6; i++) {
        if (counts[i] == 5) {
            score[12] = 50; // Yacht 점수는 50점
        }
    }
    int dip_input;
    int fake_dice[5] = {0,0,0,0,0};
    while(1){
        for(int i=0;i<5;i++){
            if(!(hold & (1<<(i+3)))){
                dice[i] = rand()%6+1;
            }
        }
        get_dip_switch(&dip_input);
        // 딥스위치 맨 오른쪽 내렸을때
        if(!(dip_input & 1)) {
            break;
        }
        set_dice(fake_dice);
        usleep(100000); //0.1 초 쉬기
    }
    set_dice(dice);

}

void set_dice(int* dice) {
    for(int i = 0; i < 5; i++){ // 열
        for(int j = 7; j > 1 ; j--){ // 행
            if(8-j <= dice[i]){
                dot_buffer[j] |= (128 >> i);  // 해당 비트를 켭니다.
            } else {
                dot_buffer[j] &= ~(128 >> i); // 해당 비트를 끕니다.
            }
        }
    }
    set_dot_matrix(dot_buffer);
    return;
}

void set_roll_cnt(char roll_cnt){
    dot_buffer[0] &= ~0x07;
    switch (roll_cnt) {
        case 3:
            dot_buffer[0] |= 0x07; // 0000 0111
            break;
        case 2:
            dot_buffer[0] |= 0x03; // 0000 0011
            break;
        case 1:
            dot_buffer[0] |= 0x01; // 0000 0001
            break;
    }
    set_dot_matrix(dot_buffer);
    return;
}

void set_lcd_bot(int line) {
    char buffer[33];  // 32글자를 위한 버퍼
    snprintf(buffer, sizeof(buffer), "%s%s", clcd_top, clcd_bot[line]);
    set_clcd(buffer);
    return;
}

void set_turn_score(int score){
    if(score>50){//턴당 점수 50 이상 불가능 USEd에 사용
        char buffer[5] = {fnd_output[10],fnd_output[11],fnd_output[12],fnd_output[13]};
        set_fnd(buffer);
        return;
    }
    int tens = (score / 10) % 10;
    int ones = score % 10;
    char buffer[5] = {fnd_output[14],fnd_output[tens],fnd_output[ones],fnd_output[14]};
    set_fnd(buffer);
    return;
}