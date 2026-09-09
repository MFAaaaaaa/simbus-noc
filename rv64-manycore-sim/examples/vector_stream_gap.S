    .section .text.init
    .globl _start
    .type _start, @function
_start:
    csrr a0, mhartid
    slli t4, a0, 8
    li t0, 0x2000
    add t0, t0, t4

    li s0, 8
    .word 0x010474d7       # vsetvli s1,s0,e32,m1,tu,mu
    li s2, 64
.Lstream:
    .word 0x0202e107       # vle32.v v2,(t0)
    .word 0x0220b157       # vadd.vi v2,v2,1
    .word 0x0202e127       # vse32.v v2,(t0)
    li t3, 16
.Lcompute_gap:
    addi t3, t3, -1
    bnez t3, .Lcompute_gap
    addi s2, s2, -1
    bnez s2, .Lstream

    lw t1, 0(t0)
    lw t2, 28(t0)
    li t3, 64
    bne t1, t3, .Lfail
    bne t2, t3, .Lfail

.Lpass:
    la t5, tohost
    li t6, 1
    sd t6, 0(t5)
.Lwait:
    j .Lwait

.Lfail:
    la t5, tohost
    addi t6, a0, 1
    slli t6, t6, 1
    ori t6, t6, 1
    sd t6, 0(t5)
    j .Lwait

    .size _start, .-_start

    .section .tohost,"aw",@progbits
    .balign 8
    .globl tohost
tohost:
    .dword 0
