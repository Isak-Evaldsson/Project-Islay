#ifndef ARCH_BOOT_H
#define ARCH_BOOT_H

/* Each architecture is required to implement an assembly routine unmapping the identity mapping
 * that was setup as a part of the higher-half booting procedure */
void unmap_identity_mapping();

#endif /* ARCH_BOOT_H */
