#
# Short assenbly rutine to load GDT for 32-bit flat model configuration
#
.section .bss
gdtr: .word 0  # For limit (table size) storage
      .long 0  # For base (table location) storage

.section .text
.global load_gdt
load_gdt:
   mov   4(%esp), %ax 
   mov   %ax, (gdtr)
   mov   8(%esp), %eax
   mov   %eax, gdtr + 2
   lgdt  gdtr
   ret