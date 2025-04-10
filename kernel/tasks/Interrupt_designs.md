# Interrupt re-design
We have three potential architectures

## Architectures
### Top + Bottom half

Needs to disable the current interrupt during the whole process in order to 
Needs to disable scheduling

### Top half + one isr service thread
Pros: 
Has the advantage of not needing to disable interrupts since we pass the state as a part of the work object put to the thread's queue form the top half ISR.

Cons:
Unclear how to design it without potentially running of of memory 

### Top half + per isr service thread
1. The top half ISR does some kind of work + recording it's state
2. Top half unblocks it's service thread
3. Service thread get scheduled, how is that implemented?
    - generic interrupt handler checks if there's need to to de re-scheduling (using 
      a Minix like pointer).
    - Does this affect how the call to schedule works as well? 
4. Service thread run
    - Needs to disable interrupts (or at least it's interrupt) while reading the state,
      rest of the work is done in an regular kthread context.
    - How do we deal with preemption? Disabled?
    - What if a service thread has a mutex waiting on a non-service thread (priority-inversion?)

Pros: Solves the multiple calls to top half problem
Cons: Needs to partially disable interrupts within the service thread.
      Does the scheduler need to an interrupt disabling lock then?

## Other considerations
When going into SMP, we need to two types of spinlocks:
1. The ones that disable interrupt (when code touches areas that are also used by an ISR), a bit slower, but ensures that we won't be preempted as well as no interrupts.
2. Regular spinlock, can be interrupted, but not preempted.

## Notes from interrupts file:
    So I see two design approaches:
    1. Top half (no interrupts) and bottom half (scheduling disabled) running after each other.
        a) How do when handle scheduling, i.e. when to to task switches
        - Set the postpone flag, at the end of the bottom half, check if whe need to do a
          re-scheduling
        - Do we need to disable interrupts during the whole critical section, or only during the
          read?

    How do we solve the interrupt on top of bottom half problem? With a state that is transfer from
    top to bottom. For example:

    timer: Stores current time as an isr_state arg, the bottom half operates based on that value
            (doesn't matter if a new isr updates it again, since that will cause a new bottom half)

    kbd: store scan code in state, or (update ringbuffer),

    But, if another interrupt fires then the second half will fire as well, on top of half 2, could
    stack infinitely. Disabling the firing interrupt seams to be the only option


    2. Top half + work_queue (separate thread, maybe on per core?)
    How to implement work queue:

    In upper half:
        1. Unblock thread (unblock it, or directly schedule it)
        2. Add work item to it's internal queue
        3. How do we allocate memory for the work items (since there might be multiple task for the
           same ISR)
           a) a fixed buffer we nope never ends
           b) We have an array of status flags, one for ISR, when the thread is woke up, it goes
              through the array checks if a handler needs to be executed


    That solution would solve syscalls like this:
    1. On int queue syscall object + unblock caller
    2. in thread do syscall
    3. in thread


    3. One thread per interrupt, so on ISR wake up thread, that particular thread only needs to
    disable interrupts while reading the state modified by the interrupt.

    if a new interrupt is run before the thread is running, we simply update the state to be read by
   thread. If the thread is running but already read it state? How do we tell it to be woken up
   again?


    DECISION: Lets go on the two halfs executing directly after each other. We do need a why to
    determine after each upper half in the bottom half needs to be run as well. Requires flagging
    magic.

    We have different cases:

    INT
    UPPER
    BOTTOM_START
    READ STATE
    BOTTOM_END
    INT <-- Should run again since there might be new data to process

    INT
    UPPER
    BOTTOM_START
    INT <-- Doesn't need to run again
    READ_STATE
    BOTTOM_END

    INT
    UPPER
    BOTTOM_START
    READ_STATE
    INT <-- Do need to run again
    BOTTOM_END

    Every time the bottom half executes, it's responsible to check if it got
    work to do (by for example reading a queue).

    However can we with some flags mechanism prevent unecessary calls that we
    now are redundant, or is that up to each bottom half?

    I assume we could allow to operating modes for the bottom half:
    1. Always run, every time the upper half is called the bottom half will follow
    2. Only keep one bottom half in flight concurrently (set inflight before enabling interrupts,
       clear after interrupts are disable again), when is this useful?
       - A ringbuf, bottom half will enter until empty and the return, a long as the insertion and
         deletion are atomic, then we're fine?

    In the upper half, interrupts are disabled so we can safely check the read and write pointers,
    enter an new entry in the buffer, adjust the write pointer

    In the bottom half, we read the current scancode at the read pointer, the increase it. In a loop
    we checks for update of the write pointer. If a new upper half runs during the lower half, we
    will simply check the pointer again.

    Will this work in SMP? Another solution would to make a look-free linked list (but keep make it
    bounded and fast by having a list items in fixed size array), Or we simply have small ringbuf
    per core, and then we have proper lock on the big kernel char queue?
