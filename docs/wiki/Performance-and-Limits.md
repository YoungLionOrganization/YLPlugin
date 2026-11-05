# Performance and Limits

Limits are host policy and are checked before large allocation/decoding. They
do not turn resource declarations into trusted budgets or promise every operation
uses the same amount of memory.

## Default reader limits

| Limit | Default |
|---|---:|
| Metadata text | 16 MiB |
| TOC | 8 MiB |
| Records | 16384 |
| Individual stored/decoded record | 64 MiB |
| Aggregate decoded data | 1 GiB |
| Expansion ratio | 256, with 1 MiB allowance |

Offsets are 64-bit. Larger containers can use multiple records and higher
aggregate limits; arbitrary-size streaming decode of one compressed/encrypted
record is not implemented. The CLI does not expose limit overrides; SDK hosts
can configure their own appropriate limits.

## Allocation behavior

| Operation | Payload memory |
|---|---|
| 0.1.3 plaintext validation | At most 1 MiB scratch per active resource validation |
| Plaintext slice | Output slice plus at most min(resource size, 1 MiB) scratch |
| Full read | Complete returned resource |
| Encrypted/compressed validation | Existing stored/decoded authenticated decode buffers |
| Signature verification | Separate SHA512 prefix scan/Ed25519 verification |

The bounded plaintext validation change is in development 0.1.3. Every byte is
still read and hashed on each call; successful hashes are not cached across a
mutable custom reader. A small slice checks data outside the slice too.
Metadata, TOC, interpreter, crypto state and concurrent calls add memory beyond
payload scratch. Preflight budgets are unchanged.

## Recorded comparison

On Linux x86_64/glibc 2.41, GCC 14.2 Release, Python 3.12.14, a 32 MiB plaintext
resource was validated after one warm-up with seven samples per isolated process:

| Metric | 0.1.2 baseline | 0.1.3 candidate |
|---|---:|---:|
| Median validation | 40.85 ms | 28.88 ms |
| Whole-process peak RSS | 50.02 MiB | 18.60 MiB |

That run showed approximately 29% lower median latency and 63% lower peak RSS.
It is a single-machine workload observation, not a universal speed guarantee.
Cold I/O, callback latency, concurrency, compiler, codecs and resource sizes can
change the result.

## Reproduce

```sh
python tools/benchmark_validation.py --cli build/ylplugin \
  --library baseline /path/to/0.1.2/libylplugin.so \
  --library candidate /path/to/0.1.3/libylplugin.so \
  --size-mib 32 --iterations 7 --output build/validation-benchmark.json
```

Use matching compiler/dependency/Release flags. The harness uses the stable C
ABI directly and isolated processes. Unix RSS includes Python and native code;
Windows reports no RSS estimate. Correctness tests assert bounded payload reads,
complete hashing, tamper rejection and cancellation rather than machine-specific
timing thresholds. [Raw measurements and method](../performance.md) are retained.
