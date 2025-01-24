#compiler and flags --> gcc dirwatch.c -o dirwatch
CC= gcc
CFLAGS = -Wall -g -std=gnu99
LDFLAGS = -lpthread -lm

#source and object files
SRC = dirwatch.c
OBJ = $(SRC:.c=.o)

#Target executable 
TARGET = dirwatch

#default taregt to build the program
all: $(TARGET)

#build target executable 
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)
	
clean:
	rm -f $(OBJ) $(TARGET)

