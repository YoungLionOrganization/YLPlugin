"""YLPlugin SDK: ctypes over the same native C++ reader/writer, never a second parser."""
from __future__ import annotations

import ctypes as C
import json
import os
from pathlib import Path
from typing import Callable

__version__ = "0.1.0"
KeyProvider = Callable[[str], bytes | None]


class Error(ValueError):
    def __init__(self, code: int, message: str):
        self.code = code
        super().__init__(message)


class _Error(C.Structure):
    _fields_ = [("code", C.c_int), ("message", C.c_char * 512)]


class _Buffer(C.Structure):
    _fields_ = [("data", C.c_void_p), ("size", C.c_size_t)]


_Key = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_char_p, C.POINTER(C.c_uint8))
_Cancel = C.CFUNCTYPE(C.c_int, C.c_void_p)


def _key(fn: KeyProvider | None):
    if fn is None:
        return _Key()

    def call(_context, name, out):
        try:
            value = fn(name.decode("utf-8"))
            if value is None or not isinstance(value, bytes) or len(value) != 32:
                return 1
            C.memmove(out, value, 32)
            return 0
        except BaseException:
            return 1

    return _Key(call)


def _cancel(fn):
    if fn is None:
        return _Cancel()

    def call(_context):
        try:
            return int(bool(fn()))
        except BaseException:
            return 1

    return _Cancel(call)


class Library:
    """Load an explicitly installed SDK. No native downloads or fallback parser."""

    def __init__(self, path: str | os.PathLike | None = None):
        path = path or os.environ.get("YLPLUGIN_LIBRARY")
        if not path:
            raise RuntimeError("Pass the native library path or set YLPLUGIN_LIBRARY")
        self.native = C.CDLL(os.fspath(path))
        p, s, n, e, b = C.c_void_p, C.c_char_p, C.c_size_t, C.POINTER(_Error), C.POINTER(_Buffer)
        signatures = {
            "ylp_version": ([], s),
            "ylp_open": ([s, p, C.POINTER(p), e], C.c_int),
            "ylp_close": ([p], None),
            "ylp_buffer_free": ([b], None),
            "ylp_inspect": ([p, b, e], C.c_int),
            "ylp_metadata": ([p, b, e], C.c_int),
            "ylp_target_count": ([p], n),
            "ylp_entrypoint_count": ([p], n),
            "ylp_target_at": ([p, n, b, e], C.c_int),
            "ylp_entrypoint_at": ([p, n, b, e], C.c_int),
            "ylp_read": ([p, s, _Key, p, _Cancel, p, b, e], C.c_int),
            "ylp_read_slice": ([p, s, C.c_uint64, n, b, e], C.c_int),
            "ylp_validate": ([p, _Key, p, _Cancel, p, e], C.c_int),
            "ylp_verify": ([p, _Key, p, _Cancel, p, e], C.c_int),
            "ylp_check": ([p, s, _Key, p, _Key, p, b, e], C.c_int),
            "ylp_extract": ([p, s, s, _Key, p, _Cancel, p, e], C.c_int),
            "ylp_create": ([s, s, s, _Key, p, _Cancel, p, e], C.c_int),
            "ylp_sign": ([s, s, s, p, n, e], C.c_int),
        }
        for name, (args, result) in signatures.items():
            function = getattr(self.native, name)
            function.argtypes, function.restype = args, result

    def _call(self, name, *args):
        error = _Error()
        status = getattr(self.native, name)(*args, C.byref(error))
        if status:
            raise Error(status, bytes(error.message).decode("utf-8", "replace"))

    def _buffer(self, name, *args) -> bytes:
        out = _Buffer()
        try:
            self._call(name, *args, C.byref(out))
            return C.string_at(out.data, out.size)
        finally:
            self.native.ylp_buffer_free(C.byref(out))

    def open(self, path: str | os.PathLike) -> Package:
        return Package(self, path)

    def create(self, output, manifest: str, resources: list[dict], *, keys: KeyProvider | None = None, cancel=None):
        provider, cancellation = _key(keys), _cancel(cancel)
        self._call("ylp_create", os.fsencode(output), manifest.encode(),
                   json.dumps({"resources": resources}).encode(), provider, None, cancellation, None)

    def sign(self, source, output, key_id: str, private_pem: bytes):
        secret = C.create_string_buffer(private_pem)
        try:
            self._call("ylp_sign", os.fsencode(source), os.fsencode(output), key_id.encode(), secret, len(private_pem))
        finally:
            C.memset(secret, 0, len(secret))


class Package:
    def __init__(self, library: Library, path):
        self.library, self._handle = library, C.c_void_p()
        library._call("ylp_open", os.fsencode(path), None, C.byref(self._handle))

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()

    def __del__(self):
        self.close()

    def close(self):
        if getattr(self, "_handle", None):
            self.library.native.ylp_close(self._handle)
            self._handle = C.c_void_p()

    def _open(self):
        if not self._handle:
            raise RuntimeError("package is closed")
        return self._handle

    @property
    def metadata(self) -> str:
        return self.library._buffer("ylp_metadata", self._open()).decode()

    def inspect(self) -> dict:
        return json.loads(self.library._buffer("ylp_inspect", self._open()))

    @property
    def targets(self) -> list[dict]:
        return [json.loads(self.library._buffer("ylp_target_at", self._open(), i))
                for i in range(self.library.native.ylp_target_count(self._open()))]

    @property
    def entrypoints(self) -> list[dict]:
        return [json.loads(self.library._buffer("ylp_entrypoint_at", self._open(), i))
                for i in range(self.library.native.ylp_entrypoint_count(self._open()))]

    def read(self, path: str, *, keys: KeyProvider | None = None, cancel=None) -> bytes:
        provider, cancellation = _key(keys), _cancel(cancel)
        return self.library._buffer("ylp_read", self._open(), path.encode(), provider, None, cancellation, None)

    def read_slice(self, path: str, offset: int, count: int) -> bytes:
        if offset < 0 or count < 0:
            raise ValueError("offset and count must be nonnegative")
        return self.library._buffer("ylp_read_slice", self._open(), path.encode(), offset, count)

    def validate(self, *, keys: KeyProvider | None = None, cancel=None):
        provider, cancellation = _key(keys), _cancel(cancel)
        self.library._call("ylp_validate", self._open(), provider, None, cancellation, None)

    def verify(self, trust: KeyProvider, *, cancel=None):
        provider, cancellation = _key(trust), _cancel(cancel)
        self.library._call("ylp_verify", self._open(), provider, None, cancellation, None)

    def check(self, host: dict, *, keys: KeyProvider | None = None, trust: KeyProvider | None = None) -> dict:
        provider, trusted = _key(keys), _key(trust)
        return json.loads(self.library._buffer("ylp_check", self._open(), json.dumps(host).encode(), provider, None, trusted, None))

    def extract(self, destination, paths: list[str] | None = None, *, keys: KeyProvider | None = None, cancel=None):
        provider, cancellation = _key(keys), _cancel(cancel)
        self.library._call("ylp_extract", self._open(), os.fsencode(destination), json.dumps(paths or []).encode(), provider, None, cancellation, None)
