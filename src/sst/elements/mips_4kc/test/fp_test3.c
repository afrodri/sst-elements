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

typedef unsigned int uint32_t;

int main() {
    uint32_t _FP_DIV_MEAT_1_udiv_norm_y = 0xffa6df00;
    uint32_t _FP_DIV_MEAT_1_udiv_norm_nh = 0x6ac42e8;
    uint32_t _FP_DIV_MEAT_1_udiv_norm_nl = 0;
    uint32_t _FP_DIV_MEAT_1_udiv_norm_q;
    uint32_t _FP_DIV_MEAT_1_udiv_norm_r;

    PRINT_INT(_FP_DIV_MEAT_1_udiv_norm_nh);     
    PRINT_INT(_FP_DIV_MEAT_1_udiv_norm_nl);
    PRINT_INT(_FP_DIV_MEAT_1_udiv_norm_y);
    PRINT_INT(0);
    PRINT_INT(0);
    
    unsigned int __d1, __d0, __q1, __q0;
    unsigned int __r1, __r0, __m;
    __d1 = ((unsigned int) (_FP_DIV_MEAT_1_udiv_norm_y) >> (32 / 2));
    __d0 = ((unsigned int) (_FP_DIV_MEAT_1_udiv_norm_y) & (((unsigned int) 1 << (32 / 2)) - 1));
    __r1 = (_FP_DIV_MEAT_1_udiv_norm_nh) % __d1;
    __q1 = (_FP_DIV_MEAT_1_udiv_norm_nh) / __d1;
    __m = (unsigned int) __q1 * __d0;
    __r1 = __r1 * ((unsigned int) 1 << (32 / 2)) | ((unsigned int) (_FP_DIV_MEAT_1_udiv_norm_nl) >> (32 / 2));
    // good
    
    if (__r1 < __m) {
        __q1--, __r1 += (_FP_DIV_MEAT_1_udiv_norm_y);
        PRINT_INT(96);
        if (__r1 >= (_FP_DIV_MEAT_1_udiv_norm_y))
            if (__r1 < __m) {
                __q1--, __r1 += (_FP_DIV_MEAT_1_udiv_norm_y);
                PRINT_INT(97);
            }
    }
    __r1 -= __m;

    PRINT_INT(__r1);
    PRINT_INT(__d1);
    __r0 = __r1 % __d1;
    PRINT_INT(__r0);
    __q0 = __r1 / __d1;
    PRINT_INT(__q0);
    PRINT_INT(__d0);
    __m = (unsigned int) __q0 * __d0;
    PRINT_INT(__m);
    __r0 = __r0 * ((unsigned int) 1 << (32 / 2)) | ((unsigned int) (_FP_DIV_MEAT_1_udiv_norm_nl) & (((unsigned int) 1 << (32 / 2)) - 1));
    PRINT_INT(__r0);
    
    if (__r0 < __m) {
        PRINT_INT(98);
        __q0--, __r0 += (_FP_DIV_MEAT_1_udiv_norm_y);
        if (__r0 >= (_FP_DIV_MEAT_1_udiv_norm_y))
            if (__r0 < __m) {
                __q0--, __r0 += (_FP_DIV_MEAT_1_udiv_norm_y);
                PRINT_INT(99);
            }
    }
    __r0 -= __m;

    (_FP_DIV_MEAT_1_udiv_norm_q) = (unsigned int) __q1 * ((unsigned int) 1 << (32 / 2)) | __q0;
    (_FP_DIV_MEAT_1_udiv_norm_r) = __r0;

    PRINT_INT(0);
    PRINT_INT(0);
    PRINT_INT(_FP_DIV_MEAT_1_udiv_norm_q);
    PRINT_INT(_FP_DIV_MEAT_1_udiv_norm_r);
}
