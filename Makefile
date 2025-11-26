CC = gcc
CFLAGS = -std=c99 -Wall -Werror -pedantic-errors -pthread -DNDEBUG
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
TARGET = smash

all: $(TARGET)
commands.o: commands.h
signals.o: signals.h
smash.o: commands.h signals.h my_system_call.h

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(OBJS)
