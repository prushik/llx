#include "libc.h"

//void *memcpy(void *dst, void * src, int size);

int main()
{
	char one[] = "cocaine\n";
	char two[] = "in a bag\n";
	char dest[50] = {0};

	memcpy(dest, one, 3);
	dest[7]=' ';
	memcpy(&dest[8],two,3);

	write(1,dest,49);

	return;
}
