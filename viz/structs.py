from __future__ import annotations

from typing import Iterable
from pprint import pprint
from pathlib import Path
from binascii import hexlify
from typing import ClassVar, Any
from ctypes import Structure, sizeof, c_uint32, c_uint8, c_time_t

BLOCK_SIZE = 1024
INODE_PTR_TABLE_SIZE = BLOCK_SIZE // sizeof(c_uint32)
# sizeof(Inode) # TODO: this apparently yiels 112, why?
INODE_SIZE = 128

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
  def _ptr_tree(cls, ptrs: Iterable[tuple[int, int]]):
    ret = []
    for lvl, ptr in BlockPtr.valid(ptrs):
      childs = []
      if lvl > 0:
        f.seek(ptr * BLOCK_SIZE)
        childs = cls._ptr_tree((lvl-1, p) for p in BlockPtr.from_buffer_copy(f.read(BLOCK_SIZE)))
      ret.append({ ptr: childs })
    return ret
  @property
  def ptrs(self) -> Iterable: return [(self.ptr_lvl(i), ptr) for i, ptr in enumerate(self._ptrs)]
  @property
  def ptr_tree(self): return Inode._ptr_tree(self.ptrs)

def inode_offset(f, ninode: int) -> int:
  f.seek(0)
  sb = Superblock.from_buffer_copy(f.read(sizeof(Superblock)))
  return sb.inode_start * BLOCK_SIZE + ninode * INODE_SIZE

def inode_ptr_tree(f, ninode) -> list:
  f.seek(inode_offset(f, ninode))
  inode = Inode.from_buffer_copy(f.read(sizeof(Inode)))

  def tree(ptrs: Iterable[tuple[int, int]]):
    ret = []
    for lvl, ptr in BlockPtr.valid(ptrs):
      childs = []
      if lvl > 0:
        f.seek(ptr * BLOCK_SIZE)
        childs = tree((lvl-1, p) for p in BlockPtr.from_buffer_copy(f.read(BLOCK_SIZE)))
      ret.append({ ptr: childs })
    return ret

  return tree(inode.ptrs)

def inodes(f):
  sb = Superblock.from_buffer_copy(f.read(sizeof(Superblock)))

  inode_offset = sb.inode_start * BLOCK_SIZE
  for i in range(sb.inode_root, sb.inode_free):
    f.seek(inode_offset + i * INODE_SIZE)
    inode = Inode.from_buffer_copy(f.read(sizeof(Inode)))
    print(f"inode {i}")
    pprint(inode.ptr_tree, indent=2, width=1)
    print()

if __name__ == "__main__":
  with Path("disco").open("rb") as f:
    inodes(f)
