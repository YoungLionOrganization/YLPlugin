// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#ifndef YLPLUGIN_H
#define YLPLUGIN_H
#include <stddef.h>
#include <stdint.h>
#include <ylspec.h>
#if defined(YLPLUGIN_RUNTIME_EXPORT) && defined(_WIN32)
#define YLP_API __declspec(dllexport)
#elif defined(YLPLUGIN_RUNTIME_EXPORT)
#define YLP_API __attribute__((visibility("default")))
#elif defined(YLPLUGIN_STATIC)
#define YLP_API
#elif defined(_WIN32)
#if defined(YLPLUGIN_BUILD)
#define YLP_API __declspec(dllexport)
#else
#define YLP_API __declspec(dllimport)
#endif
#else
#define YLP_API __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define YLP_ABI_VERSION 1
typedef struct ylp_package ylp_package;
typedef struct ylp_session ylp_session;
typedef enum {
    YLP_OK=0, YLP_INVALID_CONTAINER=1, YLP_UNSUPPORTED_PROFILE=2, YLP_INVALID_METADATA=3,
    YLP_RESOURCE_LIMIT=4, YLP_IO=5, YLP_INTEGRITY=6, YLP_INVALID_SIGNATURE=7,
    YLP_NEEDS_LICENSE=8, YLP_UNSUPPORTED=9, YLP_CANCELLED=10, YLP_PERMISSION=11,
    YLP_RUNTIME=12, YLP_MEMORY=13
} ylp_status;
typedef struct { ylp_status code; char message[512]; } ylp_error;
typedef struct { unsigned char *data; size_t size; } ylp_buffer;
typedef struct {
    uint32_t struct_size, abi_version;
    uint64_t metadata, toc, records, record, total, ratio;
} ylp_limits;
/* Callbacks are synchronous. Return 0 on success; key size is exactly 32. */
typedef int (*ylp_key_fn)(void*, const char *key_id, uint8_t out[32]);
typedef int (*ylp_cancel_fn)(void*); /* nonzero cancels */
typedef int (*ylp_read_at_fn)(void*, uint64_t offset, void *out, size_t size);
typedef struct {
    uint32_t struct_size, abi_version;
    const char *path, *media, *variant, *key_id;
    uint64_t offset, stored_size, size;
    uint32_t alignment;
    uint8_t compression, encryption, extraction;
    uint8_t sha256[32];
} ylp_resource;
typedef struct {
    uint32_t struct_size, abi_version;
    int format; /* 0=strings, 1=bitmask, 2=bytes. No permission semantics. */
    const char *registry, *registry_version;
    uint64_t mask;
    const uint8_t *bytes; size_t byte_count;
} ylp_permission;
YLP_API const char *ylp_version(void);
YLP_API ylp_limits ylp_default_limits(void);
YLP_API void ylp_buffer_free(ylp_buffer*);
YLP_API ylp_status ylp_open(const char *path, const ylp_limits*, ylp_package **out, ylp_error*);
YLP_API ylp_status ylp_open_reader(uint64_t size, ylp_read_at_fn, void *context,
                                 const ylp_limits*, ylp_package **out, ylp_error*);
YLP_API void ylp_close(ylp_package*);
YLP_API ylp_status ylp_inspect(const ylp_package*, ylp_buffer *json, ylp_error*);
YLP_API ylp_status ylp_metadata(const ylp_package*, ylp_buffer *toml, ylp_error*);
YLP_API ylp_status ylp_validate_profile(const yl_document*, ylp_error*);
YLP_API yl_profile ylp_plugin_profile(void);
YLP_API size_t ylp_resource_count(const ylp_package*);
YLP_API ylp_status ylp_resource_at(const ylp_package*, size_t, ylp_resource*, ylp_error*);
YLP_API size_t ylp_target_count(const ylp_package*);
YLP_API size_t ylp_entrypoint_count(const ylp_package*);
YLP_API ylp_status ylp_target_at(const ylp_package*, size_t, ylp_buffer *json, ylp_error*);
YLP_API ylp_status ylp_entrypoint_at(const ylp_package*, size_t, ylp_buffer *json, ylp_error*);
YLP_API ylp_status ylp_permission_at(const ylp_package*, size_t target, int optional, ylp_permission*, ylp_error*);
YLP_API size_t ylp_permission_string_count(const ylp_package*, size_t target, int optional);
YLP_API const char *ylp_permission_string_at(const ylp_package*, size_t target, int optional, size_t index);
YLP_API ylp_status ylp_read(const ylp_package*, const char *resource, ylp_key_fn, void *key_context,
                          ylp_cancel_fn, void *cancel_context, ylp_buffer*, ylp_error*);
YLP_API ylp_status ylp_read_slice(const ylp_package*, const char*, uint64_t, size_t, ylp_buffer*, ylp_error*);
YLP_API ylp_status ylp_validate(const ylp_package*, ylp_key_fn, void*, ylp_cancel_fn, void*, ylp_error*);
YLP_API ylp_status ylp_verify(const ylp_package*, ylp_key_fn trust, void*, ylp_cancel_fn, void*, ylp_error*);
YLP_API ylp_status ylp_check(const ylp_package*, const char *host_json, ylp_key_fn keys, void*,
                           ylp_key_fn trust, void*, ylp_buffer *result_json, ylp_error*);
/* Selection is a JSON string array; [] means all. Destination must be new. */
YLP_API ylp_status ylp_extract(const ylp_package*, const char *destination, const char *selection_json,
                             ylp_key_fn, void*, ylp_cancel_fn, void*, ylp_error*);
/* Build JSON: {"resources":[{"path":...,"file":...,"media":...,"compression":0|1,
   "key_id":...,"variant":...,"extraction":0|1|2,"alignment":8}]} */
YLP_API ylp_status ylp_create(const char *output, const char *manifest_toml, const char *build_json,
                            ylp_key_fn, void*, ylp_cancel_fn, void*, ylp_error*);
YLP_API ylp_status ylp_sign(const char *input, const char *output, const char *key_id,
                          const uint8_t *private_pem, size_t size, ylp_error*);
/* C runtime helpers implement data runtime only; custom/native adapters use C++ API. */
YLP_API ylp_status ylp_data_session(const ylp_package*, const char *host_json, ylp_session **out, ylp_error*);
YLP_API ylp_status ylp_session_prepare(ylp_session*, ylp_cancel_fn, void*, ylp_error*);
YLP_API ylp_status ylp_session_activate(ylp_session*, ylp_error*);
YLP_API ylp_status ylp_session_start(ylp_session*, ylp_error*);
YLP_API void ylp_session_stop(ylp_session*);
YLP_API void ylp_session_free(ylp_session*);
#ifdef __cplusplus
}
#endif
#endif
