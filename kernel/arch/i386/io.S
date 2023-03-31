# outb - Writes a byte of data to the specified port
# args: 8(%esp) - the byte of data
#       4(%esp) - the port address
.section .text
.global outb
outb:
    mov 8(%esp), %al
    mov 4(%esp), %dx
    out %al, %dx
    ret

# inb - Reads a byte of data to the specified port
# args: 4(%esp) - the port address
.global inb
inb:
    mov 4(%esp), %dx
    in %dx, %al
    ret
