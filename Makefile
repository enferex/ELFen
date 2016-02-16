APP=elfen
CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -g3 -O0
LDFLAGS=
SRCS=main.c
OBJS=$(SRCS:.c=.o)

all: $(APP)

.PHONY: aspell
aspell: CFLAGS+=-DUSE_ASPELL=1
aspell: LDFLAGS+=-laspell
aspell: clean $(OBJS) $(APP)

$(APP): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: test
test: $(APP)
	./$(APP) $(APP)

clean:
	$(RM) $(APP) $(OBJS)
