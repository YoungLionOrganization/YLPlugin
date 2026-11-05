# Permissions and Preflight

Permissions are scoped to the target host. The format transports declarations;
YLPlugin does not assign built-in meanings to bits, byte positions or names.

## Three representations

```toml
required_permissions = ["save", "custom.feature"]
```

```toml
required_permissions = { format = "bitmask", registry = "xyz.example.bits", registry_version = "1", value = 37 }
```

```toml
required_permissions = { format = "bytes", registry = "xyz.example.bits", registry_version = "1", values = [37, 8, 255, 0] }
```

Each required/optional declaration chooses one representation. Strings are
unique and nonempty. Integer masks are exact integers from 0 through
9223372036854775807. Larger masks use up to 8192 numeric bytes. Values are not
strings/floats/bools. Byte i contains bits 8*i through 8*i+7, least significant
bit first. C/C++ and bindings expose actual byte buffers.

## Registry and grant matching

A host advertises supported representations. For bit/byte masks it also
advertises the exact registry version. A similarly named registry with a
different version is insufficient. Required grants are supplied under the target
ID using the matching representation; declarations do not create grants.

For example, the data fixture's analysis target requests the maximum signed-64
mask. A JS host must preserve it as bigint; a number would lose bits. Java uses
Long and Python exact int. The viewer target requires no permissions; its optional
byte declaration demonstrates representation negotiation without authorizing
those optional operations automatically.

## Selection and results

Preflight checks host/API, runtime/version, OS/architecture/ABI, capabilities,
permissions, signature policy, content keys, extraction and budgets. Exact host
matches outrank generic targets; generic matching requires explicit opt-in.
Priority then lexical target ID determine deterministic ordering. `fallback=false`
can stop selection before lower-ranked candidates.

| Status | Interpretation |
|---|---|
| SUPPORTED | Declared requirements match host state |
| UNSUPPORTED / UNSUPPORTED_PROFILE | Target or profile cannot be used |
| NEEDS_RUNTIME | Required runtime is absent or outside bounds |
| NEEDS_PERMISSION | Host grant is missing/inadequate |
| NEEDS_LICENSE | Content key/entitlement unavailable |
| NEEDS_EXTRACTION | Required strategy is not allowed by the host |
| RESOURCE_LIMIT / UNKNOWN | Budget is insufficient or required budget is unknown |
| INVALID_SIGNATURE / INVALID_CONTAINER | Trust/validation failure |

Inspect reasons and candidate details rather than treating all non-SUPPORTED
results identically. A positive report does not itself execute code or promise
that a separately supplied adapter behaves safely. Read the
[normative profile](../profile.md) and [Host Integration](Host-Integration.md).
