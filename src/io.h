#ifndef IO_H
#define IO_H

#include "const.h"

u8 inb(u16 port);
void outb(u16 port, u8 data);
void io_wait();

#endif
