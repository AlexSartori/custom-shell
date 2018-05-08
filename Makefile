.PHONY: build clean mkdir
CC= gcc
CFLAG= -c
OFLAG= -o
SDIR= src
DDIR= bin
RL= -lreadline


build: main queue utils mkdir
	@ $(CC) $(OFLAG) shell *.o $(RL) &&  mv *.o shell ./$(DDIR)

clean: 
	@ rm -f $(DDIR)/shell $(DDIR)/*.o

main: $(SDIR)/main.c $(SDIR)/queue.h $(SDIR)/utils.h
	@ $(CC) $(CFLAG) $(SDIR)/main.c

queue: $(SDIR)/queue.c $(SDIR)/queue.h
	@ $(CC) $(CFLAG) $(SDIR)/queue.c

utils: $(SDIR)/utils.c $(SDIR)/utils.h
	@ $(CC) $(CFLAG) $(SDIR)/utils.c

mkdir: 
	@ mkdir -p $(DDIR)


