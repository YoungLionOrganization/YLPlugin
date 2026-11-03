# `xyz.younglion.plugin` profile 1.0

Normative. This profile uses the existing YLSpec 0.1 envelope, UTF-8 TOML 1.1,
signed 64-bit integers and 0.1 frame unchanged. `_ylspec.profile` MUST equal
`xyz.younglion.plugin` and `profile_version` MUST equal `1.0`. No additional
required YLSpec features are implemented in this release. Unknown required
features/profile versions MUST reject validation; inert inspection remains allowed.

`data` MUST contain `id`, `version`, `entrypoints`, and `targets`. `publisher`
is optional, informational and unverified. Version strings MUST be one to three
decimal components, at most nine digits per component. For comparisons, missing
components are zero: `1`, `1.0`, `1.0.0` compare equally. Ranges are inclusive;
minimum MUST NOT exceed maximum. Prereleases, caret/tilde and implicit wildcards
are not supported. IDs use ASCII letters, digits, dot, underscore and hyphen,
1..255 bytes. The `host` field alone MAY be `*`.

All entrypoint IDs and all target IDs MUST be independently unique. Duplicate IDs
are invalid even across different hosts. Every target references an entrypoint;
every entrypoint references an existing TOC path. A nonempty TOC `variant` refers
to an entrypoint ID and restricts its use to that entrypoint; empty allows sharing.
Plugin `id`/`version` and all profile string fields are 1..255 bytes without NUL.
Resource paths follow the longer container path limit. This release's profile
limits entrypoint resource reference strings to 255 bytes.

## Entrypoints

| Field | Requirement |
|---|---|
| `id`, `runtime`, `resource` | Required identifier, runtime identifier, portable TOC path |
| `runtime_min`, `runtime_max` | Required inclusive numeric adapter version bounds |
| `os`, `arch`, `abi` | Optional exact strings; default `*` |

Native entrypoints MUST declare concrete OS, architecture and ABI. Examples:
OS `windows`, `linux`, `macos`, `android`, `ios`; architecture `x86_64`, `arm64`.
ABI is host-defined (e.g. `ylp-c1-linux-gnu-x86_64`), not inferred from the CPU.
Native libraries depending on a C++/Qt ABI require additional host checks.

## Targets

| Field | Requirement/default |
|---|---|
| `id`, `host`, `entrypoint` | Required |
| `api_min`, `api_max` | Required inclusive host API bounds |
| `priority` | Signed integer, default 0 |
| `fallback` | Boolean, default true |
| `required_capabilities`, `optional_capabilities` | Unique string arrays, default [] |
| `required_permissions`, `optional_permissions` | Permission representations below, default [] |
| `memory`, `storage` | Nonnegative signed integers in bytes, default 0 |

Unknown fields in `data`, entrypoints, targets, or permission tables MUST reject
profile validation. Optional vendor data belongs under YLSpec
`extensions."reverse.domain.namespace"`; required semantics must use
`_ylspec.requires` and a separately supported profile/feature revision. Core
profile validation does not grant permissions or load code.

## Permissions

Each target declares required and optional permissions independently, using one
of these forms (not a mixture within one declaration):

```toml
required_permissions = ["save", "network", "custom.feature"]
optional_permissions = { format = "bitmask", registry = "example.permissions", registry_version = "1", value = 37 }
# Alternatively:
# required_permissions = { format = "bytes", registry = "example.permissions", registry_version = "1", values = [37, 8, 255, 0] }
```

String lists MUST contain unique nonempty strings. They are scoped to the target
host and have no YLPlugin-defined meanings. Bitmask/bytes tables MUST specify
nonempty `registry` and `registry_version`; host support requires exact registry
version equality. Integer mask MUST be a real integer in `0..9223372036854775807`.
Larger masks MUST use bytes. Each byte MUST be an integer in `0..255`, not a
string/float/bool; maximum 8192 bytes. Empty bytes and zero masks request no bits.
Permission byte `i` contains bits `8*i..8*i+7`, least significant bit first.
The C/C++ API returns actual `uint8_t` buffers; no binary bytes are inserted into
the TOML text. The YLSpec serializer emits all integers as decimal, including
inputs originally spelled in binary/hexadecimal. Strings such as `"37"` are invalid.

YLPlugin MUST NOT assign any meaning to bits or names. Identical bit values can
mean different things to different hosts. Unsupported representations/registries
MUST NOT silently authorize requests. Optional unsupported permissions are
reported but do not block a target. Grants are supplied by the host, keyed by
target ID within its own host context. Required bits/strings must be a subset of
explicit grants in the same representation and registry. Cross-representation
interpretation requires a host adapter; YLPlugin does not guess mappings.

## Selection

Preflight MUST execute no plugin code. Match the exact host ID; generic `*` also
matches only if `allow_generic=true`. An explicit host `target` selector limits
the candidates further. Sort exact matches before generic matches, then priority
descending, then target ID in ascending ASCII order. Evaluate in this order. The
first SUPPORTED target wins. A failing target with `fallback=false` stops search.
Otherwise try the next candidate. If none succeeds, report the first candidate's
failure and all evaluated candidate reports. Equal priority is deterministic,
never a hash iteration or file ordering choice.

Host ID matching does not bypass API, runtime version, platform, ABI, resource,
capability, permission, license, budget or signature checks. Generic targets are
subject to all the same constraints. Sessions and caches MUST be scoped by host
identity, plugin identity/version and session, never shared solely by plugin ID.

## Validation artifacts

`schemas/plugin.schema.json` describes the logical TOML tree for authoring.
The native validator also enforces reference integrity, uniqueness and ordered
version ranges; JSON Schema alone is not a conformance decision. The exported
`ylp_plugin_profile()` supplies an actual YLSpec `yl_profile` callback. Use it in a
caller-owned `yl_registry`; there is no global registration or schema fetching.
Published positive/negative vectors are under `tests/vectors`.

## Binding representation requirements

The 0.1.1 SDK frontends preserve this profile unchanged. JS uses bigint when a
mask exceeds the safe number range, Java uses Long and Python int; the writer
still emits decimal integers. A host must not stringify a mask to work around
its language's numeric limits. See the [language compatibility rules](compatibility.md).
The data example demonstrates a full signed-64 mask and explicit generic opt-in.
