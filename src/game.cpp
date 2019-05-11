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

rect screen_area;
rect game_area;
rect label_area;
rect info_area;

game_object* current_obj = NULL;
game_object* next_obj = NULL;
struct table_cell table_obj[GAME_WIDTH/2][GAME_HEIGHT];

uint_fast16_t game_score = 0;
uint_fast8_t game_speed = 1;
uint_fast8_t game_level = 1;
bool game_sound = false;
game_state game_status = GAME_OVER;

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
        mvwaddch(wnd, j, 2*i, ' ');
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
  wattron(info_wnd, COLOR_PAIR(COLOR_WHITE));
  mvwprintw(wnd, Y_INFO, 0, "%08d", score);
  wattroff(info_wnd, COLOR_PAIR(COLOR_WHITE));
}

void update_status(WINDOW* wnd, game_state status)
{
  wattron(wnd, COLOR_PAIR(COLOR_WHITE));
  if (status == GAME_OVER)
    mvwprintw(wnd, Y_INFO + 15, 0, "%s", "OVER   ");
  else if (status == GAME_PAUSE)
    mvwprintw(wnd, Y_INFO + 15, 0, "%s", "PAUSING");
  else
    mvwprintw(info_wnd, Y_INFO + 15, 0, "%s", "PLAYING");
  wattroff(wnd, COLOR_PAIR(COLOR_WHITE));
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
  
  update_score(info_wnd, game_score);
  wattron(info_wnd, COLOR_PAIR(COLOR_WHITE));
  mvwprintw(info_wnd, Y_INFO + 2, 0, "%08d", 9999);
  //mvwaddch(info_wnd, 4, 0, ACS_CKBOARD);
  mvwprintw(info_wnd, Y_INFO + 9, 0, "%d", game_speed);
  mvwprintw(info_wnd, Y_INFO + 11, 0, "%d", game_level);
  mvwprintw(info_wnd, Y_INFO + 13, 0, "OFF");
  wattroff(info_wnd, COLOR_PAIR(COLOR_WHITE));
  
  // initial draw
  wrefresh(main_wnd);
  wrefresh(game_wnd);
  wrefresh(label_wnd);
  wrefresh(info_wnd);
  
  // Init game object
  srand(time(NULL));
  current_obj = new game_object();
  next_obj = new game_object((object_type)(rand() % 5));
  
  game_status = GAME_PLAYING;
  update_status(info_wnd, game_status);

  return 0;
}

void run()
{
  uint_fast8_t tick = 0;
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
    if (game_status == GAME_OVER)
    {
      in_char = wgetch(main_wnd);
      in_char = tolower(in_char);
      if (in_char == 'r') request_obj = new_game();
      usleep(1000); // 10 ms
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
    if (tick == 100)
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
    in_char = wgetch(main_wnd);
    in_char = tolower(in_char);
    switch(in_char)
    {
      case 'q':
        exit_requested = true;
        break;
      case 'p':
        //if (max_pos.x <= GAME_WIDTH/2)
        //  request_obj = true;
        if (game_status == GAME_PLAYING) game_status = GAME_PAUSE;
        else if (game_status == GAME_PAUSE) game_status = GAME_PLAYING;
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
    //mvwprintw(info_wnd, Y_INFO + 18, 0, "%2d %2d", center.x, center.y);
    draw_object(game_wnd, current_obj);

    // refresh all
    //wrefresh(main_wnd);
    wrefresh(game_wnd);
    wrefresh(label_wnd);
    wrefresh(info_wnd);

    usleep(10000); // 10 ms

    if (game_status == GAME_PLAYING)
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
