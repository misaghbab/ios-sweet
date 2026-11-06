/* Memory-only diagnostic for sweetnativebuf=1; never submits GPU commands. */
#include <IOKit/IOKitLib.h>
#include <mach/mach.h>
#include <stdint.h>
#include <stdio.h>
#include "NativeAdrenoABI.h"
struct Connection {
    io_connect_t port=0;
    ~Connection(){if(port)IOServiceClose(port);}
    bool open(){
        io_service_t s=IOServiceGetMatchingService(kIOMainPortDefault,IOServiceMatching("SweetNativeMemory"));
        if(!s)return false;
        kern_return_t k=IOServiceOpen(s,mach_task_self(),0,&port);IOObjectRelease(s);return k==0;
    }
    kern_return_t call(uint32_t m,const void *in,size_t n,void *out,size_t size){
        size_t count=size;
        kern_return_t k=IOConnectCallStructMethod(port,m,in,n,out,size?&count:nullptr);
        return !k && count!=size?kIOReturnBadArgument:k;
    }
};
#define REQUIRE(test,label) do {if(!(test)){fprintf(stderr,"NATIVEBUF FAIL: %s\n",label);return 1;}}while(0)
int main(){
    Connection a,b;REQUIRE(a.open()&&b.open(),"service open (requires sweetnativebuf=1)");
    uint64_t wa[2]={},wb[2]={};
    REQUIRE(!a.call(0,nullptr,0,wa,sizeof wa)&&!b.call(0,nullptr,0,wb,sizeof wb),"apertures");
    REQUIRE(wa[1]&&wb[1]&&(wa[0]+wa[1]<=wb[0]||wb[0]+wb[1]<=wa[0]),"aperture separation");
    NativeAdrenoAbiBoAlloc alloc={16384,14,0};NativeAdrenoAbiBoAllocReply x={},y={};
    REQUIRE(!a.call(2,&alloc,sizeof alloc,&x,sizeof x)&&!a.call(2,&alloc,sizeof alloc,&y,sizeof y),"allocate");
    NativeAdrenoAbiBoMap map={x.handle,wb[0],3,0,2,0};NativeAdrenoAbiBoMapReply result={};
    REQUIRE(a.call(3,&map,sizeof map,&result,sizeof result)==kIOReturnBadArgument,"foreign aperture rejected");
    map.iova=wa[0];REQUIRE(!a.call(3,&map,sizeof map,&result,sizeof result)&&result.iova==wa[0],"GPU page mapping");
    map.handle=y.handle;REQUIRE(a.call(3,&map,sizeof map,&result,sizeof result)!=0,"overlap rejected");
    NativeAdrenoAbiBoCpuMap cpu={x.handle};NativeAdrenoAbiBoCpuMapReply token={};
    REQUIRE(!a.call(13,&cpu,sizeof cpu,&token,sizeof token),"CPU token");
    mach_vm_address_t address=0;mach_vm_size_t size=0;
    REQUIRE(!IOConnectMapMemory64(a.port,token.token,mach_task_self(),&address,&size,kIOMapAnywhere),"CPU mapping");
    bool valid=size>=alloc.bytes;
    if(valid){
        volatile uint32_t *p=(volatile uint32_t *)(uintptr_t)address;
        for(unsigned i=0;i<alloc.bytes/4;++i)p[i]=0x51a70000u^i;
        for(unsigned i=0;i<alloc.bytes/4;++i)if(p[i]!=(0x51a70000u^i))valid=false;
    }
    kern_return_t unmapped=IOConnectUnmapMemory64(a.port,token.token,mach_task_self(),address);
    REQUIRE(valid&&!unmapped,"CPU pattern readback and unmap");
    NativeAdrenoAbiBoCpuUnmap cpuEnd={x.handle,token.token,0};
    REQUIRE(!a.call(14,&cpuEnd,sizeof cpuEnd,nullptr,0),"CPU token release");
    NativeAdrenoAbiBoFree release={x.handle};REQUIRE(!a.call(4,&release,sizeof release,nullptr,0),"free");
    REQUIRE(a.call(4,&release,sizeof release,nullptr,0)==kIOReturnBadArgument,"stale handle rejected");
    release.handle=y.handle;REQUIRE(!a.call(4,&release,sizeof release,nullptr,0),"second free");
    puts("NATIVEBUF PASS: isolated windows, GPU mapping, overlap rejection, CPU readback, cleanup; NO GPU execution tested");
    return 0;
}
