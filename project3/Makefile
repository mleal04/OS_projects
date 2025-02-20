# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -std=gnu99
LDFLAGS = -lpthread -lm

# Source and object files
SRC = seqperf.c threadperf.c syncperf.c progressperf.c
OBJ = $(SRC:.c=.o)

# Target executables
TARGETS = seqperf threadperf syncperf progressperf

# Default target to build all programs
all: $(TARGETS)

# Build each target
seqperf: seqperf.o
	$(CC) $(CFLAGS) -o seqperf seqperf.o $(LDFLAGS)

threadperf: threadperf.o
	$(CC) $(CFLAGS) -o threadperf threadperf.o $(LDFLAGS)

syncperf: syncperf.o
	$(CC) $(CFLAGS) -o syncperf syncperf.o $(LDFLAGS)

progressperf: progressperf.o
	$(CC) $(CFLAGS) -o progressperf progressperf.o $(LDFLAGS)

# Cleanup
clean:
	rm -f $(OBJ) $(TARGETS)
