// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

package xyz.younglion.ylplugin;
import java.lang.ref.Cleaner;
import java.lang.ref.Reference;
import java.util.Map;
import java.util.List;

/** A synchronized native handle. Prefer try-with-resources over GC cleanup. */
public final class Plugin implements AutoCloseable {
    private static final Cleaner CLEANER = Cleaner.create();
    private static final class State implements Runnable {
        long handle;
        State(long handle) { this.handle = handle; }
        synchronized Object call(String op, Object args, KeyProvider keys, KeyProvider trust, Cancellation cancel) {
            if (handle == 0) throw new IllegalStateException("package is closed");
            return Native.invoke(handle, op, Json.encode(args), null, keys, trust, cancel);
        }
        public synchronized void run() { if (handle != 0) { Native.close(handle); handle = 0; } }
    }
    private final State state;
    private final Cleaner.Cleanable cleanable;
    Plugin(String path, Map<String, ?> limits) { state = new State(Native.open(path, Json.encode(limits))); cleanable = CLEANER.register(this, state); }
    private Object call(String op, Object args, KeyProvider keys, KeyProvider trust, Cancellation cancel) {
        try { return state.call(op, args, keys, trust, cancel); }
        finally { Reference.reachabilityFence(this); }
    }
    public void close() { state.run(); cleanable.clean(); }
    public String metadata() { return (String)call("metadata", Map.of(), null, null, null); }
    @SuppressWarnings("unchecked") public Map<String, Object> inspect() { return (Map<String, Object>)call("inspect", Map.of(), null, null, null); }
    @SuppressWarnings("unchecked") public List<Map<String, Object>> targets() { return (List<Map<String, Object>>)call("targets", Map.of(), null, null, null); }
    @SuppressWarnings("unchecked") public List<Map<String, Object>> entrypoints() { return (List<Map<String, Object>>)call("entrypoints", Map.of(), null, null, null); }
    public byte[] read(String path) { return read(path, null, null); }
    public byte[] permissionBytes(int target, boolean optional) { return (byte[])call("permissionBytes", Map.of("target", target, "optional", optional), null, null, null); }
    public byte[] read(String path, KeyProvider keys, Cancellation cancel) { return (byte[])call("read", Map.of("path", path), keys, null, cancel); }
    public byte[] readSlice(String path, long offset, long count) { return (byte[])call("slice", Map.of("path", path, "offset", offset, "count", count), null, null, null); }
    public void validate() { validate(null, null); }
    public void validate(KeyProvider keys, Cancellation cancel) { call("validate", Map.of(), keys, null, cancel); }
    public void verify(KeyProvider trust) { call("verify", Map.of(), null, trust, null); }
    public Map<String, Object> check(Map<String, ?> host) { return check(host, null, null); }
    @SuppressWarnings("unchecked") public Map<String, Object> check(Map<String, ?> host, KeyProvider keys, KeyProvider trust) { return (Map<String, Object>)call("check", Map.of("host", host), keys, trust, null); }
    public void extract(String destination, List<String> paths, KeyProvider keys, Cancellation cancel) { call("extract", Map.of("destination", destination, "paths", paths), keys, null, cancel); }
}
