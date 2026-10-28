    .section .text.init
    .globl _start
_start:
    csrr a0, mhartid
    li s0, 0x90000000       # local MMIO window, identical address on every hart
    slli t0, a0, 8
    li s1, 0x6000
    add s1, s1, t0         # private target address
    addi s2, a0, 41        # unique expected value
    sd s1, 0(s0)          # TARGET_ADDR
    sd s2, 8(s0)          # WRITE_DATA
    li t0, 8
    sd t0, 16(s0)         # SIZE
    li t0, 2
    sd t0, 24(s0)         # COMMAND = WRITE
    ld t3, 0(s1)          # normal Load must wait for the adapter's write
    bne t3, s2, .Lfail
.Lwrite_wait:
    ld t1, 32(s0)         # STATUS: bit 0 BUSY, bit 1 DONE, bit 2 ERROR
    andi t2, t1, 1
    bnez t2, .Lwrite_wait
    li t2, 2
    bne t1, t2, .Lfail
    ld t3, 0(s1)          # normal Load shares the same NoC port
    bne t3, s2, .Lfail
    li t0, 1
    sd t0, 24(s0)         # COMMAND = READ
.Lread_wait:
    ld t1, 32(s0)
    andi t2, t1, 1
    bnez t2, .Lread_wait
    li t2, 2
    bne t1, t2, .Lfail
    ld t3, 40(s0)         # READ_DATA
    bne t3, s2, .Lfail
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
tohost:
    .dword 0
