package xyz.younglion.ylplugin;
/** Cooperative synchronous cancellation; true stops the native operation. */
@FunctionalInterface
public interface Cancellation { boolean cancelled(); }
