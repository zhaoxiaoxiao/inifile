
CC=gcc

DEBUGFLAG=-g -Wall

INCLUDE=
LIBDIRS=
LIBS=

MALLOC_OBJS=inifile_malloc.o
STACK_OBJS=inifile_stack.o
POOL_OBJS=inifile_poll.o memory_pool.o

MALLOC_TARGET=inifile_malloc
STACK_TARGET=inifile_stack
POOL_TARGET=inifile_poll

default:$(MALLOC_TARGET) $(STACK_TARGET) $(POOL_TARGET)

clean:
	rm -f $(MALLOC_TARGET) $(STACK_TARGET) $(POOL_TARGET) $(MALLOC_OBJS) $(STACK_OBJS) $(POOL_OBJS)

$(MALLOC_TARGET):$(MALLOC_OBJS)
	$(CC) -o $(MALLOC_TARGET) $^ $(LIBDIRS) $(LIBS)

$(STACK_TARGET):$(STACK_OBJS)
	$(CC) -o $(STACK_TARGET) $^ $(LIBDIRS) $(LIBS)

$(POOL_TARGET):$(POOL_OBJS)
	$(CC) -o $(POOL_TARGET) $^ $(LIBDIRS) $(LIBS)

.c.o:
	$(CC) $(DEBUGFLAG) -fPIC -c $< -o $@ $(INCLUDE)
