CC=cc
LINKER=-ldl -lpthread -lm -lmagic -lsodium
CFLAGS=-fPIC -Wall -g3 -march=native
TARGET=./recipe

SERV_SRC=src/recipe.o src/sqlite3.o
SERV_OBJ=$(SERV_SRC:.c=.o)
SERV_DEP=$(SERV_SRC:.c=.d)

META_SRC=src/meta.o src/sqlite3.o
META_OBJ=$(META_SRC:.c=.o)
META_DEP=$(META_SRC:.c=.d)

ALL_OBJ=$(SERV_OBJ) $(META_OBJ)
ALL_DEP=$(SERV_DEP) $(META_DEP)

JS=$(wildcard src/*.js)

ADDR=127.0.0.1
PORT=5000

all: ext_uuid.so meta $(TARGET) html/ui.js

%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

-include $(DEP)

ext_uuid.so: src/sqlite3.o src/lib/uuid.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ $(LINKER)

$(TARGET): $(SERV_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LINKER)

meta: $(META_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LINKER)

html/ui.js: $(JS)
	cat $(JS) > $@

clean:
	rm -f $(ALL_OBJ) $(ALL_DEP)
	rm -f $(TARGET) ext_uuid.so
	rm -f html/ui.js

