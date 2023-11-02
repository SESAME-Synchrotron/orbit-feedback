# gcc fofb.c -o fofb -DMKL_ILP64 -m64 -I"${MKL_ROOT}/mkl/include" -L${MKL_ROOT}/mkl/lib/intel64 -Wl,--no-as-needed -lmkl_intel_ilp64 -lmkl_gnu_thread -lmkl_core -lgomp -lpthread -lm -ldl -lrt 

MKL       = ${MKL_ROOT}
PROGRAM  += fofb
SRCS     += fofb.c
INCDIRS  += $(MKL)/mkl/include
LIBDIRS  += $(MKL)/mkl/lib/intel64
LIBS     += mkl_intel_ilp64 mkl_gnu_thread mkl_core 
LIBS     += gomp pthread m dl rt
# gcc fofb.c -o fofb -DMKL_ILP64 -m64 -I"${MKLROOT}/include" -L${MKLROOT}/lib/intel64 -Wl,--no-as-needed -lmkl_intel_ilp64 -lmkl_gnu_thread -lmkl_core -lgomp -lpthread -lm -ldl
# CFLAGS += -O3
# CFLAGS += -g
# CFLAGS += -std=c99 -D_SVID_SOURCE
# CFLAGS   += -Wall
CFLAGS   += -Wall -Wno-unused-function -Wno-unused-but-set-variable
CPPFLAGS += $(INCDIRS:%=-I %)
LDFLAGS  += $(LIBDIRS:%=-L %)
LDFLAGS  += $(LIBDIRS:%=-Wl,-rpath,%)
LDFLAGS  += $(LIBS:%=-l%)

OBJS = $(addsuffix .o,$(basename $(SRCS)))
DOUT = $(addsuffix .d,$(basename $(SRCS)))

.PHONY:
.PHONY: clean install

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

clean:
	rm -rf $(PROGRAM)
	rm -rf $(PROGRAM).d
	rm -rf $(PROGRAM).o
	rm -rf $(OBJS)
	rm -rf $(DOUT)
	rm -f /usr/bin/$(PROGRAM)

install: $(PROGRAM)
	cp $(PROGRAM) /usr/bin

