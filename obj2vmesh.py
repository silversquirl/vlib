#!/usr/bin/env python3
"""
Usage: python3 obj2vmesh.py <input.obj >output.vmsh
"""

import re
import struct
import sys
from collections import OrderedDict

VMESH_VERSION = 1

pat = re.compile(r"""
^
 (?:
  v \s+
   (?P<vx> [+-]? \d+ \.? \d*) \s+
   (?P<vy> [+-]? \d+ \.? \d*) \s+
   (?P<vz> [+-]? \d+ \.? \d*)
   (?:\s+ (?P<vw> [+-]? \d+ \.? \d*))?
 |
  vt \s+
   (?P<tu> [+-]? \d+ \.? \d*)
   (?:
    \s+ (?P<tv> [+-]? \d+ \.? \d*)
    (?:\s+ (?P<tw> [+-]? \d+ \.? \d*))?
   )?
 |
  vn \s+
   (?P<nx> [+-]? \d+ \.? \d*) \s+
   (?P<ny> [+-]? \d+ \.? \d*) \s+
   (?P<nz> [+-]? \d+ \.? \d*)
 |
  f \s+
   (?P<f0v> \d+)
   (?: / (?P<f0t> \d+)?
    (?: / (?P<f0n> \d+))?
   )?
   \s+
   (?P<f1v> \d+)
   (?: / (?P<f1t> \d+)?
    (?: / (?P<f1n> \d+))?
   )?
   \s+
   (?P<f2v> \d+)
   (?: / (?P<f2t> \d+)?
    (?: / (?P<f2n> \d+))?
   )?
 )?
 \s*
 (?: \#.* )?
$
""", re.ASCII | re.IGNORECASE | re.MULTILINE | re.X)

usew = False
usenorm = False
useuv = False
useuvw = False

verts = [(0.0, 0.0, 0.0, 0.0)]
normals = [(0.0, 0.0, 0.0)]
uvs = [(0.0, 0.0, 0.0)]
tris = []

for line in sys.stdin:
	line = line.strip()
	if not line:
		continue

	m = pat.match(line)
	if not m:
		print("Ignoring unsupported instruction:", line, file=sys.stderr)
		continue

	if m.group("vx"):
		x, y, z = map(float, m.group("vx", "vy", "vz"))
		if m.group("vw"):
			w = float(m.group("vw"))
		else:
			w = 1.0
		if w != 1.0:
			usew = True

		verts.append((x, y, z, w))

	elif m.group("tu"):
		u = float(m.group("tu"))
		if m.group("tv"):
			v = float(m.group("tv"))
		else:
			v = 0.0
		if m.group("tw"):
			w = float(m.group("tw"))
		else:
			w = 0.0
		if w != 0.0:
			useuvw = True

		uvs.append((u, v, w))

	elif m.group("nx"):
		x, y, z = map(float, m.group("nx", "ny", "nz"))
		normals.append((x, y, z))

	elif m.group("f0v"):
		av, bv, cv = map(int, m.group("f0v", "f1v", "f2v"))

		if m.group("f0t"):
			at, bt, ct = map(int, m.group("f0t", "f1t", "f2t"))
			useuv = True
		else:
			at = bt = ct = 0

		if m.group("f0n"):
			an, bn, cn = map(int, m.group("f0n", "f1n", "f2n"))
			usenorm = True
		else:
			an = bn = cn = 0

		tris.append(((av, an, at), (bv, bn, bt), (cv, cn, ct)))

vm_flags = 0
if usew: vm_flags |= 0x01
if usenorm: vm_flags |= 0x02
if useuv:
	vm_flags |= 0x04
	if useuvw: vm_flags |= 0x08

vm_verts = OrderedDict()
vm_tris = []
for tri in tris:
	vm_tri = []
	for v in tri:
		if v not in vm_verts:
			vm_verts[v] = len(vm_verts)
		vm_tri.append(vm_verts[v])

	vm_tris.append(tuple(vm_tri))

def write(b):
	sys.stdout.buffer.write(b)

header = b"\x7fVMESH" + struct.pack("<BBII", VMESH_VERSION, vm_flags, len(vm_verts), len(vm_tris))
write(header)

for vi, ni, ti in vm_verts:
	v, n, t = verts[vi], normals[ni], uvs[ti]
	data = struct.pack("<fff", v[0], v[1], v[2])
	if usew:
		data += struct.pack("<f", v[3])
	if usenorm:
		data += struct.pack("<fff", n[0], n[1], n[2])
	if useuv:
		data += struct.pack("<ff", t[0], t[1])
		if useuvw:
			data += struct.pack("<f", t[2])
	write(data)

for a, b, c in vm_tris:
	write(struct.pack("<III", a, b, c))
