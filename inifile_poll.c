
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h> 

#include <fcntl.h>
#include <errno.h>

#include "inifile.h"


#define IS_SHOWDEBUG_MESSAGE			1


#if IS_SHOWDEBUG_MESSAGE
#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)


int main(int argc, char *argv[])
{
	PERROR("Hello world\n");
	return 0;
}
