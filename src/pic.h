#ifndef PIC_H
#define PIC_H

#include "const.h"

#include "pic.h"

void pic_send_eoi(u8 irq);
void pic_remap(u32 master_offset, u32 slave_offset);
void pic_disable(void);

u16 pic_get_irr(void);
u16 pic_get_isr(void);

void irq_set_mask(u8 IRQline);
void irq_clear_mask(u8 IRQline);

#endif
