

CC=gcc
TARGET=main
CFLAGS=-g -Wall -lpthread

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $(TARGET).c -o $(TARGET)

clean:
	$(RM) $(TARGET)