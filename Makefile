CC = gcc
CFLAGS = -Wno-all
TARGET = badking
SRC = decompiled.c strings.c dataseg.c

all: $(TARGET)

$(TARGET): $(SRC) compat.h decls.h dataseg.h
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
