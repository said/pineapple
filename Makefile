CC = gcc
VERSION = alpha-mega
CFLAGS = -std=c99 -O2 -Wall $(SDL_CFLAGS) $(NCURSES_CFLAGS)

SDL_CFLAGS := $(shell pkg-config --cflags sdl)
NCURSES_CFLAGS := $(shell ncurses5-config --cflags)

LIBS := -O2 $(shell pkg-config --libs sdl) \
		$(shell ncurses5-config --libs) \
		$(shell pkg-config --libs caca) #\
		#$(shell pkg-config --libs jack)

all: pineapple-tracker player

pineapple-tracker:	main.o oldchip.o gui.o modes.o actions.o musicchip_file.c conf_file.c
	$(CC) -o $@ $^ ${LIBS}

player:		player.o oldchip.o gui.o modes.o actions.o musicchip_file.c conf_file.o
	$(CC) -o $@ $^ ${LIBS}

%.o:	%.c pineapple.h gui.h musicchip_file.h conf_file.h Makefile

.PHONY:
	clean
clean:	
	@echo "clean ..."
	@rm -f *.o
