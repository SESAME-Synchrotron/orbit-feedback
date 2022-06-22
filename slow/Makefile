EPICS     = /opt/epics/base
HOST      = linux-x86_64

PROGRAM  += sofb
SRCS     += sofb.c
INCDIRS  += $(EPICS)/include
INCDIRS  += $(EPICS)/include/os/Linux
INCDIRS  += $(EPICS)/include/compiler/gcc
LIBS     += ca Com
LIBDIRS  += $(EPICS)/lib/$(HOST)

# CFLAGS += -O3
# CFLAGS += -g
# CFLAGS += -std=c99 -D_SVID_SOURCE
CFLAGS   += -Wall -Wno-unused-function
CPPFLAGS += $(INCDIRS:%=-I %)
LDFLAGS  += $(LIBDIRS:%=-L %)
LDFLAGS  += $(LIBDIRS:%=-Wl,-rpath,%)
LDFLAGS  += $(LIBS:%=-l%)

OBJS = $(addsuffix .o,$(basename $(SRCS)))
DOUT = $(addsuffix .d,$(basename $(SRCS)))

.PHONY:
.PHONY: clean install

$(PROGRAM): $(OBJS)
	@$(CC) $(LDFLAGS) $^ -o $@
	@echo LD $< $@

%.o: %.c
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
	@echo CC $< $@

clean:
	rm -rf $(PROGRAM)
	rm -rf $(PROGRAM).d
	rm -rf $(PROGRAM).o
	rm -rf $(OBJS)
	rm -rf $(DOUT)
	rm -f /usr/bin/$(PROGRAM)

install: $(PROGRAM)
	cp $(PROGRAM) /usr/bin

