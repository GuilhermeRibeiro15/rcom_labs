CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lm

SRC_FILES = download.c getip.c connection.c clientTCP.c
OBJ_FILES = $(SRC_FILES:.c=.o)

TARGET = my_program

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ_FILES)
