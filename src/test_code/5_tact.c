#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <asm/ioctls.h>

#define tact_d "/dev/tactsw"

int main() { 
	int tact; 
	unsigned char c;
	if((tact = open(tact_d,O_RDWR)) < 0) {  
		perror("open");  
		exit(1); 
	} 
	while(1) {  
		read(tact,&c,sizeof(c));
		if(c) break; 
	} 
	
	switch(c) {  
		case 1: printf("\n      Ones      ");	break;  
		case 2: printf("\n      Twos      ");   break;  
		case 3: printf("\n     Threes     ");   break;  
		case 4: printf("\n      Fours     ");   break;  
		case 5: printf("\n      Fives     ");   break;  
		case 6: printf("\n      Sixes     ");   break;  
		case 7: printf("\n   Full House   ");   break;  
		case 8: printf("\n Four of a Kind ");   break;  
		case 9: printf("\nLittle Straight ");   break;  
		case 10: printf("\n  Big Straight  ");   break; 
		case 11: printf("\n     Choice     ");   break;  
		case 12: printf("\n      Yacht     ");   break;  
	}

	close(tact); 
	return 0;
}