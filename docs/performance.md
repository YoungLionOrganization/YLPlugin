# Validation performance and memory

## Metadata-only discovery (0.1.3 development)

Standalone `read_metadata` / `readMetadata` and `ylplugin metadata` read
`128 + metadata_frame_length` bytes, independent of payload size. Memory scales
with bounded metadata and its parsed representation. They skip the TOC and
signature region as well as payloads. `Package` opening remains an indexed
operation; it has a broader read scope. See [metadata discovery](metadata-only.md).

A virtual 5 GiB regression source asserts exact ranges and total bytes. Linux
syscall tracing on the data example observed reads of 128, 32 and 1011 bytes,
with no TOC/payload read. This is read-volume evidence, not a wall-clock speedup
benchmark or a guarantee about OS block caching.

The 0.1.3 development change streams **plaintext store validation** through the
existing SHA256 slice path instead of allocating and discarding the full resource.
Every resource byte is still read and hashed on every call; results are not cached.
A damaged byte outside a requested slice still fails integrity validation.

## Operation costs

| Operation | Payload memory behavior |
|---|---|
| Plaintext `validate` in 0.1.3 | At most 1 MiB scratch per active resource validation |
| Plaintext `read_slice` | Requested output plus at most min(resource size, 1 MiB) scratch |
| Full `read` | Returns the full resource; record-sized memory is still required |
| Encrypted/compressed validation | Existing authenticated decode path, with stored/decoded buffers |
| Signature verification | Independent SHA512/Ed25519 pass; unchanged |

These are payload buffers, not bounds on total process RSS. Metadata, TOC,
OpenSSL state, facade/interpreter and concurrent calls consume additional memory.
Preflight budgets and record/decompression limits are unchanged. Validation is
linear in resource bytes; a tiny slice is not a constant-time integrity check.

## Reproduce a comparison

Build the released 0.1.2 baseline and development candidate with the same compiler,
dependencies and Release flags, then supply their C ABI shared libraries:

```sh
python tools/benchmark_validation.py \
  --cli build/ylplugin \
  --library baseline /path/to/baseline/libylplugin.so \
  --library candidate /path/to/candidate/libylplugin.so \
  --size-mib 32 --iterations 7 \
  --output build/validation-benchmark.json
```

For static-core ctypes bundles use `libylplugin_runtime.so` (or the platform's
DLL/dylib name). The harness uses the stable C ABI directly so both SDK versions
can be measured without bypassing facade/runtime version checks. Each library
runs in a separate process, validates once to warm caches, and records individual
wall-time samples and median. Unix peak RSS includes the Python interpreter and
native runtime; Windows reports no RSS rather than an invented estimate.
Inputs are a generated plaintext store record, not a compressed zero stream.
No registry, publishing token or private plugin data is involved.

## Recorded Linux comparison — 2026-11-04

Linux x86_64, glibc 2.41, GCC 14.2.0, Release build, Python 3.12.14. The baseline
is the 0.1.2 implementation; the candidate is the 0.1.3 working tree. Same 32 MiB
input, one warm-up, seven measured validations per isolated process:

| Metric | 0.1.2 | 0.1.3 candidate |
|---|---:|---:|
| Median validation | 40.85 ms | 28.88 ms |
| Whole-process peak RSS | 50.02 MiB | 18.60 MiB |

In this run, median latency fell about 29% and peak RSS about 63%. These are
observations for this machine and workload, not cross-platform promises. Different
payload sizes, cold I/O, compilers, concurrency and codecs change the result.
[Raw samples and environment](benchmarks/validation-2026-11-04.json) are retained
for inspection. Correctness tests separately assert bounded reader requests,
complete hashing, tamper rejection and cancellation without timing thresholds.
