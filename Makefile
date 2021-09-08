CC=cc
LINKER=-ldl -lpthread -lm -lmagic -lsodium
CFLAGS=-fPIC -Wall -g3 -march=native
TARGET=./recipe
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
DEP=$(OBJ:.o=.d)
JS=$(wildcard src/*.js)

ADDR=127.0.0.1
PORT=5000

all: $(TARGET) ext_uuid.so html/ui.js

%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

-include $(DEP)

ext_uuid.so: src/sqlite3.o src/lib/uuid.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ $(LINKER)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $^ $(LINKER)

html/ui.js: $(JS)
	cat $(JS) > $@

clean:
	rm -f $(OBJ) $(DEP)
	rm -f $(TARGET) ext_uuid.so
	rm -f html/ui.js

