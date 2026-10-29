/* Alioth display bridge: validate the existing BGRA-in-plane0 convention. */
#ifndef ALIOTH_DISPLAY_PLANE_H
#define ALIOTH_DISPLAY_PLANE_H
#include <stdint.h>
static inline int alioth_display_plane_valid(uint32_t format,uint32_t planes,int readonly,
    uint32_t w,uint32_t h,uint32_t pw,uint32_t ph,uint32_t row,uint32_t offset,
    uint32_t plane_size,uint32_t next_offset,uint32_t bpe,uint32_t ew,uint32_t eh,uint64_t allocation)
{
    uint64_t bytes=(uint64_t)row*h, end=(uint64_t)offset+bytes;
    return format==0x62336138u && planes==2 && !readonly && w && h && w==pw && h==ph &&
        bpe==4 && ew==1 && eh==1 && (uint64_t)w*4<=row && !(row&63) && !(offset&63) &&
        bytes<=plane_size && end<=allocation && end<=next_offset;
}
#endif
