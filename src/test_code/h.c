#include<stdio.h>

#include<stdlib.h>

#include<fcntl.h>

#include<string.h>



#define clcd "/dev/clcd"


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

void set_lcd_bot(int line) {
    int clcds;
    
    if((clcds=open(clcd, O_RDWR)) < 0 ){
        printf("clcd open error\n");
        exit(1);
    }
    char buffer[33];  // 32글자를 위한 버퍼
    snprintf(buffer, 32, "%s%s", clcd_top, clcd_bot[line]);
    printf("\nthis is printf\n%s\n",buffer);
    write(clcds, buffer, 32);
    close(clcds);
    return;
}

int main()
{
  set_lcd_bot(1);
	return 0;
}
