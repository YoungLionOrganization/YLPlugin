#!/bin/sh
# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

# Creates fresh example keys in a private directory; never use sample keys in production.
set -eu
umask 077
task_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
task_cli=${YLPLUGIN_CLI:-"$task_root/build/ylplugin"}
task_output=${1:?Pass a NEW private output directory}
task_input=${2:-"$task_root/examples/artifacts/data.ylplugin"}
mkdir -m 700 "$task_output"
openssl rand -out "$task_output/content.key" 32
openssl genpkey -algorithm ED25519 -out "$task_output/private.pem"
openssl pkey -in "$task_output/private.pem" -pubout -out "$task_output/public.pem"
"$task_cli" encrypt "$task_input" --output "$task_output/encrypted.ylplugin" --key-id demo-license --key-file "$task_output/content.key"
"$task_cli" sign "$task_output/encrypted.ylplugin" --output "$task_output/signed-encrypted.ylplugin" --key-id demo-publisher --private-key "$task_output/private.pem"
"$task_cli" verify "$task_output/signed-encrypted.ylplugin" --key-id demo-publisher --public-key "$task_output/public.pem"
"$task_cli" validate "$task_output/signed-encrypted.ylplugin" --key-id demo-license --key-file "$task_output/content.key"
