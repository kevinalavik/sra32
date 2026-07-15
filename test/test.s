.section .text
.global _start

_start:
    addi $r1, $x0, 0xFF     # r1 = 0x000000FF
    shli $r1, $r1, 8        # r1 = 0x0000FF00

    # print a simple hi
    addi $r2, $x0, 'H'      # r2 = 'H'
    sb $r2, 1($r1)          # [0xFF01] = 'H'
    addi $r2, $x0, 'I'      # r2 = 'I'
    sb $r2, 1($r1)          # [0xFF01] = 'I'
    addi $r2, $x0, '\n'     # r2 = '\n'
    sb $r2, 1($r1)          # [0xFF01] = '\n'

hang:
    br hang
