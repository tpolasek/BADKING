CC = gcc
# Suppress warnings (-w). On Linux/GCC the diagnostic below is just a warning
# and is silenced by -w. On macOS the default "gcc" is Apple clang, which has
# promoted -Wint-conversion (char* -> intptr_t in calls like game_strcmp) to a
# HARD ERROR that -w cannot silence; downgrade it back to a warning. This flag
# is valid on both GCC and clang, so the Linux build is unaffected.
CFLAGS = -w -Wno-error=int-conversion
TARGET = badking
SRC = decompiled.c strings.c dataseg.c

all: $(TARGET)

$(TARGET): $(SRC) compat.h decls.h dataseg.h
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
