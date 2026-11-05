/* Minimal IOKitLib ABI declarations for the iPhoneOS SDK, which omits this header. */
#pragma once
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef mach_port_t io_object_t;
typedef io_object_t io_service_t;
typedef io_object_t io_connect_t;
#define kIOMainPortDefault MACH_PORT_NULL
#define kIOMapAnywhere 0x00000001
#define kIOMapWriteCombineCache 0x00000400
#define kIOReturnBadArgument ((kern_return_t)0xe00002c2)
#define kIOReturnUnsupported ((kern_return_t)0xe00002c7)
#define kIOReturnBusy ((kern_return_t)0xe00002d5)
#define kIOReturnTimeout ((kern_return_t)0xe00002d6)
extern CFMutableDictionaryRef IOServiceMatching(const char *name);
extern io_service_t IOServiceGetMatchingService(mach_port_t mainPort, CFDictionaryRef matching);
extern kern_return_t IOServiceOpen(io_service_t service, task_port_t owner, uint32_t type, io_connect_t *connection);
extern kern_return_t IOServiceClose(io_connect_t connection);
extern kern_return_t IOObjectRelease(io_object_t object);
extern kern_return_t IOConnectCallStructMethod(io_connect_t connection, uint32_t selector,
    const void *input, size_t input_size, void *output, size_t *output_size);
extern kern_return_t IOConnectMapMemory64(io_connect_t connection, uint32_t type,
    task_port_t task, mach_vm_address_t *address, mach_vm_size_t *size, uint32_t options);
extern kern_return_t IOConnectUnmapMemory64(io_connect_t connection, uint32_t type,
    task_port_t task, mach_vm_address_t address);
#ifdef __cplusplus
}
#endif
