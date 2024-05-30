#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define clcd "/dev/clcd"

int main() { 
	int clcd_d; 
	char buf[10]; 

	if((clcd_d = open(clcd,O_RDWR)) < 0) {  
		perror("open"); 
		exit(1); 
	} 

	// scanf로 입력 받음
	scanf("%s",buf); 

	write(clcd_d,&buf,strlen(buf)); 

	close(clcd_d); 

	return 0;
}