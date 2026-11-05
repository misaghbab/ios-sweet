# Experimental iOS Turnip build

This builds a library only. The current alioth kernel does **not** implement
the NativeAdreno ABI required by this backend. Do not install this as a
replacement for the working OpenGL libraries or claim Metal is enabled.

Sources:
- Mesa commit `85fda3b36405bcaea8bffc64aa4f2651dc90770d`, tree
  `b72ff1eb2138026ed6aba524770899faabf7d7c8`.
- Eight Mesa patches in order from UniXNU/UniXNU commit
  `9acb29c3dffe6d9480b5523315c39f0fdae54a3d`.
- Build settings and NativeAdrenoABI.h from NWylynko/ios-flame commit
  `745c4341af86089efb0df851fa8a27fb8face624`.

The historical UniXNU revision matches the documented eight-patch set and Mesa
pin; it is a candidate baseline, not proof of the exact checkout used on Pixel 4.
Original per-file notices remain in the patches and header. See the accompanying
UniXNU and ios-flame licenses; Mesa's license is included with build artifacts.
The minimal IOKit declarations compensate for missing iPhoneOS SDK headers.
Mapping constants were checked against this project's XNU IOMapTypes.h.

Run `bash userland/turnip/build.sh` on macOS with Xcode. For an unsuccessful
source setup use a fresh workspace; the script refuses to reuse partial setup.
The GitHub workflow is manual and does not build or flash a phone image.
