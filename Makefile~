CC=gcc
CFLAGS=-g -Wall
LDFLAGS=-ldl -lrt
ASMLIBS=/export/home/fan/workspace/gem5/gem5-micro2017/util/m5/m5op_x86.S
#Current make system
BIN=bin
target=l2pp-two-threads
LIST=$(addprefix $(BIN)/,$(target))

all: $(LIST)

$(BIN)/%:  %.S
	$(CC) $(INC) $(ASMLIBS) $(CFLAGS) $(LDFLAGS) $<  -o $@
%.S: %.c config.h
	$(CC) $(CFLAGS) -S $<  -o $@
clean:
	rm $(BIN)/*
