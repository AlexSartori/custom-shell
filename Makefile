.PHONY: build clean mkdir
CC= gcc
CFLAG= -c
OFLAG= -o
SDIR= src
DDIR= bin
RL= -lreadline
GNU= -std=gnu90


build: main exec vector internals utils mkdir
	@ $(CC) $(OFLAG) shell *.o $(RL) $(GNU) &&  mv *.o shell ./$(DDIR)

clean:
	@ rm -rf $(DDIR)

main: $(SDIR)/main.c $(SDIR)/vector.h $(SDIR)/utils.h
	@ $(CC) $(CFLAG) $(SDIR)/main.c

exec: $(SDIR)/exec.c $(SDIR)/exec.h
	@ $(CC) $(CFLAG) $(SDIR)/exec.c

vector: $(SDIR)/vector.c $(SDIR)/vector.h
	@ $(CC) $(CFLAG) $(SDIR)/vector.c

internals: $(SDIR)/internals.c $(SDIR)/internals.h
	@ $(CC) $(CFLAG) $(SDIR)/internals.c

utils: $(SDIR)/utils.c $(SDIR)/utils.h
	@ $(CC) $(CFLAG) $(SDIR)/utils.c

mkdir:
	@ mkdir -p $(DDIR)
