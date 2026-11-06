# Native buffer bring-up tests

Run `python3 run_test.py` on Linux with a C++17 compiler and ASan/UBSan.
The test extracts the actual code from patch0103 and exercises it against mock
IOKit memory descriptors and page-table operations. It covers per-client address
windows, exact wire sizes, failed allocations, partial-map rollback, overlapping
maps, stale handles, retained CPU mappings and repeated close. It cannot validate
SMMU/cache behavior, Apple kernel allocation metadata or real user-client plumbing.

Patch0103 registers SweetNativeMemory only with sweetnativebuf=1. This is a
memory diagnostic service, not NativeAdrenoGpuPublished or IOAcceleratorES.
Selector0 returns two uint64 values (aperture start,size). NativeAdreno selectors
2/3/4/13/14 implement allocation/map/free/CPU token operations; Query, submission,
host import and other methods are deliberately unsupported. No Metal capability
is advertised. CPU unmap acknowledges a matching token; userspace must still
call IOConnectUnmapMemory64 to unmap its own VM region.

Before adding submissions, change free/close to wait for every referring fence
and quarantine stalled objects/windows. The current immediate cleanup is valid
only because this client cannot submit GPU work. Follow the existing global GPU
lock after the client lock, never acquire them in reverse order.
