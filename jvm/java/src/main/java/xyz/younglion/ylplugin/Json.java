// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

package xyz.younglion.ylplugin;
import java.math.BigInteger;
import java.util.IdentityHashMap;
import java.util.Map;

/** Internal request encoding only. Container parsing is always native. */
final class Json {
    static String encode(Object value) { return encode(value, new IdentityHashMap<>()); }
    private static String quote(String value) {
        StringBuilder out = new StringBuilder("\"");
        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);
            if (c == '"' || c == '\\') out.append('\\').append(c);
            else if (c < 32) out.append(String.format("\\u%04x", (int)c));
            else out.append(c);
        }
        return out.append('"').toString();
    }
    private static String encode(Object value, IdentityHashMap<Object, Boolean> seen) {
        if (value == null) return "null";
        if (value instanceof String) return quote((String)value);
        if (value instanceof Boolean) return value.toString();
        if (value instanceof Byte || value instanceof Short || value instanceof Integer || value instanceof Long) return value.toString();
        if (value instanceof BigInteger) {
            BigInteger n = (BigInteger)value;
            if (n.compareTo(BigInteger.ONE.shiftLeft(63).negate()) < 0 || n.compareTo(BigInteger.ONE.shiftLeft(64)) >= 0) throw new IllegalArgumentException("integer exceeds native 64-bit range");
            return n.toString();
        }
        if (value instanceof Double || value instanceof Float) {
            if (!Double.isFinite(((Number)value).doubleValue())) throw new IllegalArgumentException("non-finite JSON number");
            return value.toString();
        }
        if (seen.put(value, Boolean.TRUE) != null) throw new IllegalArgumentException("cyclic request");
        try {
            StringBuilder out = new StringBuilder();
            if (value instanceof Map) {
                out.append('{'); boolean first = true;
                for (Map.Entry<?, ?> entry : ((Map<?, ?>)value).entrySet()) {
                    if (!(entry.getKey() instanceof String)) throw new IllegalArgumentException("string map keys required");
                    if (!first) out.append(','); first = false;
                    out.append(quote((String)entry.getKey())).append(':').append(encode(entry.getValue(), seen));
                }
                return out.append('}').toString();
            }
            if (value instanceof Iterable) {
                out.append('['); boolean first = true;
                for (Object item : (Iterable<?>)value) { if (!first) out.append(','); first = false; out.append(encode(item, seen)); }
                return out.append(']').toString();
            }
            throw new IllegalArgumentException("request values must be strings, numbers, booleans, maps or lists");
        } finally { seen.remove(value); }
    }
    private Json() {}
}
