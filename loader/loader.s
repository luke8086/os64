;
; Copyright (c) 2014-2015 Łukasz S.
; Distributed under the terms of GPL-2 License.
;
; loader/loader.s - multiboot compatible 64-bit kernel loader
;

global entry
global loader_pml4

extern kmain
extern kmain_intr

section .text

; multiboot header items

MBOOT_PAGE_ALIGN  equ  1<<0
MBOOT_MEM_INFO    equ  1<<1
MBOOT_VIDEO_MODE  equ  1<<2
MBOOT_FLAGS       equ  MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO | MBOOT_VIDEO_MODE
MBOOT_MAGIC       equ  0x1BADB002
MBOOT_CKSUM       equ  -(MBOOT_MAGIC + MBOOT_FLAGS)

MBOOT_VID_TYPE    equ  0
MBOOT_VID_WIDTH   equ  800
MBOOT_VID_HEIGHT  equ  600
MBOOT_VID_DEPTH   equ  24

; control registers

CR0_PG_BIT        equ 0x1F        ; paging
CR0_CD_BIT        equ 0x1E        ; cache-disable

CR4_PAE_BIT       equ 0x05        ; physical-address extension
CR4_PGE_BIT       equ 0x07        ; page-global-enable

MSR_EFER_ADDR     equ 0xC0000080  ; extended feature enable register
MSR_EFER_LME_BIT  equ 0x08        ; long mode enable

; page translation tables

PAGE_SIZE         equ 0x200000  ; 2MB pages
MEM_START         equ 0x0       ; map memory from the beginning

PML4E_SIZE        equ 0x08      ; 64-bit entries
PML4E_COUNT       equ 0x200     ; 512 entries
PML4E_LOWER       equ 0x00      ; 0GB
PML4E_FLAGS       equ 0x03      ; present, read-write

PDPE_COUNT        equ 0x200     ; 512 entries
PDPE_SIZE         equ 0x08      ; 64-bit entries
PDPE_LOWER        equ 0x00      ; 0GB
PDPE_FLAGS        equ 0x03      ; present, read-write

PDE_INITIAL_COUNT equ 0x10      ; 16 * 2MB = 32MB
PDE_COUNT         equ 0x200     ; 512 entries
PDE_SIZE          equ 0x08      ; 64-bit entries
PDE_FLAGS         equ 0x83      ; present, read-write, page-size

; stack

STACK_SIZE        equ 0x10000   ; 64KB stack

; segment selectors

SS_SI_OFFSET      equ 0x03                 ; index offset in the selector
SS_NULL           equ 0x00 << SS_SI_OFFSET ; null segment selector
SS_CODE32         equ 0x01 << SS_SI_OFFSET ; 32-bit code segment selector
SS_CODE64         equ 0x02 << SS_SI_OFFSET ; 64-bit code segment selector
SS_DATA           equ 0x03 << SS_SI_OFFSET ; data segment selector

; segment descriptors

SD_SIZE           equ 0x08      ; 64-bit entries
SD_COUNT          equ 0x04      ; null, code32, code64, data
SD_ALIGNMENT      equ 0x08      ; align gdt to 8 bytes

; gate descriptors

GATE_SIZE         equ 0x10      ; 128-bit entries
GATE_EXC_COUNT    equ 0x20      ; 32 exceptions
GATE_INT_COUNT    equ 0x10      ; 16 hardware interrupts
GATE_SYS_COUNT    equ 0x10      ; 16 system interrupts
GATE_COUNT        equ GATE_EXC_COUNT + GATE_INT_COUNT + GATE_SYS_COUNT
GATE_ALIGNMENT    equ 0x10      ; align idt to 16 bytes

; 8259 pic

PIC1_CMD          equ 0x20      ; io address for master pic command
PIC1_DATA         equ 0x21      ; io address for master pic data
PIC2_CMD          equ 0xA0      ; io address for master pic command
PIC2_DATA         equ 0xA1      ; io address for master pic data

PIC_EOI           equ 0x20      ; end of interrupt

PIC1_OFFSET       equ 0x20      ; vector offset for master pic
PIC2_OFFSET       equ 0x28      ; vector offset for slave pic

PIC_ICW1          equ 00010001b ; init & require ICW4
PIC1_ICW2         equ PIC1_OFFSET
PIC2_ICW2         equ PIC2_OFFSET
PIC1_ICW3         equ 00000100b ; master pic has slave on irq 2
PIC2_ICW3         equ 00000010b ; slave pic id is 2
PIC_ICW4          equ 00000001b ; 8086 mode

PIC1_DEF_MASK     equ 00000000b ; enable all interrupts, except rtc
PIC2_DEF_MASK     equ 00000000b ; enable all interrupts

; multiboot header

mboot_header:

  ; basic

  dd MBOOT_MAGIC
  dd MBOOT_FLAGS
  dd MBOOT_CKSUM

  ; aout kludge (unused)

  dd 0
  dd 0
  dd 0
  dd 0
  dd 0

  ; video mode

  dd MBOOT_VID_TYPE
  dd MBOOT_VID_WIDTH
  dd MBOOT_VID_HEIGHT
  dd MBOOT_VID_DEPTH

; loader code

[bits 32]
entry:

  ; disable interrupts

  cli

  ; setup stack

  mov  esp, stack + STACK_SIZE
  mov  ebp, esp

  ; save multiboot structure pointer

  mov [mboot_ptr], ebx

  ; load gdt

  lgdt [gdt_pointer_32]
  jmp SS_CODE32:.flush_gdt
.flush_gdt:

  ; setup lower PDEs

  mov eax, MEM_START
  or eax, PDE_FLAGS
  mov ecx, PDE_INITIAL_COUNT
  mov edi, pd_lower
.loop_pde_lower:
  mov [edi], eax
  add edi, PDE_SIZE
  add eax, PAGE_SIZE
  loop .loop_pde_lower

  ; setup the lower PDPE

  mov eax, pd_lower
  or eax, PDPE_FLAGS
  mov dword [pdpt + (PDPE_LOWER * PDPE_SIZE)], eax

  ; setup a single PML4E in PML4

  mov eax, pdpt
  or eax, PML4E_FLAGS
  mov dword [loader_pml4 + (PML4E_LOWER * PML4E_SIZE)], eax

  ; enable 64-bit page table entries (physical-address extension)

  mov  eax, cr4
  bts  eax, CR4_PAE_BIT
  mov  cr4, eax

  ; initialize page-table base address with the PML4 table

  mov eax, loader_pml4
  mov cr3, eax

  ; set long-mode-enable bit of the extended-feature-enable-register

  mov ecx, MSR_EFER_ADDR
  rdmsr
  bts eax, MSR_EFER_LME_BIT
  wrmsr

  ; enable paging and disable cache

  mov eax, cr0
  bts eax, CR0_PG_BIT
  bts eax, CR0_CD_BIT
  mov cr0, eax

  ; jump to 64-bit code selector

  jmp  SS_CODE64:.code64
[bits 64]
.code64:

  ; reload gdt using 64-bit pointer

  lgdt [gdt_pointer_64]
  mov r9, .flush_gdt_64
  jmp r9
.flush_gdt_64:

  ; clear data segment registers

  xor eax, eax
  mov fs, eax
  mov gs, eax

  ; initialize idt gates

  mov rcx, 0
  mov rdi, idt
.generate_idt_gates_loop:
  mov rax, [isr_stubs + rcx * 8]
  stosw
  mov rax, 0x10
  stosw
  mov ax, 0x8e00
  stosw
  mov rax, [isr_stubs + rcx * 8]
  shr rax, 16
  stosw
  shr rax, 16
  stosd
  xor rax, rax
  stosd
  inc rcx
  cmp rcx, GATE_COUNT
  jnz .generate_idt_gates_loop

  ; initialize 8259 PIC

  mov al, PIC_ICW1
  out PIC1_CMD, al
  mov al, PIC_ICW1
  out PIC2_CMD, al
  mov al, PIC1_ICW2
  out PIC1_DATA, al
  mov al, PIC2_ICW2
  out PIC2_DATA, al
  mov al, PIC1_ICW3
  out PIC1_DATA, al
  mov al, PIC2_ICW3
  out PIC2_DATA, al
  mov al, PIC_ICW4
  out PIC1_DATA, al
  mov al, PIC_ICW4
  out PIC2_DATA, al
  mov al, PIC1_DEF_MASK
  out PIC1_DATA, al
  mov al, PIC2_DEF_MASK
  out PIC2_DATA, al

  ; load idtr and enable interrupts

  lidt [idt_pointer]

  ; jump to the main c function

  mov rdi, [mboot_ptr]
  mov  r9, kmain
  jmp  r9

; isr stubs

%macro build_isr_stub 1
isr_stub_%1:
    cli

    push rax
    mov rax, %1
    mov [current_interrupt], rax
    pop rax

    jmp isr_common
%endmacro

%assign i 0
%rep GATE_COUNT
    build_isr_stub i
%assign i i+1
%endrep

; common interrupt handling code

isr_common:

  ; save state

  sub rsp, 0x100

  mov [rsp+0x00], rax
  mov [rsp+0x08], rbx
  mov [rsp+0x10], rcx
  mov [rsp+0x18], rdx
  mov [rsp+0x20], rdi
  mov [rsp+0x28], rsi
  mov [rsp+0x30], rbp
  mov [rsp+0x38], r8
  mov [rsp+0x40], r9
  mov [rsp+0x48], r10
  mov [rsp+0x50], r11
  mov [rsp+0x58], r12
  mov [rsp+0x60], r13
  mov [rsp+0x68], r14
  mov [rsp+0x70], r15

  ; call interrupt handler

  mov rdi, [current_interrupt]
  mov rsi, rsp
  add rsi, 0x100
  mov rdx, rsp
  mov r9, kmain_intr
  call r9

  ; handle end-of-interrupt command

  mov rax, [current_interrupt]

  cmp rax, GATE_EXC_COUNT
  jb .skip_eoi
  cmp rax, GATE_EXC_COUNT + GATE_INT_COUNT
  ja .skip_eoi

  cmp rax, PIC2_OFFSET
  jb .skip_eoi_pic2

  mov al, PIC_EOI
  out PIC2_CMD, al

.skip_eoi_pic2:
  mov al, PIC_EOI
  out PIC1_CMD, al

.skip_eoi:

  ; restore state

  mov r15, [rsp+0x70]
  mov r14, [rsp+0x68]
  mov r13, [rsp+0x60]
  mov r12, [rsp+0x58]
  mov r11, [rsp+0x50]
  mov r10, [rsp+0x48]
  mov r9,  [rsp+0x40]
  mov r8,  [rsp+0x38]
  mov rbp, [rsp+0x30]
  mov rsi, [rsp+0x28]
  mov rdi, [rsp+0x20]
  mov rdx, [rsp+0x18]
  mov rcx, [rsp+0x10]
  mov rbx, [rsp+0x08]
  mov rax, [rsp+0x00]

  add rsp, 0x100

  iretq

[section .bss]

; page translation tables

loader_pml4:
  resb (PML4E_COUNT * PML4E_SIZE)

pdpt:
  resb (PDPE_COUNT * PDPE_SIZE)

pd_lower:
  resb (PDE_COUNT * PDE_SIZE)

; stack

stack:
  resb STACK_SIZE

; temporary storage

mboot_ptr:
  resq 1

current_interrupt:
  resq 1

[section .rodata]

; global descriptor table with universal 32 & 64 bit segment descriptors

align SD_ALIGNMENT

gdt:

; null segment

  dw 0x00       ; segment limit[15:0]
  dw 0x00       ; base addr[15:0]
  db 0x00       ; base addr[23:16]
  db 00000000b  ; P, DPL, S, type
  db 00000000b  ; G, DB, _, AVL, segment limit[19:16]
  db 0x00       ; base addr[31:24]

; 32-bit code segment

  dw 0xFFFF     ; segment limit[15:0]
  dw 0x0000     ; base addr[15:0]
  db 0x00       ; base addr[23:16]
  db 10011010b  ; P, DPL, 1, 1, C, R, A
  db 11001111b  ; G, D, L, AVL, segment limit[19:16]
  db 0x00       ; base addr[31:24]

; 64-bit code segment

  dw 0x00       ; segment limit[15:0]
  dw 0x00       ; base addr[15:0]
  db 0x00       ; base addr[23:16]
  db 10011000b  ; P, DPL, 1, 1, C, R, A
  db 00100000b  ; G, D, L, AVL, segment limit[19:16]
  db 0x00       ; base addr[31:24]

; data segment

  dw 0xFFFF     ; segment limit[15:0]
  dw 0x00       ; base addr[15:0]
  db 0x00       ; base addr[23:16]
  db 10010010b  ; P, DPL, 1, 0, E, W, A
  db 00001111b  ; G, DB, _, AVL, segment limit[19:16]
  db 0x00       ; base addr[31:24]

gdt_pointer_32:

  dw (SD_SIZE * SD_COUNT - 1)   ; limit
  dd gdt                        ; base

gdt_pointer_64:

  dw (SD_SIZE * SD_COUNT - 1)   ; limit
  dq gdt                        ; base

; interrupt descriptor table with 64 bit gate descriptors

align GATE_ALIGNMENT

idt:

    times (GATE_COUNT * GATE_SIZE) db 0

idt_pointer:

    dw (GATE_COUNT * GATE_SIZE) - 1  ; limit
    dq idt                           ; base

; convenience array of pointers to isrs

isr_stubs:

%assign i 0
%rep GATE_COUNT
    dq isr_stub_%[i]
%assign i i+1
%endrep
