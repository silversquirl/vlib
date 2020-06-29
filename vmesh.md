# vmesh

A simple 3D model format designed for use on the GPU. Models are stored as triangle meshes, with indexed vertices.
Conversion from Wavefront OBJ is quite straightforward, and a Python script (`obj2vmesh.py`) is available to do so.

WARNING: this spec is under development and may change drastically and in backwards-incompatible ways without a version number change.

## Format

A vmesh file is composed of 3 sections: the file header, the vertex table and the triangle table.

The file header stores general information about the format of the file.
The vertex table stores vertex data for indexing into from the triangle table.
The triangle table stores triples of vertex indices that construct triangles.

The vertex and triangle tables are stored as contiguous sequences of entries, the number of
which is determined by the respective counts in the file header.

All integers are stored as unsigned little-endian.
All floats are stored as IEEE 754 binary32 little-endian. Only normal values are supported; subnormals, NaN and infinities are invalid.

### File Header (16 bytes)

| Bytes | Description                 |
|-------|-----------------------------|
| 6     | Magic value (`"\x7fVMESH"`) |
| 1     | Version (`1`)               |
| 1     | Flags (see table below)     |
| 4     | Vertex count (32-bit int)   |
| 4     | Triangle count (32-bit int) |

#### Flags

| Hex | Description             |
|-----|-------------------------|
| 01  | Vertices include W      |
| 02  | Vertices include normal |
| 04  | Vertices include UVs    |
| 08  | Vertex UVs include W    |

### Vertex table entry (12-40 bytes)

| Bytes | Description                                |
|-------|--------------------------------------------|
| 4     | X coordinate (float)                       |
| 4     | Y coordinate (float)                       |
| 4     | Z coordinate (float)                       |
| 4     | W coordinate (float) (only if 01 flag set) |
| 4     | Normal X (float) (only if 02 flag set)     |
| 4     | Normal Y (float) (only if 02 flag set)     |
| 4     | Normal Z (float)  (only if 02 flag set)    |
| 4     | Texture U (float) (only if 04 flag set)    |
| 4     | Texture V (float) (only if 04 flag set)    |
| 4     | Texture W (float) (only if 08 flag set)    |

### Triangle table entry (12 bytes)

| Bytes | Description                         |
|-------|-------------------------------------|
| 4     | Index of first vertex (32-bit int)  |
| 4     | Index of second vertex (32-bit int) |
| 4     | Index of third vertex (32-bit int)  |
