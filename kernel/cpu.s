;
; Copyright (c) 2014-2015 Łukasz S.
; Distributed under the terms of GPL-2 License.
;
; kernel/cpu.s - low-level cpu instructions
;

[section .text]

[global cpu_inb]
[global cpu_outb]
[global cpu_invlpg]
[global cpu_cli]
[global cpu_sti]
[global cpu_task_switch]
[global cpu_get_flags]
[global cpu_set_flags]

; input a byte from a port
cpu_inb:
  mov dx, di
  in al, dx
  movzx eax, al
  ret

; output a byte to a port
cpu_outb:
  mov al, sil
  mov dx, di
  out dx, al
  ret

; invalidate a TLB entry
cpu_invlpg:
  invlpg [rdi]
  ret

# get flags
cpu_get_flags:
  pushf
  pop rax
  ret

# set flags
cpu_set_flags:
  push rdi
  popf
  ret

; clear the interrupts flag
cpu_cli:
  cli
  ret

; set the interrupts flag
cpu_sti:
  sti
  ret

; trigger task-switching interrupt
cpu_task_switch:
  int 0x31
  ret
