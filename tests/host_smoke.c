#include <stdio.h>
#include <string.h>
#include "vela_host.h"

int main(void){
    VelaHostedCarrier carrier;
    if(!vela_hosted_init(&carrier)){
        fputs("UN_Vela hosted carrier initialization failed\n",stderr);
        return 2;
    }
    if(!carrier.initialized||!vela_platform_is_compatible(&carrier.ops)){
        fputs("UN_Vela hosted carrier ABI mismatch\n",stderr);
        vela_hosted_shutdown(&carrier);
        return 3;
    }
    if(!(carrier.ops.capabilities&VELA_PLATFORM_CAP_HTTP)||!carrier.ops.http_get){
        fputs("UN_Vela hosted carrier missing HTTP capability\n",stderr);
        vela_hosted_shutdown(&carrier);
        return 4;
    }
    if(!(carrier.ops.capabilities&VELA_PLATFORM_CAP_TIME)||!carrier.ops.time_ms){
        fputs("UN_Vela hosted carrier missing time capability\n",stderr);
        vela_hosted_shutdown(&carrier);
        return 5;
    }
    unsigned long long before=(unsigned long long)carrier.ops.time_ms(&carrier);
    const char*name=vela_hosted_name();
    if(!name||!name[0]){
        fputs("UN_Vela hosted carrier has no platform name\n",stderr);
        vela_hosted_shutdown(&carrier);
        return 6;
    }
    printf("UN_Vela hosted carrier OK: %s / ABI %u.%u / time=%llu\n",name,(unsigned)(carrier.ops.abi_version>>16),(unsigned)(carrier.ops.abi_version&0xffffu),before);
    vela_hosted_shutdown(&carrier);
    return 0;
}
