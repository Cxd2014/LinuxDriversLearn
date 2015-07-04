#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

int fd;

/* 信号处理函数 */
void signal_fun(int signal)
{
	unsigned char val;

	read(fd, &val,1); 
	printf("ket_val = %d\n", val);
}

int main(int argc, char **argv)
{
	int oflags;

	signal(SIGIO,signal_fun);//注册信号处理函数

	fd = open("/dev/keys", O_RDWR);
	if (fd < 0)
	{
		printf("can't open!\n");
	}
	
	//把进程 PID 告诉内核，是驱动程序知道信号发给哪一个进程
	fcntl(fd,F_SETOWN,getpid());

	//调用 file_operations 中的 .fasync 函数
	oflags = fcntl(fd,F_GETFL);
	fcntl(fd,F_SETFL,oflags|FASYNC);

	while(1)
	{
		sleep(5000);
	}
	
	return 0;
}
