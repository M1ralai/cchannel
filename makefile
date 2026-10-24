CC=clang
CFLAGS=-g -O0 -fsanitize=address,undefined -pthread
OUT=bin/out

CFILES=src/main.c \
			 src/channel/channel.c \
			 src/connection/connection.c \

all: 
	$(CC) $(CFLAGS) $(CFILES) -o $(OUT) && ./$(OUT)
