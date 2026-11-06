/* Minimal C ABI used by the plugin's accelerator registry lookup. */
#pragma once
#include <mach/mach.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
kern_return_t IORegistryEntryGetRegistryEntryID(mach_port_t entry, uint64_t *entryID);
#ifdef __cplusplus
}
#endif
