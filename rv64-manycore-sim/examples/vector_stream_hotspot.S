    .section .text.init
    .globl _start
    .type _start, @function
_start:
    li t0, 0x2000
    li s0, 8
    .word 0x010474d7       # vsetvli s1,s0,e32,m1,tu,mu
    li s2, 64
.Lstream:
    .word 0x0202e107       # vle32.v v2,(t0)
    .word 0x0220b157       # vadd.vi v2,v2,1
    .word 0x0202e127       # vse32.v v2,(t0)
    addi s2, s2, -1
    bnez s2, .Lstream

    la t5, tohost
    li t6, 1
    sd t6, 0(t5)
.Lwait:
    j .Lwait

    .size _start, .-_start

    .section .tohost,"aw",@progbits
    .balign 8
    .globl tohost
tohost:
    .dword 0
