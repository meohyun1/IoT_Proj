#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

// Target System
#define fnd "/dev/fnd"         // 7-Segment FND
#define dot "/dev/dot"         // Dot Matrix
#define tact "/dev/tactsw"     // Tact Switch
#define led "/dev/led"         // LED
#define dip "/dev/dipsw"       // Dip Switch
#define clcd "/dev/clcd"       // Character LCD

void set_dice(int*);
void set_lcd_bot(int);
void set_turn_score(int);
void roll_dice(int*);
void calc_score(int*, int*);
int turn(int*);
void cleanup_resources();

// 전역 변수 삭제

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

void open_devices(int* tactsw, int* dipsw, int* clcds, int* dot_mtx, int* fnds) {
    if((*tactsw = open(tact, O_RDWR)) < 0) {
        printf("tact open error\n");
        exit(1);
    }
    if((*dipsw = open(dip, O_RDWR)) < 0) {
        printf("dip open error\n");
        exit(1);
    }
    if((*clcds = open(clcd, O_RDWR)) < 0) {
        printf("clcd open error\n");
        exit(1);
    }
    if((*dot_mtx = open(dot, O_RDWR)) < 0) {
        printf("dot open error\n");
        exit(1);
    }
    if((*fnds = open(fnd, O_RDWR)) < 0) {
        printf("fnd open error\n");
        exit(1);
    }
}

int main() {
    srand(time(NULL));
    atexit(cleanup_resources);

    int tactsw, dipsw, clcds, dot_mtx, fnds;
    open_devices(&tactsw, &dipsw, &clcds, &dot_mtx, &fnds);

    int score[2] = {0, 0};
    int score_category[2] = {0, 0};
    int r, t;

    for(r = 0; r < 12; r++) {
        for(t = 0; t < 2; t++) {
            set_lcd_bot(12 + t);
            score[t] += turn(&score_category[t]);

            clcd_top[4 + t * 8] = (score[t] / 100) + '0';
            clcd_top[5 + t * 8] = (score[t] / 10 % 10) + '0';
            clcd_top[6 + t * 8] = (score[t] % 10) + '0';
        }
    }

    if(score[0] > score[1]) {
        set_lcd_bot(14);
    } else if(score[1] > score[0]) {
        set_lcd_bot(15);
    } else {
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
    int dot_mtx = open(dot, O_RDWR);
    if (dot_mtx < 0) {
        printf("Cannot open dot device\n");
        exit(0);
    }
    write(dot_mtx, &dot_buffer, sizeof(dot_buffer));

    int score[12] = {0};
    int roll_count = 0;
    unsigned char dip_input = 0;
    unsigned char tact_input = 0;
    unsigned char before_input = -1;
    int dice[5] = {0, 0, 0, 0, 0};

    while(1) {
        usleep(10000);

        if(roll_count < 3) {
            int dipsw = open(dip, O_RDWR);
            if(dipsw < 0) {
                printf("Cannot open dip switch\n");
                exit(0);
            }
            read(dipsw, &dip_input, sizeof(dip_input));
            close(dipsw);

            if(dip_input & 128) {
                roll_dice(dice);
                calc_score(score, dice);
                set_lcd_bot(17);
                roll_count++;
                dip_input = 0;
                continue;
            }
        }

        if(roll_count == 0) continue;
        usleep(10000);

        int tactsw = open(tact, O_RDWR);
        if(tactsw < 0) {
            printf("Cannot open tact switch\n");
            exit(0);
        }
        read(tactsw, &tact_input, sizeof(tact_input));
        close(tactsw);

        while(0 < tact_input && tact_input < 13) {
            unsigned char temp = 0;
            usleep(10000);
            tactsw = open(tact, O_RDWR);
            if(tactsw < 0) {
                printf("Cannot open tact switch\n");
                exit(0);
            }
            read(tactsw, &temp, sizeof(temp));
            close(tactsw);

            if(!temp) {
                if(tact_input != before_input) {
                    set_lcd_bot(tact_input - 1);
                    printf("[dot] %d %d %d %d %d\n", dice[0], dice[1], dice[2], dice[3], dice[4]);

                    if((1 << (tact_input - 1)) & *category) {
                        set_turn_score(100);
                    } else {
                        set_turn_score(score[tact_input - 1]);
                        before_input = tact_input;
                    }
                } else {
                    *category |= 1 << (tact_input - 1);
                    close(dot_mtx);
                    return score[tact_input];
                }
                break;
            }
        }
    }
}

void roll_dice(int* dice) {
    unsigned char hold;
    int dipsw = open(dip, O_RDWR);
    if(dipsw < 0) {
        printf("Cannot open dip switch\n");
        exit(0);
    }
    read(dipsw, &hold, sizeof(hold));
    close(dipsw);

    int i;
    for(i = 0; i < 5; i++) {
        if(!(hold & (1 << i))) {
            dice[i] = rand() % 6 + 1;
        }
    }

    while(1) {
        usleep(10000);
        dipsw = open(dip, O_RDWR);
        if(dipsw < 0) {
            printf("Cannot open dip switch\n");
            exit(0);
        }
        read(dipsw, &hold, sizeof(hold));
        close(dipsw);

        if(!(hold & 128)) {
            break;
        }
    }
    set_dice(dice);
}

void calc_score(int* score, int* dice) {
    int counts[7] = {0};
    int i;

    for(i = 1; i < 7; i++) {
        int cnt = 0;
        for(int j = 0; j < 5; j++) {
            if(dice[j] == i) {
                cnt++;
            }
        }
        counts[i] = cnt;
        score[i] = cnt * i;
    }

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
        score[7] = 25;
    } else {
        score[7] = 0;
    }

    for(i = 1; i <= 6; i++) {
        if(counts[i] >= 4) {
            score[8] = counts[i] * i;
        }
    }

    if(counts[1] == 1 && counts[2] == 1 && counts[3] == 1 && counts[4] == 1 && counts[5] == 1) {
        score[9] = 30;
    } else {
        score[9] = 0;
    }

    if(counts[2] == 1 && counts[3] == 1 && counts[4] == 1 && counts[5] == 1 && counts[6] == 1) {
        score[10] = 30;
    } else {
        score[10] = 0;
    }

    for(i = 1; i <= 6; i++) {
        score[11] += counts[i] * i;
    }

    for(i = 1; i <= 6; i++) {
        if(counts[i] == 5) {
            score[12] = 50;
        }
    }
}

void set_dice(int* dice) {
    int i, j;
    for(i = 0; i < 5; i++) {
        for(j = 7; j > 1 ; j--) {
            if(8 - j <= dice[i]) {
                dot_buffer[j] |= (128 >> i);
            } else {
                dot_buffer[j] &= ~(128 >> i);
            }
        }
    }

    int dot_mtx = open(dot, O_RDWR);
    if (dot_mtx < 0) {
        printf("Cannot open dot device\n");
        exit(0);
    }
    write(dot_mtx, &dot_buffer, sizeof(dot_buffer));
    close(dot_mtx);

    printf("[dot] %d %d %d %d %d\n", dice[0], dice[1], dice[2], dice[3], dice[4]);
}

void set_lcd_bot(int line) {
    char buffer[33];
    snprintf(buffer, 32, "%s%s", clcd_top, clcd_bot[line]);

    printf("[CLCD] %s\n", buffer);

    int clcds = open(clcd, O_RDWR);
    if (clcds < 0) {
        printf("Cannot open CLCD device\n");
        exit(0);
    }
    write(clcds, buffer, 32);
    close(clcds);
}

void set_turn_score(int score) {
    unsigned char buffer[5];

    if(score > 50) {
        buffer[0] = fnd_output[10];
        buffer[1] = fnd_output[11];
        buffer[2] = fnd_output[12];
        buffer[3] = fnd_output[13];
    } else {
        int tens = (score / 10) % 10;
        int ones = score % 10;
        buffer[0] = fnd_output[14];
        buffer[1] = fnd_output[tens];
        buffer[2] = fnd_output[ones];
        buffer[3] = fnd_output[14];
    }

    int fnds = open(fnd, O_RDWR);
    if (fnds < 0) {
        printf("Cannot open FND device\n");
        exit(0);
    }
    write(fnds, &buffer, sizeof(buffer));
    close(fnds);

    printf("[fnd] %d\n", score);
}

void cleanup_resources() {
    printf("Cleanup resources\n");
}