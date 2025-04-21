from __future__ import annotations

from typing import Iterable
from pprint import pprint
from pathlib import Path
from binascii import hexlify
from typing import ClassVar, Any
from math import ceil
from ctypes import Structure, sizeof, c_uint32, c_uint8, c_time_t

import matplotlib.pyplot as plt
import numpy as np

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
  @property
  def ptrs(self) -> Iterable: return [(self.ptr_lvl(i), ptr) for i, ptr in enumerate(self._ptrs)]

def inode_offset(f, ninode: int) -> int:
  f.seek(0)
  sb = Superblock.from_buffer_copy(f.read(sizeof(Superblock)))
  return sb.inode_start * BLOCK_SIZE + ninode * INODE_SIZE

def inode_ptr_tree(f, ninode: int):
  f.seek(inode_offset(f, ninode))
  inode = Inode.from_buffer_copy(f.read(sizeof(Inode)))

  def ptr_tree(ptrs: Iterable[tuple[int, int]]):
    ret = []
    for lvl, ptr in BlockPtr.valid(ptrs):
      childs = []
      if lvl > 0:
        f.seek(ptr * BLOCK_SIZE)
        childs = ptr_tree((lvl-1, p) for p in BlockPtr.from_buffer_copy(f.read(BLOCK_SIZE)))
      ret.append({ "name": ptr, "children": childs })
    return ret

  return ptr_tree(inode.ptrs)

def inode_ptr_tree_from_file(path: str, ninode: int):
  with Path(path).open("rb") as f: return inode_ptr_tree(f, ninode)

def inodes(f):
  sb = Superblock.from_buffer_copy(f.read(sizeof(Superblock)))
  for i in range(sb.inode_root, sb.inode_free):
    tree = inode_ptr_tree(f, i)
    print(f"inode {i}")
    pprint(tree, indent=2, width=1)
    print()

def block_map(f):
    f.seek(0)
    sb = Superblock.from_buffer_copy(f.read(sizeof(Superblock)))

    f.seek(sb.bm_start * BLOCK_SIZE)
    size = ceil(sb.block_cnt / 8)
    bm = (c_uint8 * size).from_buffer_copy(f.read(size))

    table = [
        (range(0, sb.bm_start),                   1.00, "superblock"), # noqa: PIE808
        (range(sb.bm_start, sb.bm_end + 1),       0.75, "block map"),
        (range(sb.inode_start, sb.inode_end + 1), 0.50, "inode array"),
        (range(sb.data_start, sb.data_end + 1),   0.25, "data"),
        (range(sb.data_end + 1, sb.block_cnt),    0.00, "free")
    ]
    print(size)
    print(table)
    res = []
    block = 0
    for i in range(size):
      v = bm[i]
      for _ in range(8):
        c = next((c for r, c, *_ in table if block in r))
        m = float(v & 0x80 != 0)
        res.append(min(m, c))
        v = (v << 1) & 0xff
        block += 1

    rows = sb.block_cnt // 100
    cols = ceil(sb.block_cnt / rows)
    data = np.array(res).reshape(rows, cols).transpose()

    plt.imshow(data, cmap="viridis", vmin=0.0, vmax=1.0)
    cbar = plt.colorbar()
    cbar.set_ticks([x[1] for x in table])
    cbar.set_ticklabels([x[2] for x in table])
    plt.title("Block Map")
    plt.show()

if __name__ == "__main__":
  with Path("disco").open("rb") as f:
    print(inode_ptr_tree(f, 1))
    # inodes(f)
    # block_map(f)
