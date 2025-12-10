CC = gcc
CFLAGS = -std=c99 -Wall -Werror -pedantic-errors -pthread -DNDEBUG
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
# Define the external object file
EXT_OBJ = my_system_call_c.o
TARGET = smash

all: $(TARGET)

commands.o: commands.h
signals.o: signals.h
smash.o: commands.h signals.h my_system_call.h

# Link both your compiled objects (OBJS) and the provided object (EXT_OBJ)
$(TARGET): $(OBJS) $(EXT_OBJ)
	$(CC) $(CFLAGS) $(OBJS) $(EXT_OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	# FIX: Remove TARGET, your OBJS, AND the EXT_OBJ to satisfy the clean check
	rm -rf $(TARGET) $(OBJS) $(EXT_OBJ)
