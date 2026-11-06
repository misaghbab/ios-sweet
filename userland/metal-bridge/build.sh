#!/bin/bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
HERE=$ROOT/userland/metal-bridge
WORK=$ROOT/metal-bridge-work
BUILD=$WORK/bridge
OUT=$ROOT/out-metal-bridge
PIN=720ef1ce9ae2a2068999a8675717418b9416283e
mkdir -p "$WORK" "$BUILD" "$OUT"
SRC=$WORK/metal2vulkan-$PIN
if [ ! -f "$WORK/prepared" ]; then
  [ ! -d "$SRC" ] || { echo 'Partial source exists; use a fresh work directory' >&2; exit 1; }
  curl --fail --location --retry 3 "https://codeload.github.com/steelbrain/metal2vulkan/tar.gz/$PIN" -o "$WORK/source.tar.gz"
  echo "6232a3df14ba9a666742fe6fde8cf8b4967eb2d438e6b7244659b23cb0fd80a1  $WORK/source.tar.gz" | shasum -a 256 -c -
  tar -xzf "$WORK/source.tar.gz" -C "$WORK"
  # Prevent git apply from discovering the enclosing ios-sweet repository and
  # silently skipping dependency paths outside its current subdirectory.
  git -C "$SRC" init --quiet
  (cd "$HERE" && shasum -a 256 -c PATCHES.sha256)
  for p in "$HERE"/patches/*.patch; do
    (cd "$SRC" && git apply --check "$p" && git apply "$p")
  done
  touch "$WORK/prepared"
fi
grep -q 'pub fn references_resource' "$SRC/crates/metal2vulkan-runtime/src/used_resources.rs"
grep -q 'BLOCK_BYTES: u64 = 8 << 20' "$SRC/crates/metal2vulkan-runtime/src/memory.rs"
LLVM_AS="$(brew --prefix llvm)/bin/llvm-as"
"$LLVM_AS" -o "$WORK/runtime_examples.air" "$SRC/crates/metal2vulkan-validation/fixtures/public/examples/runtime_examples.ll"
"$LLVM_AS" -o "$WORK/function_constant.air" "$SRC/crates/metal2vulkan-validation/fixtures/public/kernel_function_constant_gate_fold.ll"
cat > "$BUILD/Cargo.toml" <<EOF
[package]
name = "native-metal-bridge"
version = "0.1.0"
edition = "2024"
rust-version = "1.98"
[workspace]
[lib]
crate-type = ["staticlib"]
path = "$HERE/NativeMetalBridge.rs"
[dependencies]
metal2vulkan = { path = "$SRC/crates/metal2vulkan", features = ["ash"] }
metal2vulkan-runtime = { path = "$SRC/crates/metal2vulkan-runtime" }
libc = "0.2"
sha2 = { version = "0.10", features = ["asm"] }
[profile.release]
panic = "unwind"
debug = 1
EOF
export NMB_FIXTURE_AIR=$WORK/runtime_examples.air NMB_FC_AIR=$WORK/function_constant.air
export IPHONEOS_DEPLOYMENT_TARGET=16.0
export SDKROOT="$(xcrun --sdk iphoneos --show-sdk-path)"
cargo +1.98.0 generate-lockfile --manifest-path "$BUILD/Cargo.toml" 2>&1 | tee "$WORK/resolve.log"
cargo +1.98.0 build --locked --release --manifest-path "$BUILD/Cargo.toml" --target aarch64-apple-ios --target-dir "$WORK/target" 2>&1 | tee "$WORK/build.log"
LIB=$WORK/target/aarch64-apple-ios/release/libnative_metal_bridge.a
xcrun lipo "$LIB" -verify_arch arm64
xcrun nm -gU "$LIB" > "$WORK/symbols.txt"
grep -q '_nmb_selftest' "$WORK/symbols.txt"
cp "$LIB" "$OUT/"
cp "$BUILD/Cargo.lock" "$OUT/"
cp "$HERE/LICENSE" "$OUT/LICENSE-bridge"
cp "$SRC"/LICENSE* "$OUT/"
# Include the exact modified dependency and bridge sources for reproducibility.
tar --exclude=.git -czf "$OUT/metal2vulkan-source.tar.gz" -C "$WORK" "metal2vulkan-$PIN"
tar -czf "$OUT/bridge-source.tar.gz" -C "$ROOT" userland/metal-bridge
printf 'metal2vulkan=%s\nflame=%s\n' "$PIN" 745c4341af86089efb0df851fa8a27fb8face624 > "$OUT/SOURCES.txt"
(cd "$OUT" && shasum -a 256 *.a *.tar.gz Cargo.lock SOURCES.txt > SHA256SUMS)
