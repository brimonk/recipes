CC=cc
LINKER=-ldl -lpthread -lm -lmagic -lsodium
CFLAGS=-fPIC -Wall -g3 -march=native
TARGET=./recipe
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
DEP=$(OBJ:.o=.d) # one dependency file for each source

ADDR=127.0.0.1
PORT=5000

all: $(TARGET) ext_uuid.so

watch:
	while true; do \
		make $(WATCHMAKE); \
		inotifywait -qre close_write .; \
	done

%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

-include $(DEP)

ext_uuid.so: src/sqlite3.o src/uuid.o
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ $(LINKER)

$(TARGET): src/sqlite3.o src/recipe.o
	$(CC) $(CFLAGS) -o $(TARGET) $^ $(LINKER)

node_modules: ./node_modules
	npm i

clean:
	rm -f $(OBJ) $(DEP)
	rm -f $(TARGET) ext_uuid.so
	rm -rf node_modules/

