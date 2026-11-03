// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include <ylplugin/native.h>
static int start(void *ctx) {return ctx?0:1;}
static void stop(void *ctx) {(void)ctx;}
static void destroy(void *ctx) {(void)ctx;}
int ylp_native_query(uint32_t abi,ylp_native_v1 *out) {
    static int context=1;
    if(abi!=1||out->struct_size!=sizeof(*out))return 1;
    out->abi_version=1;out->context=&context;out->start=start;out->stop=stop;out->destroy=destroy;return 0;
}
