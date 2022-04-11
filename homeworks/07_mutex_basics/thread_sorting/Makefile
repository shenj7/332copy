CC = gcc
CFLAGS = -Wall -g -pthread -c
LDFLAGS = -pthread
SOURCES = threadSort.c
OBJECTS = $(SOURCES:.c=.o) 
PROG1 = threadSort

all: $(PROG1)

# build executables
# $^ matches all the dependencies
$(PROG1): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

# convert .c files to .o files
# $< matches the first dependency
# $@ matches the target
$(SOURCES:%.c.o):
	$(CC) $(CFLAGS) $< $@

clean:
	rm -rf *.o *~ $(PROG1)
