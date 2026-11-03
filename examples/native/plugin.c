#include <ylplugin/native.h>
#include <stdlib.h>
#include <stdio.h>
typedef struct { int started; } Demo;
static int start(void *context) { Demo *demo=(Demo*)context;demo->started=1;puts("YLPlugin native example started");return 0; }
static void stop(void *context) { ((Demo*)context)->started=0; }
static void destroy(void *context) { free(context); }
int ylp_native_query(uint32_t requested,ylp_native_v1 *out) {
    if(requested!=1||!out||out->struct_size!=sizeof(*out))return 1;
    Demo *demo=(Demo*)calloc(1,sizeof(Demo));if(!demo)return 2;
    out->abi_version=1;out->context=demo;out->start=start;out->stop=stop;out->destroy=destroy;return 0;
}
