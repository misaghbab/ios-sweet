#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <set>
#include <vector>
#include <mutex>
using UInt32=uint32_t;using IOOptionBits=uint32_t;using IOReturn=int;
using IOByteCount=uint64_t;using addr64_t=uint64_t;using vm_size_t=uint64_t;
enum {kIOReturnSuccess=0,kIOReturnBadArgument=1,kIOReturnUnsupported=2,kIOReturnNotOpen=3,kIOReturnNoResources=4,kIOReturnNoMemory=5};
enum {kIODirectionInOut=1,kIOMemoryKernelUserShared=2,kIOMapWriteCombineCache=4,kIOMemoryMapperNone=0,kernel_task=0};
static constexpr const char *kIOUserClientClassKey="client";
using IOLock=std::mutex;
static IOLock global;static IOLock *gGPULock=&global;
static IOLock *IOLockAlloc(){return new IOLock;}
static void IOLockFree(IOLock*p){delete p;}
static void IOLockLock(IOLock*p){p->lock();}
static void IOLockUnlock(IOLock*p){p->unlock();}
#define APPLE_KEXT_OVERRIDE override
#define OSDeclareDefaultStructors(T) public: T()=default; private:
#define OSDefineMetaClassAndStructors(T,B)
#define OSTypeAlloc(T) new T()
#define IONewZero(T,n) new T[n]()
#define IODelete(p,T,n) delete[] p
static uint64_t round_page_64(uint64_t n){return (n+16383)&~16383ull;}
struct OSObject {virtual ~OSObject()=default;};
struct IOService:OSObject {
 virtual bool start(IOService*){return true;}
 bool init(){return true;}bool attach(IOService*){return true;}
 void detach(IOService*){}void release(){delete this;}
 void setProperty(const char*,const char*){}void setName(const char*){}void registerService(){}
};
static int liveMemory=0;static bool failAlloc=false;static int failMapAfter=-1;
struct IOMemoryDescriptor:OSObject {
 int refs=1;void retain(){++refs;}void release(){if(!--refs)delete this;}
};
struct IOBufferMemoryDescriptor:IOMemoryDescriptor {
 std::vector<unsigned char> bytes;
 explicit IOBufferMemoryDescriptor(size_t n):bytes(n){++liveMemory;}
 ~IOBufferMemoryDescriptor(){--liveMemory;}
 static IOBufferMemoryDescriptor *inTaskWithOptions(int,int,vm_size_t n,vm_size_t){return failAlloc?nullptr:new IOBufferMemoryDescriptor(n);}
 void *getBytesNoCopy(){return bytes.data();}
 addr64_t getPhysicalSegment(uint64_t off,IOByteCount*len,int){*len=bytes.size()-off;return 0x10000000+off;}
};
struct IOExternalMethodDispatch{};
struct IOExternalMethodArguments {
 uint32_t scalarInputCount=0,scalarOutputCount=0;
 void *structureInputDescriptor=nullptr,*structureOutputDescriptor=nullptr;
 const void *structureInput=nullptr;void *structureOutput=nullptr;
 size_t structureInputSize=0,structureOutputSize=0;
};
struct IOUserClient:IOService {
 virtual IOReturn clientClose(){return 0;} virtual void free(){}
 bool isInactive(){return false;}void terminate(){}
 virtual IOReturn clientMemoryForType(UInt32,IOOptionBits*,IOMemoryDescriptor**){return 0;}
 virtual IOReturn externalMethod(uint32_t,IOExternalMethodArguments*,IOExternalMethodDispatch*,OSObject*,void*){return 0;}
};
static std::set<uint64_t> windows,pages;
static bool sweet_gpu_ready(){return true;}
extern "C" bool sweet_gpu_address_reserve(uint64_t a,uint64_t){return windows.insert(a).second;}
static void sweet_gpu_address_free(uint64_t a,uint64_t){assert(windows.erase(a)==1);}
static bool sweet_gpu_map(uint64_t a,uint64_t,bool){if(failMapAfter==0)return false;if(failMapAfter>0)--failMapAfter;return pages.insert(a).second;}
static void sweet_gpu_unmap(uint64_t a,uint64_t n){for(uint64_t i=0;i<n;i+=4096)pages.erase(a+i);}
static void sweet_gpu_mapped(){}
#include "native-memory/SweetNativeMemory.inc"
static IOReturn invoke(SweetNativeMemoryUserClient&c,uint32_t m,const void*in,size_t ni,void*out,size_t no){
 IOExternalMethodArguments a;a.structureInput=in;a.structureInputSize=ni;a.structureOutput=out;a.structureOutputSize=no;
 return c.externalMethod(m,&a,nullptr,nullptr,nullptr);
}
int main(){
 IOService provider;SweetNativeMemoryUserClient a,b;
 assert(a.start(&provider)&&b.start(&provider));
 uint64_t wa[2],wb[2];assert(!invoke(a,0,nullptr,0,wa,sizeof wa));assert(!invoke(b,0,nullptr,0,wb,sizeof wb));assert(wa[0]!=wb[0]);
 NativeAdrenoAbiBoAlloc req{8192,12,0};NativeAdrenoAbiBoAllocReply x{},y{};
 assert(invoke(a,1,nullptr,0,nullptr,0)==kIOReturnUnsupported);
 assert(invoke(a,2,&req,1,&x,sizeof x)==kIOReturnBadArgument);
 failAlloc=true;assert(invoke(a,2,&req,sizeof req,&x,sizeof x)==kIOReturnNoMemory);failAlloc=false;
 assert(!invoke(a,2,&req,sizeof req,&x,sizeof x));assert(!invoke(a,2,&req,sizeof req,&y,sizeof y));
 NativeAdrenoAbiBoMap map{x.handle,wb[0],3,0,2,0};NativeAdrenoAbiBoMapReply mapped{};
 assert(invoke(a,3,&map,sizeof map,&mapped,sizeof mapped)==kIOReturnBadArgument);
 map.iova=wa[0];failMapAfter=1;
 assert(invoke(a,3,&map,sizeof map,&mapped,sizeof mapped)==kIOReturnNoMemory);assert(pages.empty());
 failMapAfter=-1;assert(!invoke(a,3,&map,sizeof map,&mapped,sizeof mapped));assert(pages.size()==2);
 map.handle=y.handle;assert(invoke(a,3,&map,sizeof map,&mapped,sizeof mapped)==kIOReturnNoResources);
 IOMemoryDescriptor *md=nullptr;IOOptionBits options=0;
 assert(!a.clientMemoryForType((uint32_t)x.handle,&options,&md));
 NativeAdrenoAbiBoFree release{x.handle};assert(!invoke(a,4,&release,sizeof release,nullptr,0));assert(pages.empty());
 assert(invoke(a,4,&release,sizeof release,nullptr,0)==kIOReturnBadArgument);
 assert(!invoke(a,2,&req,sizeof req,&x,sizeof x));assert(x.handle!=release.handle);
 assert(!a.clientClose());assert(invoke(a,0,nullptr,0,wa,sizeof wa)==kIOReturnNotOpen);assert(!a.clientClose());
 a.free();b.clientClose();b.free();assert(windows.empty()&&pages.empty());
 assert(liveMemory==1);md->release();assert(liveMemory==0);
 puts("PASS: client isolation, wire sizes, allocation failure, partial map rollback, overlap, stale handles, retained CPU mapping, repeated close");
}
