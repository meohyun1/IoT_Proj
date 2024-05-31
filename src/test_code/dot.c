#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define dot "/dev/dot"

int main() { 
	int dot_d, i; 
	unsigned char c[8] = {
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00010000,
    0b00100000,
    0b01000000,
    0b10000000
};

	if((dot_d = open(dot, O_RDWR)) < 0) {  
		printf("Can't Open\n");  
		exit(0); 
	}

	write(dot_d,&c,sizeof(c)); 

	close(dot_d); 
	sleep(1);
	return 0;
}