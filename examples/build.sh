#!/bin/sh
# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

set -eu
task_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
task_rcc=${RCC:-rcc}
task_cli=${YLPLUGIN_CLI:-"$task_root/build/ylplugin"}
task_output=${1:-"$task_root/build/rcc-example"}
mkdir -p "$(dirname -- "$task_output")"
mkdir "$task_output"
"$task_rcc" --binary "$task_root/examples/rcc/game.qrc" -o "$task_output/game.rcc"
"$task_cli" convert "$task_output/game.rcc" --manifest "$task_root/examples/plugin.ylspec" --output "$task_output/demo.ylplugin"
"$task_cli" validate "$task_output/demo.ylplugin"
"$task_cli" check "$task_output/demo.ylplugin" --host "$task_root/examples/host.json"
