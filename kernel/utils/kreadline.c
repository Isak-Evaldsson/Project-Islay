#include <devices/input_manager.h>
#include <utils.h>

static unsigned char get_char()
{
    char          c = '\n';
    input_event_t event;

    while (true) {
        input_manager_wait_for_event(&event);

        if (CHECK_IF_PRESSED(event.status) || event.key_code == KEY_CAPS) {
            continue;  // Read next char
        }

        switch (event.key_code) {
            case KEY_ENTER:
                c = '\n';
                break;

            case KEY_SPACE:
                c = ' ';
                break;

            default:
                if (event.key_code >= KEY_A && event.key_code <= KEY_Z) {
                    char tmp = (CHECK_IF_UPPER_CASE(event.status)) ? 'A' : 'a';
                    c        = tmp + (event.key_code - KEY_A);
                } else if (event.key_code >= KEY_0 && event.key_code <= KEY_9) {
                    c = '0' + (event.key_code - KEY_0);
                }
        }
        break;  // goes to return stmt
    }
    return c;
}

void kreadline(size_t size, char *str)
{
    char   c;
    size_t i = 0;

    while (i < (size - 1) && (c = get_char()) != '\n') {
        kprintf("%c", c);
        str[i++] = c;
    }
    kprintf("\n");
    str[i] = '\0';
}
