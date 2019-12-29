//
//  define.h
//  
//
//  Created by Vuong Le Quoc on 2019/05/08.
//

#ifndef define_h
#define define_h

#include <stdint.h>

#define GAME_WIDTH 20
#define GAME_HEIGHT 20
#define LABEL_WIDTH 9
#define INFO_WIDTH 9
#define PANEL_HEIGHT 9

#define BLACK_PAIR 0
#define RED_PAIR 1
#define GREEN_PAIR 2
#define YELLOW_PAIR 3
#define BLUE_PAIR 4
#define MAGENTA_PAIR 5
#define CYAN_PAIR 6
#define WHITE_PAIR 7

#define Y_INFO 1 // for draw label and info windows

// delay (ms) = TICK_COND/game_speed
// game_speed =  1 -> delay 50ms
// game_speed = 10 -> delay 5ms
#define TICK_COND 50
// complete 100 object -> increase speed [1~255]
#define SPEED_COND 100
// complete 200 object -> increase level [1~255]
#define LEVEL_COND 200

enum game_state {
  GAME_OVER=0,
  GAME_PAUSE=1,
  GAME_PLAYING=2
};

enum object_type {
  L=0,
  T=1,
  O=2,
  l=3,
  s=4
};

typedef struct {
  uint_fast8_t x;
  uint_fast8_t y;
} vec2ui;

typedef struct {
  int_fast8_t x;
  int_fast8_t y;
} vec2i;

typedef struct {
  vec2ui offset;
  vec2ui bounds;
  
  uint_fast8_t width() { return bounds.x; }
  uint_fast8_t height() { return bounds.y; }
  uint_fast8_t top() { return offset.y; }
  uint_fast8_t left() { return offset.x; }
  uint_fast8_t bottom() { return offset.y + bounds.y; }
  uint_fast8_t right() { return offset.x + bounds.x; }
  
  bool contains(vec2i a) { return (a.x >= offset.x && a.x < right()) &&
    (a.y >= offset.y && a.y < bottom()); }
  
  void update(uint_fast8_t o_x, uint_fast8_t o_y, uint_fast8_t b_x, uint_fast8_t b_y)
  {
    offset.x = o_x;
    offset.y = o_y;
    bounds.x = b_x;
    bounds.y = b_y;
  }
} rect;

#endif /* define_h */
