.PHONY: build clean mkdir
CC= gcc -g  -Wall #-Wextra
CFLAG= -c
OFLAG= -o
SDIR= src
DDIR= bin
RL= -lreadline
GNU= -std=gnu90


build: main exec vector internals utils parsers mkdir
	@ $(CC) $(GNU) $(OFLAG) shell *.o $(RL) &&  mv *.o shell ./$(DDIR)

clean:
	@ rm -rf $(DDIR)

main: $(SDIR)/main.c $(SDIR)/vector.h $(SDIR)/exec.h $(SDIR)/utils.h $(SDIR)/parsers.h
	@ $(CC) $(GNU) $(CFLAG) $(SDIR)/main.c

exec: $(SDIR)/exec.c $(SDIR)/exec.h
	@ $(CC) $(GNU) $(CFLAG) $(SDIR)/exec.c

vector: $(SDIR)/vector.c $(SDIR)/vector.h
	@ $(CC) $(GNU) $(CFLAG) $(SDIR)/vector.c

internals: $(SDIR)/internals.c $(SDIR)/internals.h
	@ $(CC) $(GNU) $(CFLAG) $(SDIR)/internals.c

utils: $(SDIR)/utils.c $(SDIR)/utils.h
	@ $(CC) $(GNU) $(CFLAG) $(SDIR)/utils.c

parsers: $(SDIR)/parsers.c $(SDIR)/parsers.h
	@ $(CC) $(GNU) $(CFLAG) $(SDIR)/parsers.c

mkdir:
	@ mkdir -p $(DDIR)
