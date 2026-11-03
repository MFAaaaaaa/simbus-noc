    .section .text.init
    .globl _start
_start:
    csrr a0, mhartid
    slli t0, a0, 8
    li s0, 0x6000
    add s0, s0, t0
    mv s1, s0
    addi s2, a0, 41
    li s3, 4
    li t0, 8
    csrw 0x7c5, t0      # BUS_SIZE
.Lburst:
    csrw 0x7c0, s1      # BUS_ADDR
    csrw 0x7c2, s2      # BUS_WDATA
    li t0, 2
    csrw 0x7c1, t0      # BUS_CMD WRITE, no polling between commands
    addi s1, s1, 8
    addi s3, s3, -1
    bnez s3, .Lburst
    fence              # drain all four transactions, then fixed execution delay
    csrr t0, 0x7c4
    andi t0, t0, 5
    bnez t0, .Lfail
    mv s1, s0
    li s3, 4
.Lcheck:
    ld t0, 0(s1)
    bne t0, s2, .Lfail
    li t0, 1
    csrw 0x7c7, t0      # POP_RESULT: free a completion slot
    addi s1, s1, 8
    addi s3, s3, -1
    bnez s3, .Lcheck
    csrw 0x7c0, s0
    li t0, 1
    csrw 0x7c1, t0      # BUS_CMD READ
.Lpoll:
    csrr t0, 0x7c4
    andi t1, t0, 1
    bnez t1, .Lpoll
    andi t0, t0, 4
    bnez t0, .Lfail
    csrr t0, 0x7c3      # BUS_RDATA
    bne t0, s2, .Lfail
    li t0, 1
    csrw 0x7c7, t0
    la t5, tohost
    li t6, 1
    sd t6, 0(t5)
.Lwait:
    j .Lwait
.Lfail:
    la t5, tohost
    li t6, 3
    sd t6, 0(t5)
    j .Lwait
    .section .tohost,"aw",@progbits
    .balign 8
    .globl tohost
tohost: .dword 0
