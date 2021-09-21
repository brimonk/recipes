CC=cc
LINKER=-ldl -lpthread -lm -lmagic -lsodium
CFLAGS=-DSQLITE_ENABLE_JSON1 -fPIC -Wall -g3 -march=native
TARGET=./recipe

SERV_SRC=src/recipe.c src/sqlite3.c
SERV_OBJ=$(SERV_SRC:.c=.o)
SERV_DEP=$(SERV_SRC:.c=.d)

META_SRC=src/meta.c src/sqlite3.c
META_OBJ=$(META_SRC:.c=.o)
META_DEP=$(META_SRC:.c=.d)

ALL_OBJ=$(SERV_OBJ) $(META_OBJ)
ALL_DEP=$(SERV_DEP) $(META_DEP)

JS=$(wildcard src/*.js)

ADDR=127.0.0.1
PORT=5000

all: ext_uuid.so meta src/generated.h $(TARGET) html/ui.js

%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

-include $(DEP)

ext_uuid.so: src/sqlite3.o src/lib/uuid.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ $(LINKER)

src/generated.h: src/meta.c src/schema.sql src/metaprogram.sql
	./meta recipe.db > src/generated.h

$(TARGET): src/generated.h $(SERV_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LINKER)

meta: $(META_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LINKER)

html/ui.js: $(JS)
	cat $(JS) > $@

clean:
	rm -f $(ALL_OBJ)
	rm -f $(ALL_DEP)
	rm -f $(TARGET) ext_uuid.so
	rm -f html/ui.js

