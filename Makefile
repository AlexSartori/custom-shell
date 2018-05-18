.PHONY: build clean mkdir
CC= gcc -g
CFLAG= -c
OFLAG= -o
SDIR= src
DDIR= bin
RL= -lreadline
GNU= -std=gnu90


build: main exec vector internals utils mkdir
	@ $(CC) $(GNU) $(OFLAG) shell *.o $(RL) &&  mv *.o shell ./$(DDIR)

clean:
	@ rm -rf $(DDIR)

main: $(SDIR)/main.c $(SDIR)/vector.h $(SDIR)/utils.h
	@ $(CC) $(GNU) $(CFLAG) $(SDIR)/main.c

exec: $(SDIR)/exec.c $(SDIR)/exec.h
	@ $(CC) $(GNU) $(CFLAG) $(SDIR)/exec.c

vector: $(SDIR)/vector.c $(SDIR)/vector.h
	@ $(CC) $(GNU) $(CFLAG) $(SDIR)/vector.c

internals: $(SDIR)/internals.c $(SDIR)/internals.h
	@ $(CC) $(GNU) $(CFLAG) $(SDIR)/internals.c

utils: $(SDIR)/utils.c $(SDIR)/utils.h
	@ $(CC) $(GNU) $(CFLAG) $(SDIR)/utils.c

mkdir:
	@ mkdir -p $(DDIR)
