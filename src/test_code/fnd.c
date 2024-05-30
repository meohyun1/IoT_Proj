#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define fnd "/dev/fnd"

int main() { 
	int fnd_d; 
	unsigned char fnd_data[4];

 	if((fnd_d = open(fnd,O_RDWR)) < 0) { 
		perror("open"); 
		exit(1); 
	}

	fnd_data[0] = 0xC0; 
	fnd_data[1] = 0xC0; 
	fnd_data[2] = 0xC0; 
	fnd_data[3] = 0xC0;
	
	write(fnd_d,fnd_data,sizeof(fnd_data));
	
	sleep(5);

	return 0;
}