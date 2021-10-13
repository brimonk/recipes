CC=cc
LINKER=-ldl -lpthread -lm -lmagic -lsodium -lasan
CFLAGS=-fPIC -Wall -g3 -march=native
TARGET=./recipe

SRC=src/recipe.c src/user.c src/object.c
OBJ=$(SRC:.c=.o)
DEP=$(SRC:.c=.d)

JS=$(wildcard src/*.js)

all: $(TARGET) html/ui.js

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

