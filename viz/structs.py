from __future__ import annotations

from typing import Iterable
from pprint import pprint
from pathlib import Path
from binascii import hexlify
from typing import ClassVar, Any
from ctypes import Structure, sizeof, c_uint32, c_uint8, c_time_t

BLOCK_SIZE = 1024
INODE_PTR_TABLE_SIZE = BLOCK_SIZE // sizeof(c_uint32)

class Block(c_uint8 * BLOCK_SIZE): pass # type: ignore[misc]
class BlockPtr(c_uint32 * INODE_PTR_TABLE_SIZE): # type: ignore[misc]
  @classmethod
  def valid(cls, ptrs: Iterable[tuple[int, int]]) -> Iterable[tuple[int, int]]: return (x for x in ptrs if x[1] != 0)

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
    ("btime", c_time_t), ("nlinks", c_uint32), ("log_size", c_uint32), ("block_cnt", c_uint32), ("_ptrs", c_uint32 * (12 + 3))
  ]
  @classmethod
  def ptr_lvl(cls, idx: int) -> int: return 0 if idx < 12 else idx - 12 + 1
  @classmethod
  def ptr_valid(cls, ptr: int) -> bool: return ptr != 0
  @property
  def ptrs(self) -> Iterable: return ((self.ptr_lvl(i), ptr) for i, ptr in enumerate(self._ptrs))

def ptr_tree_root():
  with Path("../disco_test").open("rb") as f:
    buff = f.read(sizeof(Superblock))
    sb = Superblock.from_buffer_copy(buff)

    # f.seek(sb.bm_start * BLOCK_SIZE)
    # bm = f.read((sb.bm_end - sb.bm_start + 1) * BLOCK_SIZE)

    f.seek(sb.inode_start * BLOCK_SIZE)
    buff = f.read(sizeof(Inode))
    inode = Inode.from_buffer_copy(buff)

    def _valid(ptrs: Iterable[tuple[int, int]]) -> Iterable[tuple[int, int]]: return (x for x in ptrs if x[1] != 0)

    def tree(ptrs: Iterable[tuple[int, int]]):
      ret = []
      for lvl, ptr in BlockPtr.valid(ptrs):
        childs = []
        if lvl > 0:
          f.seek(ptr * BLOCK_SIZE)
          childs = tree((lvl-1, p) for p in BlockPtr.from_buffer_copy(f.read(BLOCK_SIZE)))
        ret.append({ "name": ptr, "children": childs })
      return ret
    return tree(inode.ptrs)

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

    def _valid(ptrs: Iterable[tuple[int, int]]) -> Iterable[tuple[int, int]]: return (x for x in ptrs if x[1] != 0)

    def tree(ptrs: Iterable[tuple[int, int]]):
      ret = []
      for lvl, ptr in BlockPtr.valid(ptrs):
        childs = []
        if lvl > 0:
          f.seek(ptr * BLOCK_SIZE)
          childs = tree((lvl-1, p) for p in BlockPtr.from_buffer_copy(f.read(BLOCK_SIZE)))
        ret.append({ "name": ptr, "children": childs })
      return ret

    pprint(tree(inode.ptrs))
