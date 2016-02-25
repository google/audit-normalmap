#!/bin/sh
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -eu

: ${AUDIT_NORMALMAP:=../audit-normalmap}
: ${AUDIT_NORMALMAP_FLAGS:=}
: ${STRIP_PREFIX="*/textures/"}
: ${OUTPUT_DIR=out}
echo "Using audit-normalmap binary: AUDIT_NORMALMAP=$AUDIT_NORMALMAP"
echo "Using audit-normalmap flags: AUDIT_NORMALMAP_FLAGS=$AUDIT_NORMALMAP_FLAGS"
echo "Stripping prefix: STRIP_PREFIX=$STRIP_PREFIX"
echo "Writing report to: OUTPUT_DIR=$OUTPUT_DIR"

mkdir -p "$OUTPUT_DIR"
cp index.html index.js "$OUTPUT_DIR/"
: > "$OUTPUT_DIR/report.js"

i=0
for normalmap in "$@"; do
  ii=$(printf "%08d" "$i")
  echo "Processing $normalmap..."
  convert "$normalmap" -alpha off "$OUTPUT_DIR/$ii-norm.png"
  convert "$normalmap" -alpha extract "$OUTPUT_DIR/$ii-height.png"
  "$AUDIT_NORMALMAP" $AUDIT_NORMALMAP_FLAGS \
    -o "$OUTPUT_DIR/$ii-report.hdr" "$normalmap" > "$OUTPUT_DIR/$ii-report.json"
  convert "$OUTPUT_DIR/$ii-report.hdr" "$OUTPUT_DIR/$ii-report.png"
  convert "$OUTPUT_DIR/$ii-norm.png" \
    -geometry 64x64 "$OUTPUT_DIR/$ii-norm-t.png"
  convert "$OUTPUT_DIR/$ii-height.png" \
    -geometry 64x64 "$OUTPUT_DIR/$ii-height-t.png"
  convert "$OUTPUT_DIR/$ii-report.png" \
    -geometry 64x64 "$OUTPUT_DIR/$ii-report-t.png"
  rm -f "$OUTPUT_DIR/$ii-report.hdr"
  i=$((i + 1))
  {
    echo "addRow(\"${normalmap#$STRIP_PREFIX}\", \"$ii\","
    cat "$OUTPUT_DIR/$ii-report.json"
    echo ");"
  } >> "$OUTPUT_DIR/report.js"
done
echo "Done. View $OUTPUT_DIR/index.html to see the report."
