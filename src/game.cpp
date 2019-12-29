//
//  game.cpp
//  
//
//  Created by Vuong Le Quoc on 2019/05/06.
//

#if defined(_WIN32)
#include <curses.h>
#include <windows.h> // usleep
#else
#include <ncurses.h>
#include <unistd.h> // usleep
#endif

#include <string> // std::string
#include <stdlib.h> // rand
#include <time.h>

#include "game.h"
#include "define.h"
#include "object.h"

// Check terminal has color or not
bool has_color = false;

WINDOW* main_wnd;
WINDOW* game_wnd;
WINDOW* label_wnd;
WINDOW* info_wnd;
WINDOW* panel_wnd;

rect screen_area;
rect game_area;
rect label_area;
rect info_area;
rect panel_area;

game_object* current_obj = NULL;
game_object* next_obj = NULL;
struct table_cell table_obj[GAME_WIDTH/2][GAME_HEIGHT];

uint_fast16_t game_score = 0;
uint_fast8_t game_speed = 1;
uint_fast8_t game_level = 1;
bool game_sound = false;
game_state game_status = GAME_OVER;

void mySleep(int sleepMs)
{
  #if defined(_WIN32)
    Sleep(sleepMs);
  #else
    usleep(sleepMs*1000);
  #endif
}

void draw_object(WINDOW* wnd, game_object* obj)
{
  vec2i pos = obj->get_pos();
  uint_fast8_t color = obj->get_color();
  vec2ui* data = obj->get_data();
  wattron(wnd, COLOR_PAIR(color));
  for (size_t i = 0; i < 4; i++)
  {
    mvwaddch(wnd, pos.y+data[i].y, 2*(pos.x+data[i].x), ' '|A_REVERSE);
    mvwaddch(wnd, pos.y+data[i].y, 2*(pos.x+data[i].x)+1, ' '|A_REVERSE);
  }
  wattroff(wnd, COLOR_PAIR(color));
}

void draw_table(WINDOW* wnd)
{
  uint_fast8_t color = BLACK_PAIR;
  bool has_data = false;
  for (size_t j = 0; j < GAME_HEIGHT; j++)
  {
    for (size_t i = 0; i < GAME_WIDTH/2; i++)
    {
      color = table_obj[i][j].color;
      has_data = table_obj[i][j].has_data;
      if (has_data)
      {
        wattron(wnd, COLOR_PAIR(color));
        mvwaddch(wnd, j, 2*i, ' '|A_REVERSE);
        mvwaddch(wnd, j, 2*i+1, ' '|A_REVERSE);
        wattroff(wnd, COLOR_PAIR(color));
      }
      else
      {
        // Clear cell
        mvwaddch(wnd, j, 2*i, '`');
        mvwaddch(wnd, j, 2*i+1, ' ');
      }
    }
  }
}

bool check_touch(game_object* obj, vec2i pos_change)
{
  bool is_touch = false;
  vec2i table_cell;
  for (size_t i = 0; i < 4; i++)
      {
    table_cell.x = obj->get_pos().x+obj->get_data()[i].x+pos_change.x;
    table_cell.y = obj->get_pos().y+obj->get_data()[i].y+pos_change.y;
    if ((table_cell.y >= 0) && (table_obj[table_cell.x][table_cell.y].has_data))
        {
      is_touch = true;
      break;
        }
      }
  return is_touch;
}

void update_table(game_object* obj)
{
  vec2i table_cell;
  for (size_t i = 0; i < 4; i++)
  {
    table_cell.x = obj->get_pos().x+obj->get_data()[i].x;
    table_cell.y = obj->get_pos().y+obj->get_data()[i].y;
    if (table_cell.y >= 0)
      table_obj[table_cell.x][table_cell.y].update(obj->get_color(), true);
  }
}

void reset_table()
{
  for (size_t j = 0; j < GAME_HEIGHT; j++)
  {
    for (size_t i = 0; i < GAME_WIDTH/2; i++)
    {
      table_obj[i][j].update(BLACK_PAIR, false);
    }
  }
}

void delete_row(uint_fast8_t row)
{
  for (size_t j = row; j > 0; j--)
  {
    for (size_t i = 0; i < GAME_WIDTH/2; i++)
    {
      if (j > 0)
      {
        // Copy data of above cell
        table_obj[i][j].update(table_obj[i][j-1].color, table_obj[i][j-1].has_data);
      }
      else
      {
        table_obj[i][j].has_data = false;
      }
    }
  }
}

uint_fast8_t scoring()
{
  uint_fast8_t score = 0;
  uint_fast8_t row_count = 0;
  // Check full row in table_obj
  for (size_t j = 0; j < GAME_HEIGHT; j++)
  {
    row_count = 0;
    for (size_t i = 0; i < GAME_WIDTH/2; i++)
    {
      if (table_obj[i][j].has_data)
      {
        row_count += 1;
      }
    }
    if (row_count == GAME_WIDTH/2)
    {
      score += 1;
      // delete this row
      delete_row(j);
    }
  }
  return score;
}

void update_score(WINDOW* wnd, uint_fast16_t score)
{
  mvwprintw(wnd, Y_INFO, 0, "%08d", score);
}

void update_speed(WINDOW* wnd, uint_fast16_t speed)
{
  mvwprintw(wnd, Y_INFO + 9, 0, "%d", speed);
}

void update_level(WINDOW* wnd, uint_fast16_t level)
{
  mvwprintw(wnd, Y_INFO + 11, 0, "%d", level);
}

void update_status(WINDOW* wnd, game_state status)
{
  if (status == GAME_OVER)
    mvwprintw(wnd, Y_INFO + 15, 0, "%s", "OVER   ");
  else if (status == GAME_PAUSE)
    mvwprintw(wnd, Y_INFO + 15, 0, "%s", "PAUSING");
  else
    mvwprintw(wnd, Y_INFO + 15, 0, "%s", "PLAYING");
}

void game_over_text(WINDOW* wnd)
{
  wattron(wnd, COLOR_PAIR(COLOR_WHITE));
  mvwprintw(wnd, GAME_HEIGHT/2-1, 0, "%s", "     GAME  OVER     ");
  mvwprintw(wnd, GAME_HEIGHT/2,   0, "%s", " PRESS R TO RESTART ");
  wattroff(wnd, COLOR_PAIR(COLOR_WHITE));
}

bool new_game()
{
  game_score = 0;
  game_speed = 1;
  game_level = 1;
  update_score(info_wnd, game_score);
  game_status = GAME_PLAYING;
  update_status(info_wnd, game_status);
  reset_table();
  return true;
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
  /*
  ******************************************
  *01234567890123456789*012345678*012345678*
  *1                   *         *         *
  *2                   *         *         *
  *3                   *         *         *
  *4                   *         *         *
  *5                   *         *         *
  *6                   *         *         *
  *7                   *         *         *
  *8                   *         *         *
  *9                   *         *         *
  *0       GAME        *  LABEL  *  INFO   *
  *1                   *         *         *
  *2                   *         *         *
  *3                   *         *         *
  *4                   *         *         *
  *5                   *         *         *
  *6                   *         *         *
  *7                   *         *         *
  *8                   *         *         *
  *9                   *         *         *
  ******************************************
  *0                                       *
  *1                                       *
  *2                                       *
  *3                                       *
  *4                 PANEL                 *
  *5                                       *
  *6                                       *
  *7                                       *
  *8                                       *
  ******************************************
  */
  //                   W x H
  // all screen area  42 x 32
  //   game area      20 x 20
  //   label area      9 x 20
  //   info area       9 x 20
  //   panel area     40 x  9
  screen_area.update(0, 0, 1+GAME_WIDTH+1+LABEL_WIDTH+1+INFO_WIDTH+1, 1+GAME_HEIGHT+1+PANEL_HEIGHT+1);
  game_area.update(1, 1, GAME_WIDTH, GAME_HEIGHT);
  label_area.update(1+GAME_WIDTH+1, 1, LABEL_WIDTH, GAME_HEIGHT);
  info_area.update(1+GAME_WIDTH+1+LABEL_WIDTH+1, 1, INFO_WIDTH, GAME_HEIGHT);
  panel_area.update(1, 1+GAME_HEIGHT+1, GAME_WIDTH+1+LABEL_WIDTH+1+INFO_WIDTH, PANEL_HEIGHT);
  
  // multi windows
  // newwin nlines, ncols, begin_y, begin_x
  main_wnd = newwin(screen_area.height(), screen_area.width(), screen_area.top(), screen_area.left());
  game_wnd = newwin(game_area.height(), game_area.width(), game_area.top(), game_area.left());
  label_wnd = newwin(label_area.height(), label_area.width(), label_area.top(), label_area.left());
  info_wnd = newwin(info_area.height(), info_area.width(), info_area.top(), info_area.left());
  panel_wnd = newwin(panel_area.height(), panel_area.width(), panel_area.top(), panel_area.left());
  
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
  wbkgd(game_wnd, COLOR_PAIR(WHITE_PAIR));
  wbkgd(label_wnd, COLOR_PAIR(WHITE_PAIR));
  wbkgd(info_wnd, COLOR_PAIR(WHITE_PAIR));
  wbkgd(panel_wnd, COLOR_PAIR(WHITE_PAIR));
  
  // enable function keys
  keypad(game_wnd, true);
  
  // disable input blocking
  nodelay(game_wnd, true);
  
  // frame around screen
  wattron(main_wnd, A_BOLD); // active attribute for drawing
  box(main_wnd, 0, 0); // draw a bouding box around main_wnd
  mvwprintw(main_wnd, 0, (GAME_WIDTH+2)/2-5, "%s", "[ TETRIS ]");
  wattroff(main_wnd, A_BOLD); // inactive attribute for drawing
  
  // dividing line
  wmove(main_wnd, game_area.top(), game_area.right());
  wvline(main_wnd, ACS_VLINE, game_area.height());
  wmove(main_wnd, game_area.bottom(), game_area.left());
  whline(main_wnd, ACS_HLINE, panel_area.width());
  
  // game info
  // SCORE
  // HI-SCORE
  // NEXT
  // SPEED
  // LEVEL
  // SOUND
  // STATUS
  mvwprintw(label_wnd, Y_INFO, 1, "%8s", "SCORE");
  mvwprintw(label_wnd, Y_INFO + 2, 1, "%8s", "HI-SCORE");
  mvwprintw(label_wnd, Y_INFO + 4, 1, "%8s", "NEXT");
  mvwprintw(label_wnd, Y_INFO + 9, 1, "%8s", "SPEED");
  mvwprintw(label_wnd, Y_INFO + 11, 1, "%8s", "LEVEL");
  mvwprintw(label_wnd, Y_INFO + 13, 1, "%8s", "SOUND");
  mvwprintw(label_wnd, Y_INFO + 15, 1, "%8s", "STATUS");
  
  update_score(info_wnd, game_score);
  mvwprintw(info_wnd, Y_INFO + 2, 0, "%08d", 9999);
  mvwprintw(info_wnd, Y_INFO + 13, 0, "OFF");
  update_speed(info_wnd, game_speed);
  update_level(info_wnd, game_level);
  game_status = GAME_PLAYING;
  update_status(info_wnd, game_status);
  
  // draw panel
  mvwaddch(panel_wnd, 1, 8, ACS_UARROW);
  mvwaddch(panel_wnd, 2, 8, 'w');
  mvwaddch(panel_wnd, 4, 2, ACS_LARROW);
  mvwaddch(panel_wnd, 4, 4, 'a');
  mvwaddch(panel_wnd, 4, 14, ACS_RARROW);
  mvwaddch(panel_wnd, 4, 12, 'd');
  mvwaddch(panel_wnd, 6, 8, 's');
  mvwaddch(panel_wnd, 7, 8, ACS_DARROW);
  
  mvwaddch(panel_wnd, 2, 26, ACS_ULCORNER);
  mvwaddch(panel_wnd, 2, 34, ACS_URCORNER);
  mvwprintw(panel_wnd, 4, 28, "%s", "SPACE");
  mvwaddch(panel_wnd, 6, 26, ACS_LLCORNER);
  mvwaddch(panel_wnd, 6, 34, ACS_LRCORNER);
  
  // initial draw
  wrefresh(main_wnd);
  wrefresh(game_wnd);
  wrefresh(label_wnd);
  wrefresh(info_wnd);
  wrefresh(panel_wnd);
  
  // Init game object
  srand(time(NULL));
  current_obj = new game_object();
  next_obj = new game_object((object_type)(rand() % 5));
  
  return 0;
}

void run()
{
  uint_fast8_t tick = 0;
  uint_fast8_t obj_count = 0;
  int in_char;
  bool exit_requested = false;
  bool request_obj = true;
  vec2i cur_pos = {0,0};
  vec2i min_pos = {0,0};
  vec2i max_pos = {0,0};
  vec2i center = {0,0};
  vec2i rotate_change = {0,0};
  uint_fast8_t cur_score = 0;
  int_fast8_t rand_x = 0;
  
  while (1)
  {
    // Loop in case PAUSE/OVER
    if (game_status != GAME_PLAYING)
    {
      in_char = wgetch(game_wnd);
      in_char = tolower(in_char);
      switch(in_char)
      {
        case 'q':
          exit_requested = true;
          break;
        case 'p':
          if (game_status == GAME_PAUSE)
          {
            game_status = GAME_PLAYING;
            update_status(info_wnd, game_status);
          }
          break;
        case 'r':
          if (game_status == GAME_OVER)
            request_obj = new_game();
          break;
        default:
          break;
      }
      if (exit_requested) break;

      mySleep(100); // 100 ms
      continue;
    }

    /*
     * Create new obj
     */
    if (request_obj)
    {
      // Get new data
      //  Update current obj by next obj
      current_obj->init_data(next_obj->get_type());
      max_pos = current_obj->get_max_pos();
      // Random x position
      rand_x = rand() % 10; // 0~9
      if (rand_x + max_pos.x + 1 > GAME_WIDTH/2)
        rand_x = GAME_WIDTH/2 - max_pos.x - 1;
      // Update new cur_pos
      cur_pos.x = rand_x;
      cur_pos.y = -max_pos.y-1; // should start over screen
      current_obj->set_pos(cur_pos);

      //  Change next obj with random obj type
      next_obj->init_data((object_type)(rand() % 5));
      min_pos = next_obj->get_min_pos();
      max_pos = next_obj->get_max_pos();
      // Move to the center of info_wnd
      cur_pos.x = 2-(max_pos.x-min_pos.x+1)/2;
      cur_pos.y = 2-(max_pos.y-min_pos.y+1)/2 + Y_INFO + 4;
      next_obj->set_pos(cur_pos);

      // Clear info_wnd NEXT
      mvwprintw(info_wnd, Y_INFO + 4, 0, "%36s", " ");
      // Draw next obj
      draw_object(info_wnd, next_obj);

      request_obj = false;

      obj_count += 1 ;
      if (obj_count%SPEED_COND == 0) {
        game_speed += 1;
        update_speed(info_wnd, game_speed);
      }
      if (obj_count%LEVEL_COND == 0) {
        game_level += 1;
        obj_count = 0;
        update_level(info_wnd, game_level);
      }
    }

    /*
     * Move down current obj
     * Update table_obj
     * Check GAME OVER
     */
    cur_pos = current_obj->get_pos();
    min_pos = current_obj->get_min_pos();
    max_pos = current_obj->get_max_pos();
    // Drop object from top to bottom
    if (tick == (uint_fast8_t)(TICK_COND/game_speed))
    {
      // Check touch
      if ((cur_pos.y + max_pos.y + 1 < GAME_HEIGHT) &&
          (check_touch(current_obj, {0, 1}) == false))
      {
        cur_pos.y += 1;
      }
      else
      {
        if (cur_pos.y < 0)
        {
          game_status = GAME_OVER;
          update_status(info_wnd, GAME_OVER);
        }
        else
        {
          // Current obj touch table_obj
          update_table(current_obj);
          cur_score = scoring();
          if (cur_score > 0)
          {
            game_score += cur_score;
            update_score(info_wnd, game_score);
          }
          request_obj = true;
        }
      }
    }
    // Clear all game_wnd
    werase(game_wnd);
    draw_table(game_wnd);
    if (game_status == GAME_OVER)
      game_over_text(game_wnd);

    /*
     * Keyboard handle game play button
     * q: Quit
     * p: Pausing/Playing
     * r: New game
     * KEY_UP/w/space: Rotate
     * KEY_DOWN/s: Move down
     * KEY_LEFT/a: Move left
     * KEY_RIGHT/d: Mode right
     */
    in_char = wgetch(game_wnd);
    in_char = tolower(in_char);
    switch(in_char)
    {
      case 'q':
        exit_requested = true;
        break;
      case 'p':
        if (game_status == GAME_PLAYING)
          game_status = GAME_PAUSE;
        else if (game_status == GAME_PAUSE)
          game_status = GAME_PLAYING;
        update_status(info_wnd, game_status);
        break;
      case 'r':
        if (game_status == GAME_OVER)
          request_obj = new_game();
        break;
      case KEY_UP:
      case 'w':
      case ' ':
        if (game_status == GAME_PLAYING)
        {
          // Try to rotate, will recover back later if touch table
          center = current_obj->get_center();
          current_obj->rotate();
          // Update cur_pose to avoid over screen and touch table_obj
          cur_pos = current_obj->get_pos();
          min_pos = current_obj->get_min_pos();
          max_pos = current_obj->get_max_pos();
          // keep center
          cur_pos.x += center.x - current_obj->get_center().x;
          cur_pos.y += center.y - current_obj->get_center().y;
          // over left screen
          if (cur_pos.x + min_pos.x < 0)
            cur_pos.x = -min_pos.x;
          // over right screen
          if (cur_pos.x + max_pos.x + 1 > GAME_WIDTH/2)
            cur_pos.x = GAME_WIDTH/2 - max_pos.x - 1;
          // over top screen
          if (cur_pos.y + min_pos.y < 0)
            cur_pos.y = -min_pos.y;
          // over bottom screen
          if (cur_pos.y + max_pos.y + 1 > GAME_HEIGHT)
            cur_pos.y = GAME_HEIGHT - max_pos.y - 1;
          // avoid touch
          rotate_change.x = cur_pos.x-current_obj->get_pos().x;
          rotate_change.y = cur_pos.y-current_obj->get_pos().y;
          if (check_touch(current_obj, rotate_change)) {
            // Recover data of current_obj and cur_pos value
            current_obj->rotate_back();
            cur_pos = current_obj->get_pos();
          }
          current_obj->set_pos(cur_pos);
        }
        break;
      case KEY_DOWN:
      case 's':
        if ((game_status == GAME_PLAYING) &&
            (check_touch(current_obj, {0, 1}) == false) &&
            (cur_pos.y + max_pos.y + 1 < GAME_HEIGHT))
          cur_pos.y += 1;
        break;
      case KEY_LEFT:
      case 'a':
        if ((game_status == GAME_PLAYING) &&
            (check_touch(current_obj, {-1, 0}) == false) &&
            (cur_pos.x + min_pos.x > 0))
          cur_pos.x -= 1;
        break;
      case KEY_RIGHT:
      case 'd':
        if ((game_status == GAME_PLAYING) &&
            (check_touch(current_obj, {1, 0}) == false) &&
            (cur_pos.x + max_pos.x + 1 < GAME_WIDTH/2))
          cur_pos.x += 1;
        break;
      default:
        break;
    }
    if (exit_requested) break;

    // Update current obj
    current_obj->set_pos(cur_pos);
    //mvwprintw(info_wnd, Y_INFO + 17, 0, "%2d %2d", cur_pos.x, cur_pos.y);
    draw_object(game_wnd, current_obj);
    wrefresh(game_wnd);
    wrefresh(info_wnd);

    mySleep(10); // 10 ms

    if (game_status == GAME_PLAYING)
      tick +=1;
    if (tick >= (uint_fast8_t)(TICK_COND/game_speed)+1)
      tick = 0;
  }
}

void close()
{
  delwin(main_wnd);
  delwin(game_wnd);
  delwin(label_wnd);
  delwin(info_wnd);
  delwin(panel_wnd);
  endwin();
}
