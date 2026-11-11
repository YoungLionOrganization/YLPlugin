// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <ylplugin/ylplugin.hpp>
#include <map>
#include <optional>

namespace ylp {
struct PresentationLimits {
    uint64_t entries = 64, text = 1024 * 1024, icon = 2 * 1024 * 1024;
    uint64_t total = 16 * 1024 * 1024;
};
struct PresentationAsset {
    std::string id, role, resource, media_type, format, encoding;
    uint64_t offset = 0, length = 0;
    std::array<uint8_t,32> hash{};
};
struct PresentationCatalog {
    std::string version;
    std::map<std::string,std::string> primary;
    std::vector<PresentationAsset> assets;
};
// Copies and validates catalog declarations only; performs no file I/O.
// Missing namespace returns nullopt; unsupported version throws Code::unsupported.
// Does not validate container bounds, TOC agreement, asset bytes or publisher trust.
std::optional<PresentationCatalog> parse_presentation_catalog(
    const yl_document*, const PresentationLimits& = {});
} // namespace ylp
