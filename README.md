# terminal_tetris

## Prepare
WINDOWS:
1. Download and install MinGW: http://www.mingw.org/
2. Install PDCurses for MinGW:
```
mingw-get install mingw32-pdcurses
mingw-get install mingw32-libpdcurses
```
3. Increase Window Size for cmd before play game
- Width >= 42
- Height >= 32

UNIX:
1. Install libncurses

## Compile
WINDOWS:
```
mingw32-make all
```
UNIX:
```
make all
```

## Play game
WINDOWS:
```
bin\main.exe
```
UNIX:
```
bin/main
```
