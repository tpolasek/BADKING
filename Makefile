CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = badking
SRC = badking.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all clean
