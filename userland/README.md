# Alioth GPU userland

Imported from aTechGeekDev/ios-apollo at 4c75a006b27b195818849e0a635b5c26ba44bf56. Source files checked against the pinned Git tree. Licenses and attribution retained.

Build: userland workflow on macos-14, producing sweet-userland with SHA256SUMS. Mesa 25.0.7 is downloaded with its pinned SHA256; no Apple or phone firmware is stored here.

Alioth kernel 0093 has passed the on-device command, indirect-buffer and fence self-tests and registered SweetGPU. The programs and libraries here have NOT yet been tested on alioth. First run sweet-gpu-test, then sweet-gl-test and require hardware renderer plus validated triangle output. Softpipe fallback is not GPU acceleration.

The CoreAnimation bridge additionally requires the IOSurface and injection kernel port (Apollo 0114-0118), which is not yet included. Do not enable it with the current kernel. Building libraries does not install them or change phone partitions.


0096 experimental display-plane path: the bridge tags only b3a8 display mirror
imports with bit30 of its pseudo-fd. The DRM adapter selects new kernel request
0x7d and strips the tag; legacy0x7e remains unchanged. The kernel validates writable
plane0 geometry before exposing this port's BGRA view. Deep sampled textures keep
the old fallback. Matched libraries install under /usr/local/lib/alioth-0096/,
retaining0095. No claim of improved device performance until hardware testing.
