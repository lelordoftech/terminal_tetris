//
//  object.h
//  
//
//  Created by Vuong Le Quoc on 2019/05/08.
//

#ifndef object_h
#define object_h

#include "define.h"

struct table_cell {
  uint_fast8_t color;
  bool has_data;
  table_cell() : color(BLACK_PAIR), has_data(false) {};
  void update(uint_fast8_t _color, bool _has_data) {
    color = _color; has_data = _has_data;
  };
};

class game_object
{
protected:
  object_type type;
  vec2i pos;
  //uint_fast8_t rotate_dir;
  uint_fast8_t color;
  vec2ui data[4];
  
public:
  game_object(object_type type=L);
  ~game_object() {};
  
  void init_data(object_type type=L);
  
  object_type get_type() { return type; };
  vec2i get_pos() { return pos; };
  uint_fast8_t get_color() { return color; };
  vec2ui* get_data() { return &data[0]; };
  void set_pos(vec2i npos) { pos = npos; };
  
  vec2i get_min_pos();
  vec2i get_max_pos();
  vec2i get_center();
  
  void rotate();
  void rotate_back();
};

#endif /* object_h */
