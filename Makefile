CFLAGS=-O2 -Wall -Wextra -Werror -c
LDFLAGS=-l:libcurl.so -lm -l:libjansson.so -l:libsqlite3.so -l:libSDL.so -l:libconfig.so
HEADERS= src/structures.h src/net.h src/tree.h src/vk_errors.h src/md5.h src/structures-auto.h src/global-vars.h
AUTOGEN_SRC= src/conf/messages.conf src/conf/users.conf

all: bin/vkconcli

bin/vkconcli: obj/main.o obj/md5.o obj/structures.o obj/net.o obj/tree.o obj/structures-auto.o obj/global-vars.o
	cc ${LDFLAGS} obj/main.o obj/md5.o obj/structures.o obj/net.o obj/tree.o obj/structures-auto.o obj/global-vars.o -o bin/vkconcli

obj/main.o: src/main.c ${HEADERS}
	cc ${CFLAGS} src/main.c -o obj/main.o

obj/net.o: src/net.c ${HEADERS}
	cc ${CFLAGS} src/net.c -o obj/net.o

obj/tree.o: src/tree.c ${HEADERS}
	cc ${CFLAGS} src/tree.c -o obj/tree.o

obj/md5.o: src/md5.c ${HEADERS}
	cc ${CFLAGS} src/md5.c -o obj/md5.o

obj/structures.o: src/structures.c ${HEADERS}
	cc ${CFLAGS} src/structures.c -o obj/structures.o

obj/structures-auto.o: src/structures-auto.c ${HEADERS}
	cc ${CFLAGS} src/structures-auto.c -o obj/structures-auto.o

obj/global-vars.o: src/global-vars.c ${HEADERS}
	cc ${CFLAGS} src/global-vars.c -o obj/global-vars.o

src/structures-auto.h: src/structures-auto.c

src/structures-auto.c: src/gen.py ${AUTOGEN_SRC}
	src/gen.py src/structures-auto.h src/structures-auto.c ${AUTOGEN_SRC}



clean:
	rm obj/* bin/* src/structures-auto.c src/structures-auto.h
