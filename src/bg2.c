/* (C) 2019 Harold Tay LGPLv3 */
/*
  Alternate bg task (does nothing).
 */
#include "bg.h"
#include "yield.h"

void bg_task(void) { for ( ; ; ) yield(); }
