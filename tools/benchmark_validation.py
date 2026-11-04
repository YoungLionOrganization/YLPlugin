# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Measure validation in isolated processes; no speed threshold is asserted."""
from pathlib import Path
import argparse
import ctypes as C
import json
import os
import platform
import statistics
import subprocess
import sys
import tempfile
import time


def worker(library, package, iterations):
    class Error(C.Structure):
        _fields_ = [('code', C.c_int), ('message', C.c_char * 512)]
    lib = C.CDLL(str(Path(library).resolve()))
    lib.ylp_version.restype = C.c_char_p
    lib.ylp_open.argtypes = [C.c_char_p, C.c_void_p, C.POINTER(C.c_void_p), C.POINTER(Error)]
    lib.ylp_open.restype = C.c_int
    lib.ylp_validate.argtypes = [C.c_void_p] * 5 + [C.POINTER(Error)]
    lib.ylp_validate.restype = C.c_int
    lib.ylp_close.argtypes = [C.c_void_p]
    lib.ylp_close.restype = None
    handle, error = C.c_void_p(), Error()
    def checked(status):
        if status:
            raise RuntimeError(f'{error.code}: {error.message.decode("utf-8", "replace")}')
    checked(lib.ylp_open(os.fsencode(Path(package).resolve()), None, C.byref(handle), C.byref(error)))
    timings = []
    try:
        checked(lib.ylp_validate(handle, None, None, None, None, C.byref(error)))
        for _ in range(iterations):
            start = time.perf_counter()
            checked(lib.ylp_validate(handle, None, None, None, None, C.byref(error)))
            timings.append((time.perf_counter() - start) * 1000)
    finally:
        lib.ylp_close(handle)
    peak = None
    try:
        import resource
        value = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
        peak = value / (1024 * 1024 if sys.platform == 'darwin' else 1024)
    except ImportError:
        pass
    print(json.dumps({'version': lib.ylp_version().decode(), 'samples_ms': timings,
                      'median_ms': statistics.median(timings), 'peak_rss_mib': peak}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', type=Path, required=True)
    parser.add_argument('--library', nargs=2, action='append', metavar=('LABEL', 'PATH'), required=True)
    parser.add_argument('--size-mib', type=int, default=32)
    parser.add_argument('--iterations', type=int, default=7)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if not 1 <= args.size_mib <= 64 or args.iterations < 1:
        parser.error('size must be 1..64 MiB and iterations must be positive')
    root = Path(__file__).resolve().parents[1]
    report = {'operation': 'validate plaintext store resource, warm cache',
              'platform': platform.platform(), 'python': platform.python_version(),
              'size_bytes': args.size_mib * 1024 * 1024, 'iterations': args.iterations,
              'rss_scope': 'whole isolated process including interpreter and native runtime', 'results': []}
    with tempfile.TemporaryDirectory(prefix='ylplugin-benchmark-') as temporary:
        work = Path(temporary)
        payload, package = work / 'payload', work / 'data.ylplugin'
        with payload.open('wb') as stream:
            for _ in range(args.size_mib):
                stream.write(b'\x25' * (1024 * 1024))
        plan = work / 'plan.json'
        plan.write_text(json.dumps({'resources': [{'path': 'catalog.json', 'file': str(payload)}]}))
        subprocess.run([str(args.cli.resolve()), 'create', '--manifest', str(root / 'examples/data/plugin.ylspec'),
                        '--plan', str(plan), '--output', str(package)], check=True, stdout=subprocess.DEVNULL)
        for label, library in args.library:
            run = subprocess.run([sys.executable, str(Path(__file__).resolve()), '_worker',
                                  str(Path(library).resolve()), str(package), str(args.iterations)],
                                 check=True, text=True, capture_output=True)
            report['results'].append({'label': label, **json.loads(run.stdout)})
    text = json.dumps(report, indent=2) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text)
    print(text, end='')


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '_worker':
        worker(sys.argv[2], sys.argv[3], int(sys.argv[4]))
    else:
        main()
