# Variables for compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g
# The final executable name
TARGET = vfs

# Look for all .c files in the current directory
SRCS = $(wildcard *.c)

# Default rule that runs when you just type 'make'
all:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

# Clean rule to remove the executable
clean:
	rm -f $(TARGET)
