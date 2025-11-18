CC = gcc
CFLAGS = -Wall -Wextra -std=c99
SRC = $(wildcard *.c)
TARGET = program

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
