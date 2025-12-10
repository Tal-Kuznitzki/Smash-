CC = gcc
CFLAGS = -std=c99 -Wall -Werror -pedantic-errors -pthread -DNDEBUG
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

EXT_OBJ = my_system_call_c.o
TARGET = smash

all: $(TARGET)

commands.o: commands.h
signals.o: signals.h
smash.o: commands.h signals.h my_system_call.h

$(TARGET): $(OBJS) $(EXT_OBJ)
	$(CC) $(CFLAGS) $(OBJS) $(EXT_OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(OBJS)  $(EXT_OBJ) my_system_call.o
