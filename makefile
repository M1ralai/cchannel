CC=clang
CFLAGS=-g -O0 -fsanitize=address,undefined -pthread -Wall -Wextra -Wconversion -Wsign-conversion
OUT=bin/out

CFILES=src/main.c \
			 src/channel/channel.c \
			 src/connection/connection.c \

all: 
	$(CC) $(CFLAGS) $(CFILES) -o $(OUT) && ./$(OUT)
