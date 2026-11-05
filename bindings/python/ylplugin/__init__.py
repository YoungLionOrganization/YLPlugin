# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""YLPlugin SDK: ctypes over the same native C++ reader/writer, never a second parser."""
from __future__ import annotations

import ctypes as C
import json
import os
import ctypes.util
from pathlib import Path
import platform
from contextlib import contextmanager
from dataclasses import dataclass
import threading
from typing import Callable

__version__ = "0.1.3"
KeyProvider = Callable[[str], bytes | None]


def _text(value: str) -> bytes:
    if not isinstance(value, str) or "\0" in value:
        raise ValueError("expected a string without NUL")
    return value.encode("utf-8")


def _path(value) -> bytes:
    value = os.fspath(value)
    if isinstance(value, bytes):
        value = value.decode("utf-8")
    return _text(value)


def _integer(value: int, maximum: int, name: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or not 0 <= value <= maximum:
        raise ValueError(f"{name} must be an integer in 0..{maximum}")
    return value


@dataclass(frozen=True)
class Limits:
    metadata: int = 16 * 1024 * 1024
    toc: int = 8 * 1024 * 1024
    records: int = 16384
    record: int = 64 * 1024 * 1024
    total: int = 1024 * 1024 * 1024
    ratio: int = 256


class _Limits(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("abi_version", C.c_uint32)] + [
        (name, C.c_uint64) for name in ("metadata", "toc", "records", "record", "total", "ratio")]


def _limits(value: Limits):
    if not isinstance(value, Limits):
        raise TypeError("limits must be Limits")
    return _Limits(C.sizeof(_Limits), 1, *[_integer(getattr(value, field), 2**64 - 1, field)
                   for field in ("metadata", "toc", "records", "record", "total", "ratio")])


class Error(ValueError):
    def __init__(self, code: int, message: str):
        self.code = code
        super().__init__(message)


class _Error(C.Structure):
    _fields_ = [("code", C.c_int), ("message", C.c_char * 512)]


class _Buffer(C.Structure):
    _fields_ = [("data", C.c_void_p), ("size", C.c_size_t)]


class _Permission(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("abi_version", C.c_uint32), ("format", C.c_int),
                ("registry", C.c_char_p), ("registry_version", C.c_char_p), ("mask", C.c_uint64),
                ("bytes", C.c_void_p), ("byte_count", C.c_size_t)]


_Key = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_char_p, C.POINTER(C.c_uint8))
_Cancel = C.CFUNCTYPE(C.c_int, C.c_void_p)
_ReadAt = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint64, C.c_void_p, C.c_size_t)


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
        filename = "ylplugin_runtime.dll" if os.name == "nt" else "libylplugin_runtime.dylib" if platform.system() == "Darwin" else "libylplugin_runtime.so"
        bundled = Path(__file__).with_name("_native") / filename
        path = path or os.environ.get("YLPLUGIN_LIBRARY") or (bundled if bundled.is_file() else None) or ctypes.util.find_library("ylplugin")
        if not path:
            raise RuntimeError("Pass the native library path or set YLPLUGIN_LIBRARY")
        self.native = C.CDLL(os.fspath(path))
        self.native.ylp_version.restype = C.c_char_p
        actual = self.native.ylp_version().decode()
        if actual != __version__:
            raise RuntimeError(f"Native SDK {actual} does not match Python package {__version__}")
        p, s, n, e, b = C.c_void_p, C.c_char_p, C.c_size_t, C.POINTER(_Error), C.POINTER(_Buffer)
        signatures = {
            "ylp_version": ([], s),
            "ylp_open": ([s, p, C.POINTER(p), e], C.c_int),
            "ylp_close": ([p], None),
            "ylp_buffer_free": ([b], None),
            "ylp_inspect": ([p, b, e], C.c_int),
            "ylp_metadata": ([p, b, e], C.c_int),
            "ylp_read_metadata": ([s, p, b, e], C.c_int),
            "ylp_read_metadata_reader": ([C.c_uint64, _ReadAt, p, p, b, e], C.c_int),
            "ylp_target_count": ([p], n),
            "ylp_entrypoint_count": ([p], n),
            "ylp_target_at": ([p, n, b, e], C.c_int),
            "ylp_entrypoint_at": ([p, n, b, e], C.c_int),
            "ylp_permission_at": ([p, n, C.c_int, C.POINTER(_Permission), e], C.c_int),
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

    def open(self, path: str | os.PathLike, limits: Limits = Limits()) -> Package:
        return Package(self, path, limits)

    def read_metadata(self, path: str | os.PathLike, limits: Limits = Limits()) -> str:
        """Read only the header and YLSpec frame, without opening a Package/TOC."""
        native_limits = _limits(limits)
        return self._buffer("ylp_read_metadata", _path(path), C.byref(native_limits)).decode("utf-8")

    def read_metadata_reader(self, size: int, reader: Callable[[int, int], bytes], limits: Limits = Limits()) -> str:
        """Read header/YLSpec through synchronous byte ranges (for example a host's HTTP range reader)."""
        _integer(size, 2**64 - 1, "size")
        if not callable(reader):
            raise TypeError("reader must be callable")
        native_limits = _limits(limits)
        errors = []

        def read(_context, offset, destination, count):
            try:
                data = reader(offset, count)
                if not isinstance(data, bytes) or len(data) != count:
                    raise ValueError("reader must return exactly the requested bytes")
                C.memmove(destination, data, count)
                return 0
            except BaseException as error:
                errors.append(error)
                return 1

        callback = _ReadAt(read)
        try:
            return self._buffer("ylp_read_metadata_reader", size, callback, None, C.byref(native_limits)).decode("utf-8")
        except Error:
            if errors:
                raise errors[0]
            raise

    def create(self, output, manifest: str, resources: list[dict], *, keys: KeyProvider | None = None, cancel=None):
        provider, cancellation = _key(keys), _cancel(cancel)
        self._call("ylp_create", _path(output), _text(manifest),
                   json.dumps({"resources": resources}).encode(), provider, None, cancellation, None)

    def sign(self, source, output, key_id: str, private_pem: bytes):
        secret = C.create_string_buffer(private_pem)
        try:
            self._call("ylp_sign", _path(source), _path(output), _text(key_id), secret, len(private_pem))
        finally:
            C.memset(secret, 0, len(secret))


class Package:
    def __init__(self, library: Library, path, limits: Limits = Limits()):
        self._lock, self._busy = threading.RLock(), False
        self.library, self._handle = library, C.c_void_p()
        native_limits = _limits(limits)
        library._call("ylp_open", _path(path), C.byref(native_limits), C.byref(self._handle))

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()

    def __del__(self):
        self.close()

    def close(self):
        if not getattr(self, "_handle", None):
            return
        with self._lock:
            if self._busy:
                raise RuntimeError("cannot close a package from an active callback")
            if self._handle:
                self.library.native.ylp_close(self._handle)
                self._handle = C.c_void_p()

    @contextmanager
    def _using(self):
        with self._lock:
            if not self._handle:
                raise RuntimeError("package is closed")
            if self._busy:
                raise RuntimeError("reentrant package operations are not allowed")
            self._busy = True
            try:
                yield self._handle
            finally:
                self._busy = False

    def _buffer(self, operation, *args):
        with self._using() as handle:
            return self.library._buffer(operation, handle, *args)

    @property
    def metadata(self) -> str:
        return self._buffer("ylp_metadata").decode()

    def inspect(self) -> dict:
        return json.loads(self._buffer("ylp_inspect"))

    def _rows(self, kind):
        with self._using() as handle:
            count = getattr(self.library.native, f"ylp_{kind}_count")(handle)
            if count == 0:
                # Valid plugin profiles require nonempty arrays. Propagate the
                # native profile error instead of hiding it as an empty result.
                self.library._buffer(f"ylp_{kind}_at", handle, 0)
            return [json.loads(self.library._buffer(f"ylp_{kind}_at", handle, i)) for i in range(count)]

    @property
    def targets(self) -> list[dict]:
        return self._rows("target")

    @property
    def entrypoints(self) -> list[dict]:
        return self._rows("entrypoint")

    def read(self, path: str, *, keys: KeyProvider | None = None, cancel=None) -> bytes:
        provider, cancellation = _key(keys), _cancel(cancel)
        return self._buffer("ylp_read", _text(path), provider, None, cancellation, None)

    def permission_bytes(self, target: int, *, optional: bool = False) -> bytes:
        _integer(target, 2**(C.sizeof(C.c_size_t) * 8) - 1, "target")
        if not isinstance(optional, bool):
            raise TypeError("optional must be bool")
        with self._using() as handle:
            value = _Permission(C.sizeof(_Permission), 1)
            self.library._call("ylp_permission_at", handle, target, int(optional), C.byref(value))
            if value.format != 2:
                raise TypeError("permission representation is not bytes")
            return C.string_at(value.bytes, value.byte_count)

    def read_slice(self, path: str, offset: int, count: int) -> bytes:
        _integer(offset, 2**64 - 1, "offset")
        _integer(count, 2**(C.sizeof(C.c_size_t) * 8) - 1, "count")
        return self._buffer("ylp_read_slice", _text(path), offset, count)

    def validate(self, *, keys: KeyProvider | None = None, cancel=None):
        provider, cancellation = _key(keys), _cancel(cancel)
        with self._using() as handle:
            self.library._call("ylp_validate", handle, provider, None, cancellation, None)

    def verify(self, trust: KeyProvider, *, cancel=None):
        provider, cancellation = _key(trust), _cancel(cancel)
        with self._using() as handle:
            self.library._call("ylp_verify", handle, provider, None, cancellation, None)

    def check(self, host: dict, *, keys: KeyProvider | None = None, trust: KeyProvider | None = None) -> dict:
        provider, trusted = _key(keys), _key(trust)
        return json.loads(self._buffer("ylp_check", json.dumps(host).encode(), provider, None, trusted, None))

    def extract(self, destination, paths: list[str] | None = None, *, keys: KeyProvider | None = None, cancel=None):
        provider, cancellation = _key(keys), _cancel(cancel)
        with self._using() as handle:
            self.library._call("ylp_extract", handle, _path(destination), json.dumps(paths or []).encode(), provider, None, cancellation, None)
