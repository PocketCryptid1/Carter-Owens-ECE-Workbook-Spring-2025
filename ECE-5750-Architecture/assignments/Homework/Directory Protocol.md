# Problem 5.9

## Part (a)

```
C3: R, M4 → C3: R, M2 → C7: W, M4 <-- 0xaaaa → C1: W, M4 <-- 0xbbbb
```

**T1: C3: R, M4** — M4@Node4 is DI
- Msgs: ReadReq 3→4, DataReply 4→3

**T2: C3: R, M2** — evicts M4 (silent, S state), M2@Node2 is DI
- Msgs: ReadReq 3→2, DataReply 2→3

**T3: C7: W, M4** — M4 is DS, sharer=Node3 (stale)
- Msgs: WriteReq 7→4, Inv 4→3, InvAck 3→4, DataReply 4→7

**T4: C1: W, M4** — M4 is DM, owner=Node7
- Msgs: WriteReq 1→4, Fetch/Inv 4→7, Writeback 7→4, DataReply 4→1

### Final State

| Node | Line 0 | Line 1 |
|------|--------|--------|
| 1 | M, M4, `0xbbbb` | I |
| 3 | S, M2, `0x0202` | I |

| Block | State | Vector | Memory |
|-------|-------|--------|--------|
| M2@2 | DS | 00001000 | `0x0202` |
| M4@4 | DM | 00000010 | stale (Node 1 has `0xbbbb`) |

---

## Part (b)

```
C3: R, M0 → C3: R, M2 → C6: W, M4 <-- 0xaaaa → C3: W, M4 <-- 0xbbbb
```

**T1: C3: R, M0** — DI
- Msgs: ReadReq 3→0, DataReply 0→3

**T2: C3: R, M2** — evicts M0 (silent), DI
- Msgs: ReadReq 3→2, DataReply 2→3

**T3: C6: W, M4** — DI
- Msgs: WriteReq 6→4, DataReply 4→6

**T4: C3: W, M4** — evicts M2 (silent), DM owner=Node6
- Msgs: WriteReq 3→4, Fetch/Inv 4→6, Writeback 6→4, DataReply 4→3

### Final State

| Node | Line 0 | Line 1 |
|------|--------|--------|
| 3 | M, M4, `0xbbbb` | I |

| Block | State | Vector | Memory |
|-------|-------|--------|--------|
| M0@0 | DS | 00001000 (stale) | `0x0000` |
| M2@2 | DS | 00001000 (stale) | `0x0202` |
| M4@4 | DM | 00001000 | stale (Node 3 has `0xbbbb`) |

---

## Part (c)

```
C0: R, M7 → C3: R, M4 → C6: W, M2 <-- 0xaaaa → C2: W, M2 <-- 0xbbbb
```

**T1: C0: R, M7** — DI
- Msgs: ReadReq 0→7, DataReply 7→0

**T2: C3: R, M4** — DI
- Msgs: ReadReq 3→4, DataReply 4→3

**T3: C6: W, M2** — DI
- Msgs: WriteReq 6→2, DataReply 2→6

**T4: C2: W, M2** — local directory, DM owner=Node6
- Msgs: Local req, Fetch/Inv 2→6, Writeback 6→2, Local reply

### Final State

| Node | Line 0 | Line 1 |
|------|--------|--------|
| 0 | I | S, M7, `0x0707` |
| 2 | M, M2, `0xbbbb` | I |
| 3 | S, M4, `0x0404` | I |

| Block | State | Vector | Memory |
|-------|-------|--------|--------|
| M2@2 | DM | 00000100 | stale (Node 2 has `0xbbbb`) |
| M4@4 | DS | 00001000 | `0x0404` |
| M7@7 | DS | 00000001 | `0x0707` |

---

---

# Problem 5.10 — Protocol Optimizations

## Part (a): Write miss to shared block

**Change:** Sharers send InvAcks directly to requester (not directory).

**Optimized msgs:**
1. Requester → Directory: WriteReq
2. Directory → Requester: DataReply + sharer count
3. Directory → Sharers: Invalidate
4. Sharers → **Requester**: InvAck

**Benefit:** Shorter critical path — directory doesn't wait for acks before sending data.

## Part (b): Read miss to modified block

**Change:** Owner forwards data directly to requester.

**Optimized msgs:**
1. Requester → Directory: ReadReq
2. Directory → Owner: Fetch+Forward (w/ requester ID)
3. Owner → **Requester**: DataReply
4. Owner → Directory: Notification (optional)

**Benefit:** Eliminates one data-transfer hop on critical path (Owner→Dir→Req becomes Owner→Req).

## Part (c): Read miss to shared block

**Change:** Closest sharer forwards data directly to requester.

**Optimized msgs:**
1. Requester → Directory: ReadReq
2. Directory → Closest Sharer: Forward (w/ requester ID)
3. Sharer → **Requester**: DataReply

**Benefit:** Potentially faster if sharer closer than home node; reduces home node memory bandwidth.
**Drawback:** 3 messages instead of 2; slower if no sharer is closer than home.

---

---

# Problem 5.11 — Relaxed Serialization

> Same-core serialized; different cores concurrent. Same initial conditions as 5.9.

---

## Part (a)

```
C1: W, M4 <-- 0xbbbb    C3: R, M4      C7: R, M2
                         C3: W, M4 <-- 0xaaaa
```

**Ordering:**
- C7: R, M2 → Node 2 — independent, no contention
- C1: W, M4 → Node 4 in 2 hops; C3: R, M4 → Node 4 in 3 hops → **C1 first**
- C3: W, M4 waits for C3: R, M4 (same core)

**C7: R, M2** (DI): ReadReq 7→2, DataReply 2→7

**C1: W, M4** (DI): WriteReq 1→4, DataReply 4→1

**C3: R, M4** (DM, owner=1): ReadReq 3→4, Fetch 4→1, Writeback 1→4, DataReply 4→3

**C3: W, M4** (DS, only sharer=self): UpgradeReq 3→4, UpgradeAck 4→3

### Final State

| Node | Line 0 | Line 1 |
|------|--------|--------|
| 3 | M, M4, `0xaaaa` | I |
| 7 | S, M2, `0x0202` | I |

| Block | State | Vector | Memory |
|-------|-------|--------|--------|
| M2@2 | DS | 10000000 | `0x0202` |
| M4@4 | DM | 00001000 | `0xbbbb` (stale — Node 3 has `0xaaaa`) |

---

## Part (b)

```
C3: R, M0        C6: W, M4 <-- 0xaaaa
C3: R, M2
C3: W, M4 <-- 0xbbbb
```

**Ordering:**
- C3: R, M0 → Node 0 and C6: W, M4 → Node 4 — parallel, no contention
- C3 transactions serialized: R M0 → R M2 → W M4
- C6: W, M4 completes before C3: W, M4 arrives

**C6: W, M4** (DI): WriteReq 6→4 (1 link), DataReply 4→6

**C3: R, M0** (DI): ReadReq 3→0 (2 links), DataReply 0→3

**C3: R, M2** (DI, evicts M0 silent): ReadReq 3→2 (1 link), DataReply 2→3

**C3: W, M4** (DM owner=6, evicts M2 silent): WriteReq 3→4, Fetch/Inv 4→6, Writeback 6→4, DataReply 4→3

### Final State

| Node | Line 0 | Line 1 |
|------|--------|--------|
| 3 | M, M4, `0xbbbb` | I |

| Block | State | Vector | Memory |
|-------|-------|--------|--------|
| M0@0 | DS | 00001000 (stale) | `0x0000` |
| M2@2 | DS | 00001000 (stale) | `0x0202` |
| M4@4 | DM | 00001000 | stale (Node 3 has `0xbbbb`) |

---

## Part (c)

```
C0: R, M7    C2: W, M2 <-- 0xbbbb    C3: R, M4    C6: W, M2 <-- 0xaaaa
```

**Ordering:**
- C0→Node7, C3→Node4 — independent
- C2: W, M2 is local (instant); C6: W, M2 → Node 2 in 1 hop → **C2 first**

**C0: R, M7** (DI): ReadReq 0→7 (3 links), DataReply 7→0

**C3: R, M4** (DI): ReadReq 3→4 (3 links), DataReply 4→3

**C2: W, M2** (DI, local): local req + local reply, no inter-node msgs

**C6: W, M2** (DM owner=2): WriteReq 6→2 (1 link), local fetch/inv, DataReply 2→6

### Final State

| Node | Line 0 | Line 1 |
|------|--------|--------|
| 0 | I | S, M7, `0x0707` |
| 3 | S, M4, `0x0404` | I |
| 6 | M, M2, `0xaaaa` | I |

| Block | State | Vector | Memory |
|-------|-------|--------|--------|
| M2@2 | DM | 01000000 | `0xbbbb` (stale — Node 6 has `0xaaaa`) |
| M4@4 | DS | 00001000 | `0x0404` |
| M7@7 | DS | 00000001 | `0x0707` |