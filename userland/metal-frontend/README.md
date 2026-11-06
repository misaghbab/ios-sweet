# Alioth frontend staging (not built or installed)

Source: NWylynko/ios-flame commit
745c4341af86089efb0df851fa8a27fb8face624, phone/metal2.
NativeBridgeMetalFrontend files carry MIT notices; accompanying LICENSE retained.

Alioth changes to SweetMetal2.mm:
- Remove console output; diagnostics use the existing /var/local/mtl2.log only.
- Remove unconditional backboardd opt-in. A process must match the explicit
  /var/local/mtl2-clients allowlist. Do not use `*` during initial tests.

This directory is experimental source, not a validated driver. The manual
metal-frontend workflow links and signs a bundle and standalone m2vtest using
the two successful pinned CI artifacts. build.sh verifies both library hashes.
No install script is provided. Do not use the Pixel4 install script on alioth.

Before linking:
1. Verify the downloaded bridge library and Turnip hashes.
2. build.sh stages bridge headers from ../metal-bridge; include/ supplies the
   minimal missing IOKit/IOSurface C declarations.
3. Audit the explicit dc civac cache-flush routine against Alioth's user-cache
   instruction permissions and mapping attributes. Retaining reference code is
   not evidence these instructions work on the current kernel.
4. Review runtime-reported formats and device capabilities for A650. Comments
   and policies inherited from the A640 reference are not hardware validation.
5. Build/link/sign separately and inspect imports against iOS16.7.16.

Before enabling the plugin: implement and test the NativeAdreno kernel ABI,
buffer lifetime/cache coherency, Vulkan submissions, then Metal isolated probes.
The existing OpenGL compositor remains the tested path.
