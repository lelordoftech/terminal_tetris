# terminal_tetris

[//]: # (Image References)

[image1]: ./img/CMD_Layout.PNG "CMD Layout"

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

![alt text][image1]

UNIX:
1. Install libncurses
- Ubuntu:
```
sudo apt-get install libncurses5
sudo apt-get install libncurses5-dev
```

- MacOS: TODO

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
