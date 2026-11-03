// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

package xyz.younglion.ylplugin;
/** Native status code matches ylp_status in the public C SDK. */
public final class YLPluginException extends RuntimeException {
    private final int code;
    public YLPluginException(String message, int code) { super(message); this.code = code; }
    public int getCode() { return code; }
}
