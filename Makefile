PROG := gcc -Wall -g -I include/
RM := rm -rf

all: clean distclean cassini saturnd launch

cassini: timing-text-io.o common.o client-request.o client-reply.o cassini.o
	$(PROG) -o cassini timing-text-io.o common.o client-request.o client-reply.o cassini.o

saturnd: timing-text-io.o common.o client-reply.o saturnd.o
	$(PROG) -o saturnd timing-text-io.o common.o client-reply.o saturnd.o 
	
cassini.o: src/cassini.c 
	$(PROG) -c src/cassini.c

saturnd.o: src/saturnd.c
	$(PROG) -c src/saturnd.c

common.o: src/util/common.c
	$(PROG) -c src/util/common.c

client-request.o: src/cassini/client-request.c
	$(PROG) -c src/cassini/client-request.c

client-reply.o: src/cassini/client-reply.c
	$(PROG) -c src/cassini/client-reply.c
	
timing-text-io.o: src/util/timing-text-io.c
	$(PROG) -c src/util/timing-text-io.c -o timing-text-io.o

launch:
	./saturnd

distclean:
	$(RM) saturnd cassini *.o

clean:
	if pgrep saturnd; then killall saturnd; fi

remove:
	$(RM) /tmp/$(USER)/