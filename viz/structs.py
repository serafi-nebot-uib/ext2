from __future__ import annotations

from pathlib import Path
from binascii import hexlify
from typing import ClassVar, Any
from ctypes import Structure, sizeof, c_uint32, c_uint8, c_time_t

BLOCK_SIZE = 1024

class CStruct(Structure):
  def print(self): print("\n".join(f"{x}: {getattr(self, x)}" for x, _ in self._fields_))

class Superblock(CStruct):
  _fields_: ClassVar[list[tuple[str, Any]]] = [
    ("bm_start", c_uint32), ("bm_end", c_uint32), ("inode_start", c_uint32), ("inode_end", c_uint32),
    ("data_start", c_uint32), ("data_end", c_uint32), ("inode_root", c_uint32), ("inode_free", c_uint32),
    ("block_free_cnt", c_uint32), ("inode_free_cnt", c_uint32), ("block_cnt", c_uint32), ("inode_cnt", c_uint32)
  ]

class Inode(CStruct):
  _fields_: ClassVar[list[tuple[str, Any]]] = [
    ("_type", c_uint8), ("permission", c_uint8), ("atime", c_time_t), ("mtime", c_time_t), ("ctime", c_time_t),
    ("btime", c_time_t), ("nlinks", c_uint32), ("log_size", c_uint32), ("block_cnt", c_uint32), ("ptrs", c_uint32 * 15)
  ]

if __name__ == "__main__":
  with Path("disco_test").open("rb") as f:
    print("superblock")
    buff = f.read(sizeof(Superblock))
    print(hexlify(buff).decode())
    sb = Superblock.from_buffer_copy(buff)
    sb.print()
    print()

    print("block map")
    f.seek(sb.bm_start * BLOCK_SIZE)
    bm = f.read((sb.bm_end - sb.bm_start + 1) * BLOCK_SIZE)
    # print(hexlify(bm).decode())
    print(len(bm))
    print()

    print("root inode")
    f.seek(sb.inode_start * BLOCK_SIZE)
    buff = f.read(sizeof(Inode))
    print(hexlify(buff).decode())
    inode = Inode.from_buffer_copy(buff)
    inode.print()
    print(", ".join(map(str, inode.ptrs)))
    print()
