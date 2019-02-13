CC=gcc
CFLAGS=-g -Wall
LDFLAGS=-ldl -lrt 
INC=-I../util/
OBJ=../util/util.o
ASMLIBS=/home/hongyu/gem5-fy/util/m5/m5op_x86.S
#Current make system
BIN=bin
#target=c5-single-thread c5-spy c5-trojan c5-trojan-measure\
#	   test-latency l2pp-two-threads l1pp-0.1 nos-two-threads
#LIST=$(addprefix $(BIN)/,$(target))
l2pp-2-thds:bin/l2pp-two-threads
all:l2pp-2-thds

bin/%:  %.c
	$(CC) $(INC) $(OBJ) $(ASMLIBS) $(CFLAGS) $(LDFLAGS) $^  -o $@
clean:
	rm $(BIN)/*
