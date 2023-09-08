# Strcut offests used within the task switch function
#define PCB_ESP_OFFSET  0
#define PCB_CR3_OFFSET  4
#define PCB_ESP0_OFFSET 8
#define TSS_ESPO_OFFSET 4

# External pointers
.extern kernel_tss

#
# i386 implementation of: void kernel_thread_switch(thread_regs_t* new_thread, thread_regs_t* old_thread),
# defined in arch/thread.h
#
.section .text
.global kernel_thread_switch
kernel_thread_switch:
    # Save previous task's state
    # Notes:
    #  For cdecl# EAX, ECX, and EDX are already saved by the caller and don't need to be saved again
    #  EIP is already saved on the stack by the caller's "CALL" instruction
    #  The task isn't able to change CR3 so it doesn't need to be saved
    #  Segment registers are constants (while running kernel code) so they don't need to be saved
    push %ebx
    push %esi
    push %edi
    push %ebp
 
    # Loading arguments into registers
    movl 20(%esp), %esi              # esi = new_thread
    movl 24(%esp), %edi              # edi = old_thread
    
    # Save old current state in old_threads register
    movl %esp, PCB_ESP_OFFSET(%edi)  # Save ESP for previous task's kernel stack

    # Load the new state from new_threads registers
    # Done in c code: movl %esi, current_task                    # replace current_task to the new task
    movl PCB_ESP_OFFSET(%esi),  %esp           # Load ESP for next task's kernel stack from  its PCB
    movl PCB_CR3_OFFSET(%esi),  %eax           # eax = address of page directory for next task
    movl PCB_ESP0_OFFSET(%esi), %ebx           # ebx = address for the top of the next task's kernel stack
    movl %ebx, (kernel_tss + TSS_ESPO_OFFSET)  # adjust the ESP0 field in the TSS (used by CPU for for CPL=3 -> CPL=0 privilege level changes)

    # Compare and change page tables if necesary
    movl %cr3, %ecx             # ecx = previous task's virtual address space
    cmp  %ecx, %eax             # Are we change the virutal address space?
    je   done_change_virt_addr  # no -> no need to change cr3, avoid unnecesary tlb flushes
    movl %eax, %cr3             # yes -> swap the virutal space
done_change_virt_addr:

    # Restore registers save onto the stack
    pop %ebp
    pop %edi
    pop %esi
    pop %ebx
 
    ret # Load next task's EIP from its kernel stack