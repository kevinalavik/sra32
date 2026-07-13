/*
 * sra32ka firmware entry point.
 *
 * _start reads the device tree, looks up the device named "serial",
 * and brings up (CTRL.ENABLE) every device that names it as parent
 */

.section .text
.global _start
.extern boot

/* puts(r0 = nul-terminated string, r1 = serial port base) */
puts:
    push $r1
    push $r2
    push $r3
    push $r4
.puts_loop:
    lb    $r2, 0($r0)
    cmp   $r2, $x0
    br.eq .puts_done
.wait_tx:
    lw    $r3, 4($r1)    // STATUS
    addi  $r4, $x0, 2    // TX_READY
    and   $r3, $r3, $r4
    cmp   $r3, $x0
    br.eq .wait_tx
    sw    $r2, 0($r1)    // DATA
    addi  $r0, $r0, 1
    br    .puts_loop
.puts_done:
    pop  $r4
    pop  $r3
    pop  $r2
    pop  $r1
    spc  $rp

/* streq(r0 = string a, r1 = string b) -> r0 = 1 if equal, else 0 */
streq:
    push $r2
    push $r3
.streq_loop:
    lb   $r2, 0($r0)
    lb   $r3, 0($r1)
    cmp  $r2, $r3
    br.ne .streq_ne
    cmp  $r2, $x0
    br.eq .streq_eq
    addi $r0, $r0, 1
    addi $r1, $r1, 1
    br   .streq_loop
.streq_ne:
    addi $r0, $x0, 0
    pop  $r3
    pop  $r2
    spc  $rp
.streq_eq:
    addi $r0, $x0, 1
    pop  $r3
    pop  $r2
    spc  $rp

/* find_device_by_name(r0 = nul-terminated name) -> r0 = 1 if found
 * (else 0), r1 = its index in the device tree (valid only if found).
 */
find_device_by_name:
    push $rp
    push $r2
    push $r3
    push $r4
    push $r6
    push $r14
    mov  $r14, $r0          // r14 = name to look for (r0)
    addi $r2, $x0, 0
    sui  $r2, 0xFFFE0       // r2 = device tree base
    lw   $r3, 0($r2)        // r3 = device_count
    addi $r4, $x0, 0        // r4 = loop index i
    mov  $r6, $r2
    addi $r6, $r6, 0x20     // r6 = &entries[0]
.fdbn_loop:
    cmp  $r4, $r3
    br.ge .fdbn_notfound
    mov  $r0, $r6           // streq(entry.name, target name)
    mov  $r1, $r14
    call streq
    cmp  $r0, $x0
    br.ne .fdbn_found
    addi $r4, $r4, 1
    addi $r6, $r6, 0x20
    br   .fdbn_loop
.fdbn_found:
    mov  $r1, $r4
    addi $r0, $x0, 1
    br   .fdbn_ret
.fdbn_notfound:
    addi $r0, $x0, 0
.fdbn_ret:
    pop  $r14
    pop  $r6
    pop  $r4
    pop  $r3
    pop  $r2
    pop  $rp
    spc  $rp

_start:
    /* find the device named "serial". r0 is guaranteed 0 here (since its an emulator all regs are zerod), so sui is safe */
    sui  $r0, serial_name
    addi $r0, $r0, serial_name
    call find_device_by_name
    cmp  $r0, $x0
    br.eq .no_console        // no such device: nothing to bring up
    mov  $r13, $r1           // r13 = serials own index

    /* bring up every device whose parent is "serial" (r13), remembering the first as the console */
    addi $r1, $x0, 0
    sui  $r1, 0xFFFE0
    lw   $r2, 0($r1)          // device_count
    addi $r3, $x0, 0          // loop index i
    mov  $r6, $r1
    addi $r6, $r6, 0x20       // &entries[0]
    addi $r5, $x0, 0          // console port base, 0 = none found yet
.child_loop:
    cmp  $r3, $r2
    br.ge .child_done
    lw   $r7, 0x10($r6)       // entry.parent
    cmp  $r7, $r13
    br.ne .child_next
    lw   $r9, 0x14($r6)       // entry.base
    addi $r10, $x0, 1
    sw   $r10, 8($r9)         // CTRL.ENABLE = 1
    cmp  $r5, $x0
    br.ne .child_next
    mov  $r5, $r9             // first child found becomes the console
.child_next:
    addi $r3, $r3, 1
    addi $r6, $r6, 0x20
    br   .child_loop
.child_done:
    cmp  $r5, $x0
    br.eq .no_console
    mov  $r1, $r5
    addi $r0, $x0, 0
    sui  $r0, boot_msg
    addi $r0, $r0, boot_msg
    call puts
.no_console:
    call boot
.halt:
    br .halt

.section .rodata
serial_name:
    .asciz "serial"
boot_msg:
    .asciz "jumping to boot() in C!\n"
