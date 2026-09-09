    .section .text.init
    .globl _start
    .type _start, @function
_start:
    csrr t0, mhartid

    # Each hart writes and reads a private shared-memory word through the NoC.
    slli t1, t0, 3
    lui  t2, 0x1
    add  t2, t2, t1
    addi t3, t0, 1
    sd   t3, 0(t2)
    ld   t4, 0(t2)
    bne  t3, t4, .Lfail

    la   t5, tohost
    li   t6, 1
    sd   t6, 0(t5)
.Lwait:
    j    .Lwait

.Lfail:
    la   t5, tohost
    addi t6, t0, 1
    slli t6, t6, 1
    ori  t6, t6, 1
    sd   t6, 0(t5)
    j    .Lwait

    .size _start, .-_start

    .section .tohost,"aw",@progbits
    .balign 8
    .globl tohost
tohost:
    .dword 0
