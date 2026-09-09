    .section .text.init
    .globl _start
    .type _start, @function
_start:
    csrr t4, mhartid
    slli t4, t4, 8
    li t0, 0x2000
    add t0, t0, t4
    li t2, 0x4000
    add t2, t2, t4

    li t1, 1
    sw t1, 0(t0)
    li t1, 2
    sw t1, 4(t0)
    li t1, 3
    sw t1, 8(t0)
    li t1, 4
    sw t1, 12(t0)

    li s0, 4
    .word 0x010474d7       # vsetvli s1,s0,e32,m1,tu,mu
    .word 0x0202e107       # vle32.v v2,(t0)
    .word 0x0220b257       # vadd.vi v4,v2,1
    .word 0x0203e227       # vse32.v v4,(t2)

    lw a4, 0(t2)
    lw a5, 4(t2)
    lw a6, 8(t2)
    lw a7, 12(t2)
    li t3, 2
    bne a4, t3, .Lfail
    li t3, 3
    bne a5, t3, .Lfail
    li t3, 4
    bne a6, t3, .Lfail
    li t3, 5
    bne a7, t3, .Lfail

    la t5, tohost
    li t6, 1
    sd t6, 0(t5)
.Lwait:
    j .Lwait

.Lfail:
    la t5, tohost
    addi t6, t4, 2
    ori t6, t6, 1
    sd t6, 0(t5)
    j .Lwait

    .size _start, .-_start

    .section .tohost,"aw",@progbits
    .balign 8
    .globl tohost
tohost:
    .dword 0
