#
# Loads the TSS segment at index 5 gdt
# 
.section .text
.global load_tss
load_tss:
    mov $0x28, %ax
    ltr %ax
    ret