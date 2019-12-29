
CFLAGS = -std=c++11 -Wall -pedantic

ifeq ($(OS),Windows_NT)
  CURSES=pdcurses
  TARGET=main.exe
  CLEAN_CMD=rd /s /q bin
else
  CURSES=ncurses
  TARGET=main
  CLEAN_CMD=rm -r bin
endif

all: clean makeout main

main.o: src/main.cpp
	g++ -g -c -o bin/main.o src/main.cpp $(CFLAGS) 

game.o: src/game.cpp src/game.h
	g++ -g -c -o bin/game.o src/game.cpp $(CFLAGS)

object.o: src/object.cpp src/object.h
	g++ -g -c -o bin/object.o src/object.cpp $(CFLAGS)

main: main.o game.o object.o
	g++ -g -o bin/$(TARGET) bin/main.o bin/game.o bin/object.o $(CFLAGS) -l$(CURSES)
	@echo ==========
	@echo Compile done
	@echo Please run game with command:
	@echo Windows : bin\main.exe
	@echo Unix    : bin/main

makeout:
	@echo Create output folder
	mkdir bin
	@echo ==========

clean:
	@echo Clean output foldel
	$(CLEAN_CMD)
	@echo ==========
