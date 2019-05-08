//
//  game.cpp
//  
//
//  Created by Vuong Le Quoc on 2019/05/06.
//

#include <ncurses.h>
#include <string> // std::string
#include <unistd.h> // usleep
#include <stdlib.h> // rand

#include "game.h"
#include "define.h"
#include "object.h"

// Check terminal has color or not
bool has_color = false;

WINDOW* main_wnd;
WINDOW* game_wnd;
WINDOW* label_wnd;
WINDOW* info_wnd;

rect screen_area;
rect game_area;
rect label_area;
rect info_area;

game_object* current_obj = new game_object();
game_object* next_obj = new game_object((object_type)(rand() % 5));

uint_fast16_t game_score = 0;
uint_fast8_t game_speed = 1;
uint_fast8_t game_level = 1;
bool game_sound = false;
bool game_pause = false;

void draw_object(WINDOW* wnd, game_object* obj)
{
  vec2i pos = obj->get_pos();
  uint_fast8_t color = obj->get_color();
  vec2ui* data = obj->get_data();
  wattron(wnd, COLOR_PAIR(color));
  for (size_t i = 0; i < 4; i++)
  {
    mvwaddch(wnd, pos.y+data[i].y, pos.x+2*data[i].x, ' '|A_REVERSE);
    mvwaddch(wnd, pos.y+data[i].y, pos.x+2*data[i].x+1, ' '|A_REVERSE);
  }
  wattroff(wnd, COLOR_PAIR(color));
}

int init()
{
  initscr();
  cbreak();
  noecho();
  clear();
  refresh();
  
  // hide cursor
  curs_set(0);
  
  // enable color
  start_color();
  has_color = has_colors();
  
  // area offset_x, offset_y, bounds_x, bounds_y
  // all screen area 42x20
  // game area 20x20
  // label area 8x20
  // info area 9x20
  screen_area.update(0, 0, GAME_WIDTH+LABEL_WIDTH+INFO_WIDTH+3, GAME_HEIGHT+2);
  game_area.update(1, 1, GAME_WIDTH, GAME_HEIGHT);
  label_area.update(1+GAME_WIDTH + 1, 1, LABEL_WIDTH, GAME_HEIGHT);
  info_area.update(2+GAME_WIDTH + LABEL_WIDTH, 1, INFO_WIDTH, GAME_HEIGHT);
  
  // multi windows
  // newwin nlines, ncols, begin_y, begin_x
  main_wnd = newwin(screen_area.height(), screen_area.width(), screen_area.top(), screen_area.left());
  game_wnd = newwin(game_area.height(), game_area.width(), game_area.top(), game_area.left());
  label_wnd = newwin(label_area.height(), label_area.width(), label_area.top(), label_area.left());
  info_wnd = newwin(info_area.height(), info_area.width(), info_area.top(), info_area.left());
  
  // useful color pairs
  init_pair(BLACK_PAIR, COLOR_BLACK, COLOR_BLACK);
  init_pair(RED_PAIR, COLOR_RED, COLOR_BLACK);
  init_pair(GREEN_PAIR, COLOR_GREEN, COLOR_BLACK);
  init_pair(YELLOW_PAIR, COLOR_YELLOW, COLOR_BLACK);
  init_pair(BLUE_PAIR, COLOR_BLUE, COLOR_BLACK);
  init_pair(MAGENTA_PAIR, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(CYAN_PAIR, COLOR_CYAN, COLOR_BLACK);
  init_pair(WHITE_PAIR, COLOR_WHITE, COLOR_BLACK);
  
  // color for windows
  wbkgd(game_wnd, COLOR_PAIR(GREEN_PAIR));
  wbkgd(label_wnd, COLOR_PAIR(GREEN_PAIR));
  wbkgd(info_wnd, COLOR_PAIR(GREEN_PAIR));
  
  // enable function keys
  keypad(main_wnd, true);
  keypad(game_wnd, true);
  keypad(label_wnd, true);
  keypad(info_wnd, true);
  
  // disable input blocking
  nodelay(main_wnd, true);
  nodelay(game_wnd, true);
  nodelay(label_wnd, true);
  nodelay(info_wnd, true);
  
  // frame around screen
  wattron(main_wnd, A_BOLD); // active attribute for drawing
  box(main_wnd, 0, 0); // draw a bouding box around main_wnd
  wattroff(main_wnd, A_BOLD); // inactive attribute for drawing
  
  // vertical dividing line
  wmove(main_wnd, game_area.top(), game_area.right());
  wvline(main_wnd, ACS_VLINE, game_area.height());
  
  // game info
  // SCORE
  // HI-SCORE
  // NEXT
  // SPEED
  // LEVEL
  // SOUND
  // STATUS
  wattron(label_wnd, COLOR_PAIR(COLOR_WHITE));
  mvwprintw(label_wnd, Y_INFO, 1, "%8s", "SCORE");
  mvwprintw(label_wnd, Y_INFO + 2, 1, "%8s", "HI-SCORE");
  mvwprintw(label_wnd, Y_INFO + 4, 1, "%8s", "NEXT");
  mvwprintw(label_wnd, Y_INFO + 9, 1, "%8s", "SPEED");
  mvwprintw(label_wnd, Y_INFO + 11, 1, "%8s", "LEVEL");
  mvwprintw(label_wnd, Y_INFO + 13, 1, "%8s", "SOUND");
  mvwprintw(label_wnd, Y_INFO + 15, 1, "%8s", "STATUS");
  wattroff(label_wnd, COLOR_PAIR(COLOR_WHITE));
  
  wattron(info_wnd, COLOR_PAIR(COLOR_WHITE));
  mvwprintw(info_wnd, Y_INFO, 0, "%08d", game_score);
  mvwprintw(info_wnd, Y_INFO + 2, 0, "%08d", 9999);
  //mvwaddch(info_wnd, 4, 0, ACS_CKBOARD);
  mvwprintw(info_wnd, Y_INFO + 9, 0, "%d", game_speed);
  mvwprintw(info_wnd, Y_INFO + 11, 0, "%d", game_level);
  mvwprintw(info_wnd, Y_INFO + 13, 0, "OFF");
  mvwprintw(info_wnd, Y_INFO + 15, 0, "PAUSE");
  wattroff(info_wnd, COLOR_PAIR(COLOR_WHITE));
  
  // initial draw
  wrefresh(main_wnd);
  wrefresh(game_wnd);
  wrefresh(label_wnd);
  wrefresh(info_wnd);
  
  return 0;
}

void run()
{
  uint_fast8_t tick = 0;
  int in_char;
  bool exit_requested = false;
  bool request_obj = true;
  vec2i pos;
  vec2i min_pos;
  vec2i max_pos;
  vec2i info_pos;
  vec2i nmin_pos;
  vec2i nmax_pos;
  int_fast8_t rand_x = 0;
  
  while(1)
  {
    // Clear all game_wnd
    werase(game_wnd);
    // Clear info_wnd NEXT
    mvwprintw(info_wnd, Y_INFO + 4, 0, "%36s", " ");

    pos = current_obj->get_pos();
    min_pos = current_obj->get_min_pos();
    max_pos = current_obj->get_max_pos();
    // Drop object from top to bottom
    if (tick == 100)
    {
      if (pos.y + max_pos.y + 1 < GAME_HEIGHT)
      {
        pos.y += 1;
      }
      else
      {
        request_obj = true;
      }
    }
    // Change from local to global coordinate
    min_pos.x += pos.x;
    min_pos.y += pos.y;
    max_pos.x += pos.x + 2;
    max_pos.y += pos.y + 1;

    if (request_obj)
    {
      rand_x = 2*(rand() % 10); // 0~18
      current_obj->init_data(next_obj->get_type());
      min_pos = current_obj->get_min_pos();
      max_pos = current_obj->get_max_pos();
      //mvwprintw(label_wnd, Y_INFO + 17, 0, "%2d %2d", rand_x, max_pos.x);
      if (rand_x + max_pos.x + 2 > GAME_WIDTH) rand_x = GAME_WIDTH - max_pos.x - 2;
      //mvwprintw(label_wnd, Y_INFO + 18, 0, "%2d", rand_x);
      // Update new pos
      pos.x = rand_x;
      pos.y = -max_pos.y;
      // Change from local to global coordinate
      min_pos.x += pos.x;
      min_pos.y += pos.y;
      max_pos.x += pos.x + 2;
      max_pos.y += pos.y + 1;
      //if (max_pos.x < GAME_WIDTH)
      next_obj->init_data((object_type)(rand() % 5));
      next_obj->set_pos({0, Y_INFO + 4});
      info_pos = next_obj->get_pos();
      nmin_pos = next_obj->get_min_pos();
      nmax_pos = next_obj->get_max_pos();
      // Change from local to global coordinate
      nmin_pos.x += info_pos.x;
      nmin_pos.y += info_pos.y;
      nmax_pos.x += info_pos.x + 2;
      nmax_pos.y += info_pos.y + 1;
      // Move to the center of info_wnd
      info_pos.x += 4-(nmax_pos.x-nmin_pos.x)/2;
      info_pos.y += 2-(nmax_pos.y-nmin_pos.y)/2;
      next_obj->set_pos(info_pos);
      //mvwprintw(info_wnd, Y_INFO + 17, 0, "%2d %2d", info_pos.x, info_pos.y);
      //mvwprintw(info_wnd, Y_INFO + 18, 0, "%2d %2d", nmax_pos.x, nmax_pos.y);
      request_obj = false;
    }

    // Keyboard handle
    in_char = wgetch(main_wnd);
    in_char = tolower(in_char);
    switch(in_char)
    {
      case 'q':
        exit_requested = true;
        break;
      case KEY_UP:
      case 'w':
        if (min_pos.y > 0)
          pos.y -= 1;
        break;
      case KEY_DOWN:
      case 's':
        if (max_pos.y < GAME_HEIGHT)
          pos.y += 1;
        break;
      case KEY_LEFT:
      case 'a':
        if (min_pos.x > 0)
          pos.x -= 2;
        break;
      case KEY_RIGHT:
      case 'd':
        if (max_pos.x < GAME_WIDTH)
          pos.x += 2;
        break;
      case 'p':
        if (max_pos.x <= GAME_WIDTH)
          request_obj = true;
        break;
      case 'r':
        current_obj->rotate();
      default:
        break;
    }

    if (exit_requested) break;

    draw_object(info_wnd, next_obj);

    current_obj->set_pos(pos);
    mvwprintw(info_wnd, Y_INFO + 17, 0, "%2d %2d", pos.x, pos.y);
    mvwprintw(info_wnd, Y_INFO + 18, 0, "%2d %2d", max_pos.x, max_pos.y);
    draw_object(game_wnd, current_obj);

    // refresh all
    wrefresh(main_wnd);
    wrefresh(game_wnd);
    wrefresh(label_wnd);
    wrefresh(info_wnd);

    usleep(10000); // 10 ms

    tick +=1;
    if (tick >= 101) tick = 0;
  }
}

void close()
{
  delwin(main_wnd);
  delwin(game_wnd);
  delwin(label_wnd);
  delwin(info_wnd);
  endwin();
}
