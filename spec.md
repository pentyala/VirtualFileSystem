Here is the complete, self-contained **VFS Technical Specification Manual**. This document contains every structural layout, byte boundary, bitmask, and algorithm state machine required to build your filesystem entirely offline.

---

## 1. System Constants & Global Map

Your 2GB file is viewed as an array of uniform **1024-byte (1KB) blocks**.

```
[ Block 0 ]        -> Superblock (Filesystem Metadata)
[ Block 1 ]        -> Root Inode Block (Contains the "/" entry)
[ Block 2 to M ]   -> The Free-Block Allocation Table (Bitmap or Linked List)
[ Block M+1 to N ] -> Data Chunks & Dynamic Nested Inodes (The Data Region)

```

### Core Constraints:

* **Total Volume Capacity:** $2\text{ GB} = 2,147,483,648\text{ bytes}$
* **Total Usable Blocks ($N$):** $2,147,483,648 / 1024 = 2,097,152\text{ blocks}$
* **Maximum Filename Length:** 31 characters + 1 null terminator (`32 bytes`)
* **Sentinel End-of-File (EOF) Marker:** `0xFFFFFFFF` (used for terminal offsets)

---

## 2. In-Memory and On-Disk Binary Layouts

To guarantee exact disk alignments across cross-compilation environments, every structure **must** be enforced with `__attribute__((__packed__))`.

### 2.1 The Superblock (Block 0 Anchor)

Occupies the absolute first 1024 bytes of the storage file.

| Field | Data Type | Size (Bytes) | Description |
| --- | --- | --- | --- |
| `magic` | `uint32_t` | 4 | Unique FS signature identifier: `0xDEADBEEF` |
| `block_size` | `uint32_t` | 4 | Hardcoded block dimension boundary: `1024` |
| `total_blocks` | `uint64_t` | 8 | Total size of disk grid volume: `2097152` |
| `free_blocks` | `uint64_t` | 8 | Dynamic counter tracking remaining unallocated blocks |
| `root_inode_ptr` | `uint32_t` | 4 | Absolute byte pointer to the `/` directory: `1024` |
| `reserved` | `uint8_t[996]` | 996 | Zero-filled padding to round out exactly to 1024 bytes |

### 2.2 The Inode Descriptor Block

Inodes represent *both* files and directories. Directories contain no raw data blocks; instead, they act as directory trees pointing to child inodes.

| Field | Data Type | Size (Bytes) | Description |
| --- | --- | --- | --- |
| `name` | `char[32]` | 32 | Null-terminated string file/folder label identifier |
| `size` | `uint32_t` | 4 | For files: true byte size. For directories: `0` |
| `flags` | `uint8_t` | 1 | Bitmask state identifier (Allocated, Type) |
| `first_block_ptr` | `uint32_t` | 4 | File: Byte offset of first `fnode`. Dir: `0xFFFFFFFF` |
| `next_sibling` | `uint32_t` | 4 | Byte offset of next inode inside the *same* folder directory |
| `first_child` | `uint32_t` | 4 | For directories: Byte offset of the first nested file/folder |
| `reserved` | `uint8_t[15]` | 15 | Padding rounding structural layout dimension to `64 bytes` |

> **Architectural Note:** Because each Inode is exactly 64 bytes, a single 1KB data block can comfortably pack up to 16 individual dynamic Inode entries side-by-side if you choose to bundle them.

### 2.3 The Data Payload Node (`fnode`)

Every chunk dedicated to file content uses this precise 1024-byte envelope.

| Field | Data Type | Size (Bytes) | Description |
| --- | --- | --- | --- |
| `next_block_ptr` | `uint32_t` | 4 | Byte offset of next sequential chunk (`0xFFFFFFFF` if EOF) |
| `valid_bytes` | `uint16_t` | 2 | Bytes utilized in payload segment ($0 \le \text{val} \le 1018$) |
| `payload` | `char[1018]` | 1018 | Raw application user data storage segment |

---

## 3. The Bitmask Flag Specifications

Do not use native compiler bitfields. Use explicit bitwise masking operations against the 1-byte `flags` register inside the Inode layout:

```c
#define VFS_FLAG_FREE      0x00  // Sibling slot is completely unallocated
#define VFS_FLAG_IN_USE    0x01  // Bit 0: Slot contains active metadata (00000001)
#define VFS_FLAG_DIR       0x02  // Bit 1: Node is a directory container (00000010)
#define VFS_FLAG_FILE      0x04  // Bit 2: Node is a regular data file    (00000100)

```

---

## 4. System Call Algorithm State Machines

### 4.1 Path Resolution Protocol (`path_to_offset`)

Translates an absolute literal string path (e.g., `/docs/work/notes.txt`) into the target Inode's absolute physical byte position inside the 2GB file.

1. Read the Superblock at byte 0. Extract `root_inode_ptr` (Byte 1024).
2. Tokenize the input string using `/` as the delimiter boundary array.
3. For each token parsed:
* Load the Inode structure located at the current tracking offset pointer.
* Verify the active structure has the `VFS_FLAG_DIR` marker active.
* Follow the target's `first_child` byte pointer to jump to the directory level.
* Loop through the children layer sequentially by loading the structure found and stepping along `next_sibling` links.
* Compare the structural `name` string with your active token string.
* **Match Found:** Set current tracking offset to this matching node's block location and step to the next token parsing sequence.
* **Match Fails (and `next_sibling` is `0xFFFFFFFF`):** Terminate traversal loop and return `ENOENT` (Error: Path component not found).



### 4.2 The File Append Engine (`my_vfs_write`)

Appends $B$ bytes from an application memory buffer into a target file file descriptor pointer.

1. Track file location via path resolution. Load target Inode into memory buffer.
2. If `first_block_ptr == 0xFFFFFFFF`:
* Scan allocation block map for a free space index.
* Assign the found block's raw byte address to the Inode's `first_block_ptr`.
* Initialize a fresh, empty `fnode` structure layout at that position.


3. Traverse the file's data block link path by loading the `fnode` at `first_block_ptr` and continuously stepping along the `next_block_ptr` addresses until you reach the final chunk (where `next_block_ptr == 0xFFFFFFFF`).
4. **Evaluate Room in Current Tail Block:**
* Calculate space remaining: $\text{Available} = 1018 - \text{valid\_bytes}$.
* If $\text{Available} > 0$, copy incoming memory bytes directly into `payload[valid_bytes]`.
* Update `valid_bytes` counter and increment the Inode's master `size` metric.
* Write the updated tail `fnode` block back to the storage drive space.


5. **Handle Leftover Overflow Bytes:**
* While incoming buffer bytes still remain to be allocated:
* Find a fresh, available 1KB block index using the block tracking allocation engine.
* Update the old tail block's `next_block_ptr` value from `0xFFFFFFFF` to point directly to this newly claimed block address. Flush old block to disk.
* Initialize a blank `fnode` structure wrapper. Fill `payload` up to 1018 bytes max capacity limits.
* Set new block's `next_block_ptr = 0xFFFFFFFF` and update its individual `valid_bytes` counter.
* Flush new block layout to disk.




6. Commit the final modified Inode tracking record back to its metadata position.

### 4.3 The File Read Engine (`my_vfs_read`)

Reads a continuous stream of raw binary data from an active file starting from a specific byte offset locator.

1. Locate the file's Inode descriptor via the path lookup system.
2. If the user's current seek cursor position matches or exceeds the structure's `size` property, return `0` immediately (End of File).
3. Compute the logical block index jump index location:

$$\text{Target Block Chain Jump Index} = \frac{\text{Seek Cursor Position}}{1018}$$


4. Jump along the file's `first_block_ptr` address chain link by link for $N$ iterations matching your calculated Target Block Jump Index to locate the exact `fnode` holding your starting data cursor point.
5. Compute the initial local chunk offset target index:

$$\text{Local Chunk Byte Entry Point} = \text{Seek Cursor Position} \pmod{1018}$$


6. Read data out of the current `fnode`'s payload array starting from your calculated Local Entry Point up to its internal `valid_bytes` limit boundary line.
7. If more bytes are requested by the user, follow the current block's `next_block_ptr` to pull down the next sequential chunk block from disk storage, reading its payload starting from array index position `0`.
8. Update the user application's tracking seek cursor position dynamically to ensure consecutive read operations continue smoothly.

---

## 5. Diagnostic Diagnostic Logging Layouts

To help track code health without an internet connection, format your metrics string layout inside your diagnostic scanner function using this diagnostic block structure template:

```
==================================================
VFS DIAGNOSTIC SCAN REPORT
==================================================
Storage Device Magic      : [0xDEADBEEF / Verified]
Total Volume Capacity    : 2,097,152 Blocks (2048 MB)
Allocated Block Load      : X Blocks
Remaining Storage Overhead: Y Blocks
Current Global Fragment   : Z% 
--------------------------------------------------
Structural Entry Tree Map:
/ (DIR - Inode Pos: 1024)
  ├── docs (DIR - Inode Pos: 2048)
  │   └── notes.txt (FILE - Size: 4100 bytes -> 5 fnodes)
  └── photos (DIR - Inode Pos: 4096)
==================================================

```

This reference specification manual contains all structural configurations, byte parameters, and operational logic flows needed to build the system. You have the full design pattern in hand—good luck coding it!