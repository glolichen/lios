#include <stdbool.h>

#include "io.h"
#include "const.h"

void serial_configure_baud_rate(u16 port, u16 divisor) {
	outb(GET_SERIAL_LINE_COMMAND_PORT(port), SERIAL_LINE_ENABLE_DLAB);
	// serial port only accepts a byte at a time
	// send high byte of divisor then low byte
	outb(GET_SERIAL_DATA_PORT(port), (divisor >> 8) & 0xFF);
	outb(GET_SERIAL_DATA_PORT(port), divisor & 0xFF);
}

void serial_configure_line(u16 port) {
	outb(GET_SERIAL_LINE_COMMAND_PORT(port), 0x3);
}

void serial_configure_buffers(u16 port) {
	outb(GET_SERIAL_FIFO_COMMAND_PORT(port), 0xC7);
}

void serial_configure_modem(u16 port) {
	outb(GET_SERIAL_MODEM_COMMAND_PORT(port), 0x3);
}

bool serial_fifo_empty(u16 port) {
	return (inb(GET_SERIAL_LINE_STATUS_PORT(port)) & (1 << 5));
}

void serial_init() {
	serial_configure_buffers(SERIAL_COM1_BASE);
	serial_configure_line(SERIAL_COM1_BASE);
	serial_configure_modem(SERIAL_COM1_BASE);

	serial_configure_baud_rate(SERIAL_COM1_BASE, 3);
	serial_configure_line(SERIAL_COM1_BASE);

	while (!serial_fifo_empty(SERIAL_COM1_BASE));
}

void serial_putchar(char c) {
	while (!serial_fifo_empty(SERIAL_COM1_BASE));
	outb(GET_SERIAL_DATA_PORT(SERIAL_COM1_BASE), c);
}