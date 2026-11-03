// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#ifndef YLPLUGIN_NATIVE_H
#define YLPLUGIN_NATIVE_H
#include <stdint.h>
#ifdef _WIN32
#define YLP_NATIVE_EXPORT __declspec(dllexport)
#else
#define YLP_NATIVE_EXPORT __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* Independent of LeoMiniGames IGamePlugin. No exceptions may cross this ABI. */
typedef struct {
    uint32_t struct_size, abi_version;
    void *context;
    int (*start)(void *context);
    void (*stop)(void *context);
    void (*destroy)(void *context);
} ylp_native_v1;
typedef int (*ylp_native_query_fn)(uint32_t requested_abi, ylp_native_v1 *out);
YLP_NATIVE_EXPORT int ylp_native_query(uint32_t requested_abi, ylp_native_v1 *out);
#ifdef __cplusplus
}
#endif
#endif
