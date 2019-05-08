//
//  main.cpp
//  
//
//  Created by Vuong Le Quoc on 2019/05/06.
//

#include "game.h"

int main(int argc, char **argv)
{
  int init_status = init();
  
  if (init_status == 0)
  {
    run();
  }
  
  close();
  
  return 0;
}
