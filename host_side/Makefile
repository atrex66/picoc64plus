CC      = gcc
CFLAGS  = -O2 -Wall -Wextra $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)
TARGET  = c64terminal

all: font.h $(TARGET)

# Extract the PETSCII font from the project's character ROM header
font.h:
	python3 extract_font.py

$(TARGET): main.c font.h protocol.h
	$(CC) $(CFLAGS) -o $@ main.c $(LDFLAGS)

clean:
	rm -f $(TARGET) font.h

.PHONY: all clean
