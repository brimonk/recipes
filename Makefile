CC=cc
LINKER=-ldl -lpthread -lm -lmagic -lsodium -ljansson
CFLAGS=-fPIC -Wall -g3 -march=native
TARGET=./recipe

SRC=src/main.c src/recipe.c src/sqlite3.c src/mongoose.c src/objects.c src/user.c src/tag.c
OBJ=$(SRC:.c=.o)
DEP=$(SRC:.c=.d)

all: $(TARGET) sqlite3_uuid.so sqlite3_passwd.so

watch: all
	while [ true ] ; do \
		pkill $(TARGET) ; \
		make ; \
		./$(TARGET) database.db & \
		inotifywait src -e MODIFY -e CREATE ; \
	done ; \
	true

run: all
	./$(TARGET) database.db

$(TARGET): $(OBJ)
	$(CC) -fsanitize=address $(CFLAGS) -o $@ $^ -static-libasan $(LINKER)

sqlite3_uuid.so: src/uuid.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ $(LINKER)

sqlite3_passwd.so: src/passwd.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ $(LINKER)

%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) -fsanitize=address -c $(CFLAGS) -o $@ $<

-include $(DEP)

clean:
	rm -f $(OBJ) $(DEP) $(TARGET) sqlite3_uuid.so sqlite3_passwd.so
