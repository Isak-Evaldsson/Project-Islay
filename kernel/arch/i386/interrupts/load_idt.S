# load_idt - Loads the interrupt descriptor table (IDT).
.global load_idt
.section .text
load_idt:
    mov   4(%esp), %eax
    lidt  (%eax)
    ret