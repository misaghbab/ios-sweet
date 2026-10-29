#include <assert.h>
#include "display_plane.h"
#define VALID(w,h,row,off,ps,next,alloc) alioth_display_plane_valid(0x62336138,2,0,w,h,w,h,row,off,ps,next,4,1,1,alloc)
int main(void) {
 assert(VALID(1125,2436,4544,0,11069184,11071488,13893632));
 assert(VALID(1125,2436,4544,64,11069184,11071552,13893696));
 assert(!VALID(1125,2436,4480,0,11069184,11071488,13893632));
 assert(!VALID(1125,2436,4544,0,1,11071488,13893632));
 assert(!VALID(1125,2436,4544,0,11069184,1,13893632));
 assert(!VALID(1125,2436,4544,0,11069184,11071488,1));
 assert(!VALID(1125,2436,4544,1,11069184,11071488,13893632));
 assert(!VALID(0xffffffffu,0xffffffffu,0xffffffc0u,0,0xffffffffu,0xffffffffu,0xffffffffffffffffull));
 assert(!alioth_display_plane_valid(0x42475241,2,0,1,1,1,1,64,0,64,64,4,1,1,128));
 assert(!alioth_display_plane_valid(0x62336138,2,1,1,1,1,1,64,0,64,64,4,1,1,128));
 assert(!alioth_display_plane_valid(0x62336138,2,0,1,1,2,1,64,0,64,64,4,1,1,128));
 return 0;
}
