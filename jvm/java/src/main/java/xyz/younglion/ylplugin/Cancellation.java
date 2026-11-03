// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

package xyz.younglion.ylplugin;
/** Cooperative synchronous cancellation; true stops the native operation. */
@FunctionalInterface
public interface Cancellation { boolean cancelled(); }
