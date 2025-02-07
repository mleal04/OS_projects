CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = myshell

# All target, to build the final executable
all: $(TARGET)

# Link the object files to create the target
$(TARGET): myshell.o dirwatch.o
	$(CC) $(CFLAGS) -o $(TARGET) myshell.o dirwatch.o

# Compile myshell.o
myshell.o: myshell.c dirwatch.h
	$(CC) $(CFLAGS) -c myshell.c

# Compile dirwatch.o
dirwatch.o: dirwatch.c dirwatch.h
	$(CC) $(CFLAGS) -c dirwatch.c

# Clean target to remove object files and the target executable
clean:
	rm -f $(TARGET) *.o

