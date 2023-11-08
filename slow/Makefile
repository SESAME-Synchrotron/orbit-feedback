EPICS     = /opt/epics/base
HOST      = linux-x86_64
MKL       = ${HOME}/intel/oneapi/mkl/latest

PROGRAM  += sofb
SRCS     += sofb.c main.c

INCDIRS  += $(MKL)/include
INCDIRS  += $(EPICS)/include
INCDIRS  += $(EPICS)/include/os/Linux
INCDIRS  += $(EPICS)/include/compiler/gcc

LIBS     += ca Com
LIBS     += mkl_intel_lp64 mkl_gnu_thread mkl_core 
LIBS     += gomp pthread m dl rt
LIBDIRS  += $(EPICS)/lib/$(HOST)
LIBDIRS  += $(MKL)/lib/intel64

# CFLAGS += -O3 -g -std=c99 -D_SVID_SOURCE
CFLAGS   += -pedantic
CPPFLAGS += $(INCDIRS:%=-I %)
LDFLAGS  += $(LIBDIRS:%=-L %)
LDFLAGS  += $(LIBDIRS:%=-Wl,-rpath,%)
LDFLAGS  += $(LIBS:%=-l%)

OBJS = $(addsuffix .o,$(basename $(SRCS)))

.PHONY:
.PHONY: clean install

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

clean:
	rm -rf $(PROGRAM)
	rm -rf $(PROGRAM).o
	rm -rf $(OBJS)
	rm -f /usr/bin/$(PROGRAM)

install: $(PROGRAM)
	cp $(PROGRAM) /usr/bin

