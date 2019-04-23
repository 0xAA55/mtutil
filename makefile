CC=gcc
LD=gcc
AR = $(GCC_PREFIX)gcc-ar
RANLIB = $(GCC_PREFIX)gcc-ranlib
OPTIMIZATIONS=-g -O3 -fdata-sections -ffunction-sections -fmerge-all-constants -flto -fuse-linker-plugin -ffat-lto-objects
CFLAGS=-Wall $(OPTIMIZATIONS) -I.. -I../common/inc

OBJS=mtatomic.o
OBJS+=mtcommon.o
OBJS+=mtsched.o
OBJS+=mtschedman.o
OBJS+=mutex.o
OBJS+=rwlock.o
OBJS+=randinst.o

all: libmtutil.a

-include $(OBJS:.o=.d)

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	$(CC) -MM $(CFLAGS) $*.c > $*.d
	
libmtutil.a: $(OBJS)
	$(AR) rcu $@ $+
	$(RANLIB) $@

clean:
	del *.o *.a *.d
