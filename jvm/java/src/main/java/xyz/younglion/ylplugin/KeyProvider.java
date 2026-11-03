// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

package xyz.younglion.ylplugin;
/** Supplies exactly 32 key bytes, or null to deny. Called synchronously. */
@FunctionalInterface
public interface KeyProvider { byte[] keyFor(String keyId); }
