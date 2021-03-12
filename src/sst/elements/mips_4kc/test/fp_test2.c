#include "soft-fp/soft-fp.h"
#include "soft-fp/single.h"

#if defined(__MIPSEL__)
#warning asm missing correct clobbers!
#define PRINT_INT(x)   asm volatile ("move $a0, %0\n"   \
                                     "li $v0, 1\n"      \
				     "syscall" : : "r" (x));
#define PRINT_FLOAT(x) asm volatile ("move $a0, %0\n" \
                                     "li $v0, 2\n"    \
				     "syscall" : : "r" (x));
#else
#include <stdio.h>
#define PRINT_INT(x) printf("PRINT_INT_SYSCALL(pX): %d %u %x\n", x, x, x);
#define PRINT_FLOAT(x) {                                \
  unsigned int *xp = (unsigned int*)&x;                   \
  printf("PRINT_FLOAT_SYSCALL(pX): %f 0x%x\n", x, *xp);   \
  }
#endif


SFtype my_divsf3(SFtype a,SFtype b);

int main() {
  float a = 0.052132f;
  float b = 0.998640f;
  float c;

  PRINT_FLOAT(a);
  PRINT_FLOAT(b);

  //c = a/b;
  c = my_divsf3(a,b);

  PRINT_FLOAT(c);

#if defined(__MIPSEL__) 
  // exit syscall. we have to put this here because, embaressingly, if
  // there is code after the main() function in the file
  // (e.g. soft-float library), we currently start executing that
  // code. So we 'manually' stick in a syscall to exit
  asm volatile ("li $v0, 10\n"      /* Set for 'EXIT' syscall */
		"syscall" : : );
#endif
}
