#include<stdio.h>

#include<stdlib.h>

#include<fcntl.h>

#include<string.h>



#define dot "/dev/dot"			// Dot Matrix-

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

int dot_mtx;

void set_dice(int* dice) {
    if((dot_mtx = open(dot, O_RDWR)) < 0 ){
        printf("dot open error\n");
        exit(1);
    }
    
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
    close(dot_mtx);
    return;
}

int main()
{
    int d[5] = {1,2,3,4,5};
    set_dice(d);
    int i;
    for(i = 0;i<8;i++){
        printf("%x\n",dot_buffer[i]);
    }
	return 0;
}
