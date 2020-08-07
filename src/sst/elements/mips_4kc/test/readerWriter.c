typedef unsigned int uint32_t;

volatile uint32_t mailbox[4] = {0,0,0,0};
uint32_t scratch[32];

int main() {
    // find out who I am
    uint32_t proc_num;
    asm volatile ("li $v0, 11\n"      /* Set for 'GET_PROC_NUM_SYSCALL' syscall */
                  "syscall\n"
                  "addi $0, r2, 0" : "=r" (proc_num) : : "v0");

    // print proc_num
    asm volatile ("move $a0, %0\n"   /* Move 'proc_num' into $a0 */
                  "li $v0, 1\n"      /* Set for 'PRINT_INT' syscall */
                  "syscall" : : "r" (proc_num) : "v0");

    if (proc_num) {
        // "reader"
        while(mailbox[0] == 0) ;  // wait for data to be written
        
        // read & print contents
        for (int i = 0; i <= 3; ++i) {
            asm volatile ("move $a0, %0\n"   /* Move 'proc_num' into $a0 */
                          "li $v0, 1\n"      /* Set for 'PRINT_INT' syscall */
                          "syscall" : : "r" (mailbox[i]) : "v0");
        }            
        
        mailbox[0] = 0; // reset 

        while(mailbox[0] == 0) ;  // wait for data to be written again

        // read & print contents
        for (int i = 0; i <= 3; ++i) {
            asm volatile ("move $a0, %0\n"   /* Move 'proc_num' into $a0 */
                          "li $v0, 1\n"      /* Set for 'PRINT_INT' syscall */
                          "syscall" : : "r" (mailbox[i]) : "v0");
        }            

    } else {
        // "writer"
        mailbox[1] = 1;
        mailbox[2] = 2;
        mailbox[3] = 0xdeadbeef;
        mailbox[0] = 1;

        while(mailbox[0] == 1) ;  // wait for data to be read out

        mailbox[1] = 2;
        mailbox[2] = 4;
        mailbox[3] = 5;
        mailbox[0] = 1;
    }
    
    return 0;
}
