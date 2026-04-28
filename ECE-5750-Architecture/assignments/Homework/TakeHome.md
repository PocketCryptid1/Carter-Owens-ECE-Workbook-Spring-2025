# ECE 5750 Take Home Final

Carter Owens

---

## Question 1

### (a)

An I-cache miss stalls the front-end (Fetch stage). Since the front-end is in-order, no new instructions can enter the pipeline during the miss. The OoO back-end will keep executing whatever is already in the ROB/instruction window, but once that drains the whole pipeline just sits idle. There is no way to hide the miss because nothing new is coming in.

A D-cache miss stalls a load in the memory stage, which is in the OoO back-end. The front-end is unaffected and keeps fetching, so the ROB fills up with future instructions. The OoO engine can issue and execute any of those that dont depend on the missing load, so some or all of the miss latency gets hidden.

So even with the same miss penalty M, an I-cache miss hurts more because the pipeline cant overlap the miss with any useful work. A D-cache miss can be partially hidden by OoO execution of independent instructions.

---

### (b)

Additional assumptions (parametric):

- f_i = I-cache miss rate (misses per instruction)
- f_d = D-cache miss rate (misses per instruction)
- I = issue width (instructions per cycle, peak)
- R = ROB size (entries)
- W = instruction window size (entries)
- M = miss penalty (cycles), same for I-cache and D-cache
- Pipeline front-end depth (Fetch to Dispatch) = F stages (assume F = 4 for this pipeline: Fetch, Decode, Rename, Dispatch)

**I-cache miss penalty model:**

When an I-cache miss happens, fetch stalls for M cycles and the back-end drains. The ROB holds at most R in-flight instructions and the machine issues at width I, so the back-end stays busy for at most R/I cycles before it goes idle.

Effective stall cycles per I-cache miss = max(M - R/I, 0)

If M > R/I, the ROB drains completely and the pipeline is idle for (M - R/I) cycles.
If M <= R/I, the ROB never fully drains and the miss is partially hidden.

CPI penalty per I-cache miss:

  penalty_I = max(M - R/I, 0)

CPI contribution from I-cache misses:

  CPI_icache = f_i * max(M - R/I, 0)

**D-cache miss penalty model:**

A D-cache miss takes up an ROB entry and stalls dependent instructions. Independent instructions keep executing. The miss is fully hidden if there are enough independent instructions in the window to keep issue busy for the whole M cycles, i.e., W >= I * M.

If W < I * M, the window drains after W/I cycles and the pipeline stalls for the rest.

Effective stall cycles per D-cache miss = max(M - W/I, 0)

CPI contribution from D-cache misses:

  CPI_dcache = f_d * max(M - W/I, 0)

**Full CPI model:**

  CPI = CPI_ideal + CPI_icache + CPI_dcache

  CPI = CPI_ideal + f_i x max(M - R/I, 0) + f_d x max(M - W/I, 0)

Where CPI_ideal is the base CPI with no misses (1/I for a perfectly utilized I-wide machine, but treated as a given parameter here).

---

## Question 2

### Table III-B

| initial state | other cached | ops | actions by this cache | final state | this cache | other caches | mem |
| --- | --- | --- | --- | --- | --- | --- | --- |
| cleanExclusive | no | none | none | CE | yes | | yes |
| | | CPU read | none | CE | yes | | yes |
| | | CPU write | none | OE | yes | | |
| | | replace | none | I | | | yes |
| | | CR | none | CS | yes | yes | yes |
| | | CRI | none | I | | yes | |
| | | CI | | impossible | | | |
| | | WR | | impossible | | | |
| | | CWI | none | I | | | yes |

Notes:

- CPU write: already exclusive so no bus tx needed, just mark dirty, CE -> OE
- CR: CE is clean so memory supplies the data; this cache downgrades to CS, requesting cache gets CS
- CRI: requesting cache gets OE (dirty exclusive); this cache invalidates; memory is now stale
- CI: impossible, no other cache has a shared copy (other cached = no)
- WR: impossible, no other cache has this block
- CWI: DMA overwrites block in memory; cached copy invalidated

---

### Table III-C

| initial state | other cached | ops | actions by this cache | final state | this cache | other caches | mem |
| --- | --- | --- | --- | --- | --- | --- | --- |
| ownedExclusive | no | none | none | OE | yes | | |
| | | CPU read | none | OE | yes | | |
| | | CPU write | none | OE | yes | | |
| | | replace | WR (writeback) | I | | | yes |
| | | CR | CCI | OS | yes | yes | |
| | | CRI | CCI | I | | yes | |
| | | CI | | impossible | | | |
| | | WR | | impossible | | | |
| | | CWI | none | I | | | yes |

Notes:

- CPU read/write: hit on dirty exclusive block, no state change, memory stays stale
- replace: block is dirty so must writeback via WR before eviction; memory updated
- CR: memory is stale (OE is dirty) so this cache must supply via CCI; downgrades OE -> OS, requesting cache gets CS; memory still stale
- CRI: requesting cache wants to write; this cache supplies via CCI then invalidates; requesting cache gets OE; memory still stale
- CI: impossible, OE is exclusive so no other shared copies exist
- WR: impossible, no other cache has this block
- CWI: DMA full-block write overrides the dirty copy; this cache invalidates; DMA data goes to memory

---

## Question 3

### (A)

Standard LRU is not a great policy for a directory cache. Evicting a directory entry is way more expensive than evicting a regular cache line because you have to send invalidations to every sharer, which wastes network bandwidth and stalls those caches. LRU ignores all of that cost.

A better approach is to pick the block that is cheapest to evict:

1. Prefer evicting Shared blocks over Modified blocks. Modified blocks require a dirty writeback, which is extra work compared to just invalidating clean copies.
2. Among same-state blocks, prefer fewer sharers since that means fewer invalidation messages.
3. Use LRU only as a tiebreaker.

Applying this to the given set:

- Way 0: Shared, sharers = popcount(0xbb) = 6
- Way 1: Shared, sharers = popcount(0xfa) = 6
- Way 2: Modified, sharers = popcount(0x4) = 1
- Way 3: Modified, sharers = popcount(0x8) = 1

Step 1: prefer Shared over Modified, so candidates are Way 0 and Way 1.

Step 2: both have 6 sharers, so use LRU as tiebreaker. Way 1 has last use 4577 (older).

Choice: replace Way 1.

Justification: replacing a Shared block avoids a dirty writeback. Way 1 is the least recently used of the two Shared candidates and has the same sharer count, so it is the least costly to evict.

---

### (B)

This is a race condition. The directory sends invalidations at T=10000 but Cache 0's upgrade request is already in flight before it receives the INV.

Timeline:

- T=10000: directory starts eviction of X (Shared, sharers=0x7). Sends INV to Cache 0, 1, 2.
- T=10050: Cache 0 sends Upgrade (GetM) for X to directory. Still in transit.
- T=10100: Cache 0, 1, and 2 all receive INV from directory.
- T=10150: directory receives Cache 0's Upgrade.

By T=10150 the directory has already evicted X and has no entry for it. A naive implementation would either drop the request or handle it incorrectly.

To fix this, add a Pending Eviction (PE) state to the directory.

When the directory decides to evict a block, it does not immediately remove the entry. It moves the entry to PE state and sets an ACK counter equal to the number of sharers (3 here). It sends INV to all sharers and waits for acknowledgment.

While in PE state, the directory still handles requests for that block:

1. INV-ACK from a sharer: decrement the counter. When it hits 0 and no other requests are pending, delete the entry.

2. Upgrade request while in PE (Cache 0 at T=10150): the directory needs to handle this carefully. Since Cache 0 was a sharer, its INV is in flight anyway. The directory can treat the Upgrade as an implicit ACK from Cache 0, re-allocate the entry in Modified state, assign ownership to Cache 0, and wait for the remaining ACKs from Cache 1 and Cache 2 before sending Cache 0 a grant.

If the request came from a cache that was NOT a sharer, the directory re-allocates the entry normally and processes the miss.

Summary:

- Directory uses a PE state with an ACK counter instead of immediately deleting entries on eviction.
- Incoming requests during PE are handled rather than dropped.
- An upgrade from a known sharer in PE counts as an implicit ACK and gets ownership once all other ACKs arrive.
- Entry is deleted only after all ACKs are received with no pending requests.

---

## Question 4

### Part 1 -- Register Renaming Table

Free list is FIFO -- dequeue from top on rename, enqueue to bottom on commit.
fsd and bne have no destination register -- no allocation, no free.

| Instruction | Arch Dest | Phys Dest | Freed on Commit |
| --- | --- | --- | --- |
| fld f1, 0(x5) | f1 | P11 | P15 |
| fld f2, 0(x6) | f2 | P9 | P22 |
| fmul f3, f1, f4 | f3 | P17 | P3 |
| fadd f1, f2, f3 | f1 | P18 | P11 |
| fsd f1, 0(x6) | -- | -- | -- |
| addi x5, x5, 8 | x5 | P19 | P16 |
| addi x6, x6, 8 | x6 | P20 | P13 |
| addi x7, x7, -1 | x7 | P21 | P14 |
| bne x7, x0, loop | -- | -- | -- |
| fld f1, 0(x5) | f1 | P1 | P18 |
| fld f2, 0(x6) | f2 | P2 | P9 |
| fmul f3, f1, f4 (RED) | f3 | P4 | P17 |
| fadd f1, f2, f3 | f1 | P5 | P1 |
| fsd f1, 0(x6) | -- | -- | -- |
| addi x5, x5, 8 | x5 | P6 | P19 |
| addi x6, x6, 8 | x6 | P8 | P20 |
| addi x7, x7, -1 | x7 | P10 | P21 |
| bne x7, x0, loop | -- | -- | -- |

---

### Part 2 -- Precise Exception State

When the commit pointer reaches the red fmul (instr 12), instructions 1-11 have committed and their freed registers are in the free list. Instr 12-18 are squashed -- their allocated physical regs return to the bottom of the free list.

Map table -- reflects committed state after instrs 1-11:

| Arch Reg | Physical Reg |
| --- | --- |
| f1 | P1 |
| f2 | P2 |
| f3 | P17 |
| f4 | P7 |
| x5 | P19 |
| x6 | P20 |
| x7 | P21 |

Free list after squash (top = next to allocate):

Committed freed regs (in order): P15, P22, P3, P11, P16, P13, P14, P18, P9

Then squashed instrs 12-18 return allocations (appended to bottom in program order):
P4 (instr 12), P5 (instr 13), P6 (instr 15), P8 (instr 16), P10 (instr 17)

Final free list (top to bottom):
P15, P22, P3, P11, P16, P13, P14, P18, P9, P4, P5, P6, P8, P10

Sanity check: 7 map table entries + 14 free list entries = 21 total physical regs. Matches initial count (7 initial mappings + 14 initial free list). Correct.

---

## Question 5

### Favorite: Out-of-Order Execution

OoO execution is my favorite topic from this course. The hardware dynamically finds parallelism in code that was written sequentially, without the programmer or compiler doing anything special. Register renaming removes false dependencies, the ROB handles precise exceptions even though instructions complete out of order, and the instruction window lets the machine look ahead far enough to find independent work. All the pieces fit together nicely. It also makes the hardware really complex, which makes it interesting to think about.

### Least Favorite: Value Prediction

Value prediction is clever but feels like a lot of complexity for not much payoff. You need predictor tables, confidence counters, verification logic, and rollback for mispredictions, all to speculatively execute past a true data dependence. Those are rare compared to control dependences, and the benefit only shows up in narrow workloads like stride-1 loops. The hardware cost is hard to justify for most real designs. Interesting to read about but not something I would want to build or debug.
