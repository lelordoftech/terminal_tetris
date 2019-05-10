//
//  object.cpp
//  
//
//  Created by Vuong Le Quoc on 2019/05/08.
//

#include "object.h"

game_object::game_object(object_type type)
{
  this->pos.x = 0;
  this->pos.y = 0;
  this->rotate_dir = 0;
  
  init_data(type);
}

void game_object::init_data(object_type type)
{
  this->type = type;
  switch(type)
  {
    case L:
      this->color = BLUE_PAIR;
      this->data[0] = {0,0};
      this->data[1] = {0,1};
      this->data[2] = {0,2};
      this->data[3] = {1,2};
      break;
    case T:
      this->color = MAGENTA_PAIR;
      this->data[0] = {1,0};
      this->data[1] = {0,1};
      this->data[2] = {1,1};
      this->data[3] = {1,2};
      break;
    case O:
      this->color = YELLOW_PAIR;
      this->data[0] = {0,0};
      this->data[1] = {0,1};
      this->data[2] = {1,0};
      this->data[3] = {1,1};
      break;
    case l:
      this->color = CYAN_PAIR;
      this->data[0] = {0,0};
      this->data[1] = {0,1};
      this->data[2] = {0,2};
      this->data[3] = {0,3};
      break;
    case s:
      this->color = GREEN_PAIR;
      this->data[0] = {0,0};
      this->data[1] = {0,1};
      this->data[2] = {1,1};
      this->data[3] = {1,2};
      break;
    default:
      break;
  }
}

vec2i game_object::get_min_pos()
{
  vec2i min_pos;
  min_pos.x = this->data[0].x;
  min_pos.y = this->data[0].y;
  for (uint_fast8_t i = 1; i < 4; i++) {
    if (this->data[i].x < min_pos.x)
    {
      min_pos.x = this->data[i].x;
    }
    if (this->data[i].y < min_pos.y)
    {
      min_pos.y = this->data[i].y;
    }
  }
  return min_pos;
}

vec2i game_object::get_max_pos()
{
  vec2i max_pos;
  max_pos.x = this->data[0].x;
  max_pos.y = this->data[0].y;
  for (uint_fast8_t i = 1; i < 4; i++) {
    if (this->data[i].x > max_pos.x)
    {
      max_pos.x = this->data[i].x;
    }
    if (this->data[i].y > max_pos.y)
    {
      max_pos.y = this->data[i].y;
    }
  }
  return max_pos;
}

void game_object::rotate()
{
  // TODO
}
