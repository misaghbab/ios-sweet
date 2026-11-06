"""Apple compiler check against the pinned 0101 build tree; not a kernel link."""
from pathlib import Path
import subprocess,shlex,json
root=Path.cwd();tree=root/'native-check/tree';out=root/'out-native-check';out.mkdir(exist_ok=True)
# Buildtree carries patched source. Isolate git so apply never discovers parent repo.
subprocess.run(['git','init','--quiet',str(tree/'xnu')],check=True)
for name in ['0102-alioth-gpu-address-reservation.patch','0103-alioth-native-buffer-client.patch']:
 p=root/'xnu/patches'/name
 subprocess.run(['git','apply','--check',str(p)],cwd=tree/'xnu',check=True)
 subprocess.run(['git','apply',str(p)],cwd=tree/'xnu',check=True)
reports=[]
for group,source in [('bsd','bsd/dev/sweet_gpu.c'),('iokit','iokit/Kernel/arm/SweetGPU.cpp')]:
 cwd=tree/f'build/xnu.obj/DEVELOPMENT_ARM64_SWEET/{group}/DEVELOPMENT'
 def flags(name):
  s=(cwd/name).read_text().replace('\0','').strip()
  s=s.replace('-DCONFIG_NMBCLUSTERS=((1024 * 512) / MCLBYTES)',"'-DCONFIG_NMBCLUSTERS=((1024 * 512) / MCLBYTES)'")
  return shlex.split(s)[1:]
 args=flags('.CFLAGS')+(flags('.CXXFLAGS') if group=='iokit' else [])
 old='/Users/runner/work/ios-sweet/ios-sweet/dxb'
 args=[('-I'+str(tree/'sdk-kernel-platform/platform') if a.startswith('-I/Applications/') else a.replace(old,str(tree))) for a in args]
 cc=subprocess.check_output(['xcrun','--find','clang++' if group=='iokit' else 'clang'],text=True).strip()
 command=[cc,*args,'-c',str(tree/'xnu'/source),'-o',str(out/(Path(source).name+'.o'))]
 r=subprocess.run(command,cwd=cwd,capture_output=True,text=True)
 (out/(group+'.log')).write_text(r.stdout+r.stderr)
 reports.append(dict(source=source,returncode=r.returncode,command=command))
 print(source,r.returncode,r.stderr[-6000:],flush=True)
(out/'report.json').write_text(json.dumps(reports,indent=2))
raise SystemExit(any(r['returncode'] for r in reports))
