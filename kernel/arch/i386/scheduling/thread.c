#include <arch/i386/processor.h>
#include <arch/thread.h>
#include <klib/klib.h>

// The registers that needs to be stored
struct thread_regs {
    uint32_t esp;   // the contents of esp
    uint32_t cr3;   // the content of cr3
    uint32_t esp0;  // the content of the kernel tss, esp0 field
};

/* Creates a set of thread register for a kernel thread. And initiates the stack with the supplied
 * instruction pointer unless it set to NULL */
thread_regs_t* create_thread_regs_with_stack(void* stack_top, void* ip)
{
    thread_regs_t* regs = kmalloc(sizeof(thread_regs_t));
    if (regs == NULL) {
        return NULL;
    }

    // New to make enough space on the stack to allow the 4 register pops in
    // switch_to_task, as well as the implicit pop when it returns
    regs->esp = (uintptr_t)stack_top - 5 * sizeof(int32_t);

    // Set esp0 == esp for know, may change when we introduce a user-space
    regs->esp0 = regs->esp;

    // All kernel processes shares the same page tables
    regs->cr3 = get_cr3();

    // TODO: Implement task startup function, that handles initial unlock + kills process once the
    // process function returns
    *(uint32_t*)(regs->esp + 4 * sizeof(uint32_t)) = (uint32_t)ip;

    return regs;
}

/* Create the thread registers for the initial thread */
thread_regs_t* create_initial_thread_regs()
{
    thread_regs_t* regs = kmalloc(sizeof(thread_regs_t));
    if (regs == NULL) {
        return NULL;
    }

    regs->esp  = 0;  // Will be correctly assigned at context switch
    regs->esp0 = 0;
    regs->cr3  = get_cr3();
    return regs;
}

/* Frees the thread_regs_t struct */
void free_thread_regs(thread_regs_t* regs)
{
    kfree(regs);
}
