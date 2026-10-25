#!/usr/bin/env python3
"""Reuse a known bootable alioth v3 shim and ADT with a new XNU kernel.

Normal console text is hidden after the GUI's first swap by default.
Early boot text and panic output can still appear. Persistent logging stays on.
This only writes a new host file; it never contacts or flashes a phone.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import tempfile

import mkpayload


def repack(base_path, kernel_path, output_path, show_console=False):
    base = Path(base_path).read_bytes()
    output = Path(output_path)
    if output.exists():
        raise ValueError('Output already exists; choose a new filename')
    if base[:8] != b'ANDROID!' or struct.unpack_from('<I', base, 40)[0] != 3:
        raise ValueError('Expected an Android v3 boot image')
    align = lambda size: (size + 4095) & ~4095
    kernel_size, ramdisk_size = struct.unpack_from('<II', base, 8)
    rd_off = 4096 + align(kernel_size)
    if len(base) != rd_off + align(ramdisk_size):
        raise ValueError('Unexpected image size or verification trailer')
    old_payload = base[rd_off:rd_off + ramdisk_size]
    if old_payload[:12] != b'SWEETXNU\x01\0\0\0':
        raise ValueError('Expected SWEET payload at ramdisk start')
    adt_off, adt_len = struct.unpack_from('<QQ', old_payload, 0x20)
    if adt_off + adt_len > len(old_payload):
        raise ValueError('Truncated ADT')
    adt = old_payload[adt_off:adt_off + adt_len]
    old_args = old_payload[0x48:0x48 + 608].split(b'\0', 1)[0].decode()
    args = [a for a in old_args.split() if not a.startswith(
        ('sweetfb_noconsole=', 'sweetlog='))]
    args += [f'sweetfb_noconsole={0 if show_console else 1}', 'sweetlog=1']
    cmd = ' '.join(args)
    if len(cmd.encode()) >= 608:
        raise ValueError('Boot arguments exceed payload capacity')
    with tempfile.TemporaryDirectory() as tmp:
        adt_path = Path(tmp) / 'working.adt'
        adt_path.write_bytes(adt)
        payload, info = mkpayload.build(str(kernel_path), str(adt_path), cmd)
    if info['cpusub'] != 0:
        raise ValueError('Expected plain arm64 kernel')
    new_off, new_len = struct.unpack_from('<QQ', payload, 0x20)
    if payload[new_off:new_off + new_len] != adt:
        raise ValueError('ADT changed unexpectedly')
    prefix = bytearray(base[:rd_off])
    struct.pack_into('<I', prefix, 12, len(payload))
    image = prefix + payload + b'\0' * (align(len(payload)) - len(payload))
    if image[4096:rd_off] != base[4096:rd_off]:
        raise ValueError('Shim changed unexpectedly')
    report = dict(base_sha256=hashlib.sha256(base).hexdigest(),
        kernel_sha256=hashlib.sha256(Path(kernel_path).read_bytes()).hexdigest(),
        output_sha256=hashlib.sha256(image).hexdigest(), size=len(image),
        cmdline=cmd, hardware_test='not run')
    with output.open('xb') as stream:
        stream.write(image)
    Path(str(output) + '.json').write_text(json.dumps(report, indent=2))
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('base', help='Known bootable alioth image')
    parser.add_argument('kernel', help='Plain arm64 XNU Mach-O')
    parser.add_argument('output', help='New output filename')
    parser.add_argument('--show-console', action='store_true')
    opts = parser.parse_args()
    print(json.dumps(repack(opts.base, opts.kernel, opts.output, opts.show_console), indent=2))
