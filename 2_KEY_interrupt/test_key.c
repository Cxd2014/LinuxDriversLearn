
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	int fd;
	unsigned char val;
	
	fd = open("/dev/keys", O_RDWR);
	if (fd < 0)
	{
		printf("can't open!\n");
	}
	
	while(1)
	{
		read(fd,&val,1);
		printf("key_val = %d\n",val);
	}
	
	return 0;
}
