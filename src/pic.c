#include "pic.h"
#include "io.h"
#include "const.h"

void pic_send_eoi(uint8_t irq) {
	if(irq >= 8)
		outb(PICS_COMMAND, PIC_EOI);
	outb(PICM_COMMAND,PIC_EOI);
}

#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */
 
#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */
 
/*
arguments:
	offset1 - vector offset for master PIC
		vectors on the master become offset1..offset1+7
	offset2 - same for slave PIC: offset2..offset2+7
*/
void pic_remap(u32 master_offset, u32 slave_offset) {
	u8 a1 = inb(PICM_DATA);                        // save masks
	u8 a2 = inb(PICS_DATA);
 
	outb(PICM_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PICS_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PICM_DATA, master_offset);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PICS_DATA, slave_offset);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PICM_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PICS_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
 
	outb(PICM_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PICS_DATA, ICW4_8086);
	io_wait();
 
	outb(PICM_DATA, a1);   // restore saved masks.
	outb(PICS_DATA, a2);
}

void pic_disable(void) {
    outb(PICM_DATA, 0xff);
    outb(PICS_DATA, 0xff);
}

void irq_set_mask(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;
 
    if (IRQline < 8)
        port = PICM_DATA;
    else {
        port = PICS_DATA;
        IRQline -= 8;
    }
    value = inb(port) | (1 << IRQline);
    outb(port, value);        
}
 
void irq_clear_mask(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;
 
    if (IRQline < 8)
        port = PICM_DATA;
    else {
        port = PICS_DATA;
        IRQline -= 8;
    }
    value = inb(port) & ~(1 << IRQline);
    outb(port, value);        
}

#define PIC_READ_IRR                0x0a    /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR                0x0b    /* OCW3 irq service next CMD read */
 
/* Helper func */
static uint16_t __pic_get_irq_reg(int ocw3) {
    /* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
     * represents IRQs 8-15.  PIC_master is IRQs 0-7, with 2 being the chain */
    outb(PICM_COMMAND, ocw3);
    outb(PICS_COMMAND, ocw3);
    return (inb(PICS_COMMAND) << 8) | inb(PICM_COMMAND);
}
 
/* Returns the combined value of the cascaded PICs irq request register */
uint16_t pic_get_irr(void) {
    return __pic_get_irq_reg(PIC_READ_IRR);
}
 
/* Returns the combined value of the cascaded PICs in-service register */
uint16_t pic_get_isr(void) {
    return __pic_get_irq_reg(PIC_READ_ISR);
}
