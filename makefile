CC=gcc
#FLAGS=-ggdb -Wall
FLAGS=-Wall
RAYLIB = `pkg-config --libs --cflags raylib`
#linux use this
#RAYLIB = -lraylib

chart:main.c
	$(CC) $(FLAGS) main.c -ogame $(RAYLIB) -lm 