#ifndef TIMER_H
#define TIMER_H

#include "types.h"

#define TIMER_HZ 100  // 100 ticks per second

void timer_init(void);
uint64_t timer_get_ticks(void);
uint64_t timer_get_seconds(void);
void timer_sleep(uint64_t ms);

#endif
