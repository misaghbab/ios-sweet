#!/usr/bin/env python3
"""Reuse pinned 0101 iig outputs only for identical DriverKit interface sources.

Xcode16 iig emits a newer IORPC ABI. Do not edit that ABI to accommodate a tool.
Fail closed on changed definitions; this is not a general iig replacement.
"""
from pathlib import Path
import argparse,os,hashlib
p=argparse.ArgumentParser();p.add_argument('--def',dest='definition',required=True)
p.add_argument('--header',required=True);p.add_argument('--impl',required=True)
p.add_argument('--framework-name',required=True)
a,rest=p.parse_known_args()
assert a.framework_name=='DriverKit'
ref=Path(os.environ['ALIOTH_IIG_REFERENCE']).resolve()
source=Path(a.definition).resolve();assert source.suffix=='.iig'
current=source.parent
assert current.name=='DriverKit'
refdir=ref/'xnu/iokit/DriverKit'
def digest(path):return hashlib.sha256(path.read_bytes()).digest()
# All sibling definitions can affect inheritance, not just the requested file.
names={f.name for f in current.glob('*.iig')}
assert names=={f.name for f in refdir.glob('*.iig')},'DriverKit definition set changed'
for name in names:
 assert digest(current/name)==digest(refdir/name),'Changed iig definition: '+name
for name in ['IORPC.h','IOTypes.h','IOReturn.h']:
 assert digest(current.parent/'IOKit'/name)==digest(ref/'xnu/iokit/IOKit'/name),name+' changed'
generated=ref/'build/xnu.obj/DEVELOPMENT_ARM64_SWEET/iokit/DEVELOPMENT/DriverKit'
for target,suffix in [(a.header,'.h'),(a.impl,'.iig.cpp')]:
 data=(generated/(source.stem+suffix)).read_bytes()
 assert b'iig(' in data[:200], 'Missing generated-file provenance'
 Path(target).parent.mkdir(parents=True,exist_ok=True);Path(target).write_bytes(data)
