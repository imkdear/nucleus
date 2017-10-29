/**
 * (c) 2014-2016 Alexandro Sanchez Bach. All rights reserved.
 * Released under GPL v2 license. Read LICENSE for more details.
 */

#include "sys_rsx.h"
#include "nucleus/system/scei/cellos/lv1/lv1_gpu.h"
#include "nucleus/gpu/rsx/rsx.h"
#include "nucleus/logger/logger.h"
#include "nucleus/emulator.h"

namespace sys {

HLE_FUNCTION(sys_rsx_device_open)
{
    return CELL_OK;
}

HLE_FUNCTION(sys_rsx_device_close)
{
    return CELL_OK;
}

/**
 * LV2 SysCall 668 (0x29C): sys_rsx_memory_allocate
 * Allocate space in the RSX local memory.
 *  - mem_handle (OUT): Context / ID, which is used by sys_rsx_memory_free to free allocated memory.
 *  - mem_addr (OUT): Returns the local memory base address, usually 0xC0000000.
 *  - size (IN): Local memory size. E.g. 0x0F900000 (249 MB).
 *  - flags (IN): E.g. Immediate value passed in cellGcmSys is 8.
 *  - a5 (IN): E.g. Immediate value passed in cellGcmSys is 0x00300000 (3 MB?).
 *  - a6 (IN): E.g. Immediate value passed in cellGcmSys is 16.
 *  - a7 (IN): E.g. Immediate value passed in cellGcmSys is 8.
 */
HLE_FUNCTION(sys_rsx_memory_allocate, BE<U32>* mem_handle, BE<U64>* mem_addr, U32 size, U64 flags, U64 a5, U64 a6, U64 a7)
{
    // LV1 Syscall: lv1_gpu_memory_allocate (0xD6)
    const U32 addr = kernel.memory->getSegment(mem::SEG_RSX_LOCAL_MEMORY).alloc(size);
    if (!addr) {
        return CELL_EINVAL;
    }

    *mem_handle = addr; // HACK: On the PS3, this is: *mem_handle = id ^ 0x5A5A5A5A
    *mem_addr = addr;
    return CELL_OK;
}

/**
 * LV2 SysCall 669 (0x29D): sys_rsx_memory_free
 *  - mem_handle (OUT): Context / ID, for allocated local memory generated by sys_rsx_memory_allocate
 */
HLE_FUNCTION(sys_rsx_memory_free, U32 mem_handle)
{
    // LV1 Syscall: lv1_gpu_memory_free (0xD8)
    kernel.memory->getSegment(mem::SEG_RSX_LOCAL_MEMORY).free(mem_handle);

    return CELL_OK;
}

/**
 * LV2 SysCall 670 (0x29E): sys_rsx_context_allocate
 *  - context_id (OUT): RSX context, E.g. 0x55555555 (in vsh.self)
 *  - lpar_dma_control (OUT): Control register area. E.g. 0x60100000 (in vsh.self)
 *  - lpar_driver_info (OUT): RSX data like frequencies, sizes, version... E.g. 0x60200000 (in vsh.self)
 *  - lpar_reports (OUT): Report data area. E.g. 0x60300000 (in vsh.self)
 *  - mem_ctx (IN): mem_ctx given by sys_rsx_memory_allocate
 *  - system_mode (IN):
 */
HLE_FUNCTION(sys_rsx_context_allocate, BE<U32>* context_id, BE<U64>* lpar_dma_control, BE<U64>* lpar_driver_info, BE<U64>* lpar_reports, U64 mem_ctx, U64 system_mode)
{
    U32 ret = lv1_gpu_context_allocate(kernel,  context_id, lpar_dma_control, lpar_driver_info, lpar_reports, mem_ctx, system_mode);
    if (ret != LV1_SUCCESS) {
        return CELL_EINVAL;
    }

    return CELL_OK;
}

/**
 * LV2 SysCall 671 (0x29F): sys_rsx_context_free
 *  - context_id (IN): RSX context generated by sys_rsx_context_allocate to free the context.
 */
HLE_FUNCTION(sys_rsx_context_free, U32 context_id)
{
    return CELL_OK;
}

/**
 * LV2 SysCall 672 (0x2A0): sys_rsx_context_iomap
 *  - context_id (IN): RSX context, E.g. 0x55555555 (in vsh.self)
 *  - io (IN): IO offset mapping area. E.g. 0x00600000
 *  - ea (IN): Start address of mapping area. E.g. 0x20400000
 *  - size (IN): Size of mapping area in bytes. E.g. 0x00200000
 *  - flags (IN):
 */
HLE_FUNCTION(sys_rsx_context_iomap, U32 context_id, U32 io, U32 ea, U32 size, U64 flags)
{
    gpu::rsx::rsx_iomap_t iomap;
    iomap.io = io;
    iomap.ea = ea;
    iomap.size = size;

    // TODO: Implement flags
    static_cast<gpu::rsx::RSX*>(kernel.getEmulator()->gpu.get())->iomaps.push_back(iomap);
    return CELL_OK;
}

/**
 * LV2 SysCall 673 (0x2A1): sys_rsx_context_iounmap
 *  - context_id (IN): RSX context, E.g. 0x55555555 (in vsh.self)
 *  - a2 (IN): ?
 *  - io_addr (IN): IO address. E.g. 0x00600000 (Start page 6)
 *  - size (IN): Size to unmap in byte. E.g. 0x00200000
 */
HLE_FUNCTION(sys_rsx_context_iounmap, U32 context_id, U32 a2, U32 io_addr, U32 size)
{
    logger.warning(LOG_HLE, "LV2 Syscall (0x2A1) called: sys_rsx_context_iounmap");
    return CELL_OK;
}

// LV2 Syscall 674 (0x2A2): sys_rsx_context_attribute
HLE_FUNCTION(sys_rsx_context_attribute, S32 context_id, U32 operation_code, U64 p1, U64 p2, U64 p3, U64 p4)
{
    U32 ret = lv1_gpu_context_attribute(kernel, context_id, operation_code, p1, p2, p3, p4);
    if (ret != LV1_SUCCESS) {
        return CELL_EINVAL;
    }

    return CELL_OK;
}

/**
 * LV2 SysCall 675 (0x2A3): sys_rsx_device_map
 *  - a1 (OUT): For example: In vsh.self it is 0x60000000, global semaphore. For a game it is 0x40000000.
 *  - a2 (OUT): Unused?
 *  - dev_id (IN): An immediate value and always 8. (cellGcmInitPerfMon uses 11, 10, 9, 7, 12 successively).
 */
HLE_FUNCTION(sys_rsx_device_map, BE<U32>* mapped_addr, BE<U32>* a2, U32 dev_id)
{
    if (dev_id > 15) {
        return CELL_EINVAL;
    }

    if ((dev_id == 0 || dev_id > 8)/*&& !sub_52450()*/) {
        return CELL_EPERM;
    }

    // LV1 Syscall: lv1_gpu_device_map (0xD4)
    switch (dev_id) {
    case 8:
        // HACK: We already store data in the memory on RSX initialization, mapping is not necessary
        *mapped_addr = 0x40000000;
        break;
    }

    return CELL_OK;
}

/**
 * LV2 SysCall 676 (0x2A4): sys_rsx_device_unmap
 *  - dev_id (IN): An immediate value and always 8.
 */
HLE_FUNCTION(sys_rsx_device_unmap, U32 dev_id)
{
    return CELL_OK;
}

/**
 * LV2 SysCall 677 (0x2A5): sys_rsx_attribute
 */
HLE_FUNCTION(sys_rsx_attribute, U32 a1, U32 a2, U32 a3, U32 a4, U32 a5)
{
    // LV1 Syscall: lv1_gpu_attribute (0xE4)
    switch (a1) {
    case L1GPU_ATTRIBUTE_UNK002:
    case L1GPU_ATTRIBUTE_UNK100:
    case L1GPU_ATTRIBUTE_UNK105:
    case L1GPU_ATTRIBUTE_UNK202:
    case L1GPU_ATTRIBUTE_UNK400:
    case L1GPU_ATTRIBUTE_UNK401:
    case L1GPU_ATTRIBUTE_UNK402:
    case L1GPU_ATTRIBUTE_UNK403:
        break;

    default:
        break;
    }

    return CELL_OK;
}

}  // namespace sys
