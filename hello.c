#include<stdio.h>
#include<unistd.h>

int main(void) {
	int i = 1;
	while (i < 60) {
		i++;
		sleep(1);
	}
	return 0;
}