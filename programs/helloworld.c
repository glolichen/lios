int main(void) {
	const char *msg = "hello world!\n";
	unsigned long long length = 14;

	asm volatile("mov rax, 1; mov rdi, 1");
	asm volatile("mov rsi, %0" :: "rm"(msg));
	asm volatile("mov rdx, %0" :: "rm"(length));
	asm volatile("syscall");

	return 0;
}
