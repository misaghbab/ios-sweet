# Experimental Metal translator bridge

Builds an arm64 iOS 16 static library only. This is not a Metal plugin or a boot
image and does not activate Metal. Alioth still needs a compatible kernel ABI,
the Objective-C frontend, linking and device validation.

Bridge and build settings: NWylynko/ios-flame commit
745c4341af86089efb0df851fa8a27fb8face624, phone/metal2/bridge (MIT, LICENSE).
Translator/runtime: steelbrain/metal2vulkan commit
720ef1ce9ae2a2068999a8675717418b9416283e (GPL-2.0-or-later OR LGPL-3.0-or-later).
Seven reference patches are checksum-verified. Pixel4's 0101 padded-writer alias
optimization is omitted until A650 row-padding tests establish compatibility.

Run `bash userland/metal-bridge/build.sh` on macOS with Xcode, Homebrew LLVM,
and Rust 1.98.0 with aarch64-apple-ios. The manual workflow provides these tools.
The source archive hash is pinned. Cargo resolves external crates once per fresh
build, records Cargo.lock in the artifact, then builds with --locked; external
crate versions are not yet pinned across fresh builds. Exact modified dependency
and bridge sources accompany the static library. No proprietary firmware is used.
