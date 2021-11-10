CC=cc
LINKER=-ldl -lpthread -lm -lmagic -lsodium -ljansson
CFLAGS=-fPIC -Wall -g3 -march=native
TARGET=./recipe

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
DEP=$(SRC:.c=.d)

JS=$(wildcard src/*.js)

all: $(TARGET) html/ui.js

watch: all
	while [ true ] ; do \
		pkill $(TARGET) ; \
		make ; \
		./$(TARGET) recipe.db & \
		inotifywait src -e MODIFY -e CREATE ; \
	done ; \
	true

run: all
	./$(TARGET) recipe.db

%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) -fsanitize=address -c $(CFLAGS) -o $@ $<

-include $(DEP)

$(TARGET): $(OBJ)
	$(CC) -fsanitize=address $(CFLAGS) -o $@ $^ -static-libasan $(LINKER)

html/ui.js: $(JS)
	cat $(JS) > $@

clean:
	rm -f $(OBJ)
	rm -f $(DEP)
	rm -f $(TARGET)
	rm -f html/ui.js

