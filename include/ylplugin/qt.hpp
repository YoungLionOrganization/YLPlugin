// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <ylplugin/ylplugin.hpp>
#include <QString>

namespace ylp {
// The host must validate RCC internals and publisher policy before approving.
// Container hashes/signatures alone do not establish Qt parser safety.
using RccApproval = std::function<bool(const std::string& plugin_id, const Resource&)>;
std::unique_ptr<Adapter> rcc_adapter(QString mount_root, RccApproval);
// Buffers retained because a QFile/QResource still existed at unregister time.
// Such buffers are deliberately kept until process exit; there is no unsafe force-free.
size_t retained_rcc_buffers();
}
