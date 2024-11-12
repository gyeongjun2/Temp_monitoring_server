CC = gcc
CFLAGS = -Wall
TARGET = main_program

SRCS = main.c database.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
    $(CC) $(CFLAGS) -o $@ $^ -lsqlite3 -lwiringPi

%.o: %.c
    $(CC) $(CFLAGS) -c $<

clean:
    rm -f $(TARGET) $(OBJS)