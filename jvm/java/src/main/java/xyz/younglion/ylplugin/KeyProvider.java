package xyz.younglion.ylplugin;
/** Supplies exactly 32 key bytes, or null to deny. Called synchronously. */
@FunctionalInterface
public interface KeyProvider { byte[] keyFor(String keyId); }
