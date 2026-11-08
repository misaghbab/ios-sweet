#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
HERE=$ROOT/userland/metal-frontend
WORK=$ROOT/metal-frontend-work
OUT=$ROOT/out-metal-frontend
BRIDGE=$WORK/deps/bridge/libnative_metal_bridge.a
TURNIP=$WORK/deps/turnip/libvulkan_freedreno.dylib
mkdir -p "$WORK/include/bridge" "$OUT/SweetMetal2.bundle"
echo "411b431660ab71a12c9478ecbefe88c890b6228cd82c00c96200dd711c2a1572  $BRIDGE" | shasum -a 256 -c -
echo "34bce03d2b0d15cc56ce83412a3e6743ec9889f33473f7f380153230333369b0  $TURNIP" | shasum -a 256 -c -
cp "$ROOT"/userland/metal-bridge/*.h "$WORK/include/bridge/"
SDK=$(xcrun --sdk iphoneos --show-sdk-path)
CC=$(xcrun --sdk iphoneos --find clang)
B=$OUT/SweetMetal2.bundle
FLAGS=(-target arm64-apple-ios16.0 -isysroot "$SDK" -I"$HERE/include" -I"$WORK/include" -O2 -g0 -Wall)
"$CC" "${FLAGS[@]}" -x objective-c++ -std=c++17 -fobjc-arc -Wno-deprecated-declarations -Wno-unused-function -bundle \
  -o "$B/SweetMetal2" "$HERE/SweetMetal2.mm" "$HERE/NativeBridgeMetalFrontend.mm" -x none "$BRIDGE" \
  -Wl,-dead_strip -lc++ -framework Foundation -framework Metal -framework IOSurface -framework IOKit -framework CoreFoundation 2>&1 | tee "$WORK/link.log"
"$CC" "${FLAGS[@]}" -o "$OUT/m2vtest" "$HERE/m2vtest.c" "$BRIDGE" -Wl,-dead_strip 2>&1 | tee "$WORK/probe-link.log"
cp "$HERE/Info.plist" "$B/"
cp "$TURNIP" "$B/"
for binary in "$B/SweetMetal2" "$B/libvulkan_freedreno.dylib" "$OUT/m2vtest"; do
  xcrun lipo "$binary" -verify_arch arm64
  xcrun otool -L "$binary" >> "$WORK/dependencies.log"
  codesign --force --sign - "$binary"
done
rm -rf "$B/SweetMetal2.dSYM"
codesign --force --sign - "$B"
codesign --verify --strict "$B"
codesign --verify --strict "$OUT/m2vtest"
xcrun nm -gU "$B/SweetMetal2" > "$WORK/symbols.txt"
grep -q 'OBJC_CLASS_.*SweetMTL2Device' "$WORK/symbols.txt"
mkdir -p "$OUT/licenses/bridge" "$OUT/licenses/turnip"
cp "$WORK"/deps/bridge/LICENSE* "$OUT/licenses/bridge/"
cp "$WORK"/deps/turnip/LICENSE* "$OUT/licenses/turnip/"
cp "$HERE/README.md" "$OUT/"
printf 'bridge run=42217006446 artifact=13504433311\nturnip run=42111679443 artifact=13453813357\n' > "$OUT/SOURCES.txt"
tar -czf "$OUT/frontend-source.tar.gz" -C "$ROOT" userland/metal-frontend userland/metal-bridge
(cd "$OUT" && shasum -a 256 SweetMetal2.bundle/SweetMetal2 SweetMetal2.bundle/libvulkan_freedreno.dylib m2vtest frontend-source.tar.gz > SHA256SUMS)
