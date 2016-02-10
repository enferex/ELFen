APP=elfish
CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -g3 -O0
SRCS=main.c
OBJS=$(SRCS:.c=.o)

all: $(APP)

$(APP): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: test
test: $(APP)
	./$(APP) $(APP)

clean:
	$(RM) $(APP) $(OBJS)
