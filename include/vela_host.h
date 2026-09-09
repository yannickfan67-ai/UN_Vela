#ifndef UN_VELA_HOST_H
#define UN_VELA_HOST_H

#include <stdint.h>
#include "vela_platform.h"

/*
 * Hosted carrier wrapper for desktop operating systems.
 *
 * This is intentionally separate from UN_Orion's native carrier. The same
 * UN_Vela core can therefore be embedded in Orion or hosted by a conventional
 * desktop OS without changing browser-core code.
 */
typedef struct VelaHostedCarrier {
    VelaPlatformOps ops;
    uint64_t private_state[4];
    int initialized;
} VelaHostedCarrier;

int vela_hosted_init(VelaHostedCarrier *carrier);
void vela_hosted_shutdown(VelaHostedCarrier *carrier);
const char *vela_hosted_name(void);

#endif
