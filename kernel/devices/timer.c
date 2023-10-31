#include <devices/timer.h>
#include <utils.h>

// The initial priority queue size
#define INITIAL_QUEUE_SIZE (10)

// Enables verification of the min-heap data structure
#define DEBUG_TIMER_HEAP 1

// Enables logging of the timer
#define LOG_TIMER 0

#if LOG_TIMER
#define LOG(...) log("[TIMER]: " __VA_ARGS__)
#else
#define LOG(...)
#endif

#if DEBUG_TIMER_HEAP
#define VERIFY_HEAP() verify_minheap_property(0, __FILE__, __FUNCTION__, __LINE__)
#else
#define VERIFY_HEAP()
#endif

#define GET_PARENT(n)      (((n)-1) / 2)
#define GET_LEFT_CHILD(n)  ((n) * 2 + 1)
#define GET_RIGHT_CHILD(n) ((n) * 2 + 2)

// TODO: Have a more granular system with fixed point time to allowing avoiding clock drift for
// clock with a decimal period
static uint64_t time_since_boot_ns;  // Time since the device was booted in ns allows about 584
                                     // years of uptime before overflow, should proably be enough :)

/*
    Struct for each element in the internal timed event priority queue
 */
typedef struct timed_event {
    uint64_t             timestamp_ns;
    timed_event_callback callback;
} timed_event_t;

/*
    The internal timed event min-heap priority queue
 */
static struct {
    timed_event_t* array;
    size_t         size;
    size_t         max_size;
} event_queue = {.array = NULL, .size = 0, .max_size = 0};

/*
    Helper functions returning pointers to entries in queue based on supplied indicies
*/
static timed_event_t* get_parent(size_t child_index)
{
    return event_queue.array + GET_PARENT(child_index);
}

static timed_event_t* get_left(size_t parent_index)
{
    return event_queue.array + GET_LEFT_CHILD(parent_index);
}

static timed_event_t* get_right(size_t parent_index)
{
    return event_queue.array + GET_RIGHT_CHILD(parent_index);
}

static timed_event_t* get_entry(size_t index)
{
    return event_queue.array + index;
}

/*
    Debug functions
*/
#if DEBUG_TIMER_HEAP
void print_heap_element(size_t index, size_t indent)
{
    timed_event_t* event = event_queue.array + index;

    // Ensure termination
    if (index >= event_queue.size) return;

    // Insert proper indent
    for (size_t i = 0; i < indent; i++) kprintf("  ");

    kprintf("%u: timestamp %u, callback %u\n", index, event->timestamp_ns, event->callback);
    print_heap_element(GET_LEFT_CHILD(index), indent + 1);
    print_heap_element(GET_RIGHT_CHILD(index), indent + 1);
}

void print_heap()
{
    kprintf("\nDumping timer event priority queue\n");
    print_heap_element(0, 0);
}

void verify_minheap_property(size_t index, const char* file, const char* function,
                             unsigned int line)
{
    size_t left  = GET_LEFT_CHILD(index);
    size_t right = GET_RIGHT_CHILD(index);

    if (left < event_queue.size) {
        // kassert(get_left(index)->timestamp_ns >= get_entry(index)->timestamp_ns);

        if (get_left(index)->timestamp_ns < get_entry(index)->timestamp_ns) {
            print_heap();
            kpanic("%s():%s:%u: left %x < parent %x", function, file, line, left, index);
        }

        verify_minheap_property(left, function, file, line);
    }

    if (right < event_queue.size) {
        // kassert(get_right(index)->timestamp_ns >= get_entry(index)->timestamp_ns);

        if (get_right(index)->timestamp_ns < get_entry(index)->timestamp_ns) {
            print_heap();
            kpanic("%s():%s:%u: right %x < parent %x", function, file, line, right, index);
        }

        verify_minheap_property(right, function, file, line);
    }
}

#endif

static void swap(timed_event_t* a, timed_event_t* b)
{
    timed_event_t temp = *a;
    *a                 = *b;
    *b                 = temp;
}

// Recursively swaps the parent node at index with its smallest child
static void heapify(size_t index)
{
    size_t left  = GET_LEFT_CHILD(index);
    size_t right = GET_RIGHT_CHILD(index);

    // initially assume parent to be the smallest element
    size_t smallest = index;

    // check if left is less than smallest
    if (left < event_queue.size &&
        get_left(index)->timestamp_ns < get_entry(smallest)->timestamp_ns) {
        smallest = left;
    }

    // check if right is less than smallest
    if (right < event_queue.size &&
        get_right(index)->timestamp_ns < get_entry(smallest)->timestamp_ns) {
        smallest = right;
    }

    // swap the smallest entry with the current entry and repeat this process until the current
    // entry is smaller than the right and the left entry
    if (smallest != index) {
        swap(get_entry(smallest), get_entry(index));
        heapify(smallest);
    }
}

static void extract_min_element(timed_event_t* event)
{
    // copy min element
    *event = event_queue.array[0];

    //  replace first with last element
    event_queue.array[0] = event_queue.array[--event_queue.size];

    // maintain heap property by applying the heapify operation
    heapify(0);

    VERIFY_HEAP();
}

/*
    Allows the registration of timed events, one the supplied timestamps is reached the callback
    will be executed. The time system does not guarantee the callback to be invoked at exactly the
    specified time, however it guarantees to not invoke it earlier than the specified timestamp.
 */
bool timer_register_timed_event(uint64_t timestamp_ns, timed_event_callback callback)
{
    LOG("Register timed event to %x at %u", callback, timestamp_ns);

    // (re-)allocate array if necessary
    if (event_queue.size == event_queue.max_size) {
        void*  new_array;
        size_t new_max_size;
        size_t old_max_size = event_queue.max_size;

        // Set initial size if array is empty, else double size
        new_max_size = old_max_size == 0 ? INITIAL_QUEUE_SIZE : old_max_size * 2;

        new_array = krealloc(event_queue.array, sizeof(timed_event_t) * new_max_size);
        if (new_array == NULL) {
            return false;  // Failed to expand array
        } else {
            // update array
            event_queue.array    = new_array;
            event_queue.max_size = new_max_size;
        }
    }

    // insert element in end of heap
    event_queue.array[event_queue.size++] =
        (timed_event_t){.timestamp_ns = timestamp_ns, .callback = callback};

    // move element until the heap property is satisfied
    size_t i = event_queue.size - 1;
    while (i != 0 && get_parent(i)->timestamp_ns > timestamp_ns) {
        swap(get_parent(i), event_queue.array + i);
        i = GET_PARENT(i);
    }

    VERIFY_HEAP();
    return true;
}

/*
    Get the system time
*/
uint64_t timer_get_time_since_boot()
{
    return time_since_boot_ns;
}

/*
    Used for drivers to report the increase in time every clock pulse
*/
void timer_report_clock_pulse(uint64_t period_ns)
{
    timed_event_t event;

    time_since_boot_ns += period_ns;

    // Check the event queue for timed event
    while (event_queue.size > 0 && event_queue.array[0].timestamp_ns <= time_since_boot_ns) {
        extract_min_element(&event);

        LOG("Executing callback %x with timestamp %u", event.callback, event.timestamp_ns);
        event.callback(time_since_boot_ns, event.timestamp_ns);
    }
}
