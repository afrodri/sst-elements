int main() {
  float a = 1.0;
  float b = 2.5;
  float c;

  asm volatile ("move $a0, %0\n"   /* Move 'a' into $a0 */
		"li $v0, 2\n"      /* Set for 'PRINT_FLOAT' syscall */
		"syscall" : : "r" (a));

  asm volatile ("move $a0, %0\n"   /* Move 'a' into $a0 */
		"li $v0, 2\n"      /* Set for 'PRINT_FLOAT' syscall */
		"syscall" : : "r" (b));

  c = a+b;

  asm volatile ("move $a0, %0\n"   /* Move 'c' into $a0 */
		"li $v0, 2\n"      /* Set for 'PRINT_FLOAT' syscall */
		"syscall" : : "r" (c));

  c = a/b;

  asm volatile ("move $a0, %0\n"   /* Move 'c' into $a0 */
		"li $v0, 2\n"      /* Set for 'PRINT_FLOAT' syscall */
		"syscall" : : "r" (c));

  // exit syscall. we have to put this here because, embaressingly, if
  // there is code after the main() function in the file
  // (e.g. soft-float library), we currently start executing that
  // code. So we 'manually' stick in a syscall to exit
  asm volatile ("li $v0, 10\n"      /* Set for 'EXIT' syscall */
		"syscall" : : );

  return 0;
}
