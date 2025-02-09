/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef DEVICES_TTY_H
#define DEVICES_TTY_H

/* For driver registration */
extern struct driver tty_driver;

/* Creates tty devices based on the available numbers of vga text mode devices, and mounts them into
 * devfs */
int make_tty_devs();

#endif /*  DEVICES_TTY_H */
