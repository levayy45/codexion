# Codexion — Full Roadmap

Two parts: **(A)** everything you must know to defend this project, **(B)** the project structure, phase by phase, with every file and function explained.

---

# PART A — Knowledge Roadmap (what you must know)

Learn these in order. Each level builds on the previous one.

## Level 1 — Threads

- A **thread** is an execution flow inside a process. All threads share the same heap and globals (which is why globals are forbidden here — shared mutable state is the enemy). Each thread has its own **stack**.
- `pthread_create(&tid, NULL, routine, arg)` — starts a thread running `routine(arg)`. `arg` is a `void *`, which is how we pass `&sim->coders[i]` to each coder.
- `pthread_join(tid, NULL)` — blocks until that thread finishes. Without joining, `main` could free memory while threads still use it → segfault.
- Key mental model: after `pthread_create`, you have **no idea** in what order threads run. The OS scheduler decides. All correctness must come from synchronization, never from "it usually runs in this order".

## Level 2 — Race conditions and mutexes

- A **race condition** = two threads access the same data, at least one writes, and the result depends on timing. Example: two coders both read `held == 0` and both take the same dongle.
- A **mutex** (`pthread_mutex_t`) guarantees only one thread at a time is inside the region between `lock` and `unlock` (the **critical section**).
- Rule used everywhere in this project: **every shared field belongs to exactly one mutex**, and you never touch the field without holding it:
  - dongle fields (`held`, `ready_at`, `queue`) → that dongle's `mtx`
  - coder fields (`last_compile`, `compiles`) → that coder's `mtx`
  - `stopped` → `stop_mtx`; `seq` → `seq_mtx`; stdout → `print_mtx`
- `pthread_mutex_init` / `destroy` — every init must have a matching destroy (valgrind checks this).

## Level 3 — Condition variables

This is the hardest primitive; know it cold.

- Problem it solves: a thread must wait for a condition ("dongle free AND I'm first in queue AND cooldown over") without burning CPU in a spin loop.
- `pthread_cond_wait(&cond, &mtx)` atomically: unlocks `mtx`, sleeps, and re-locks `mtx` when woken. The atomicity matters — without it, a signal could arrive between your unlock and your sleep and be lost forever (**lost wakeup**).
- **Spurious wakeups**: POSIX allows a thread to wake with no signal at all. Therefore a wait is ALWAYS written as:
  ```c
  while (!predicate())
      pthread_cond_wait(&cond, &mtx);
  ```
  never `if`. The while re-checks the predicate after every wakeup.
- `pthread_cond_signal` wakes one waiter; `pthread_cond_broadcast` wakes all. We broadcast because we don't know which waiter is top of the heap.
- `pthread_cond_timedwait(&cond, &mtx, &abs_time)` — same, but also wakes at an **absolute** deadline (built with `clock_gettime(CLOCK_REALTIME)`). We use a 1 ms timeout so waiters can notice two things nobody signals: cooldown expiry (pure passage of time) and the stop flag.

## Level 4 — Deadlock (Coffman's conditions)

A deadlock needs ALL FOUR simultaneously:

1. **Mutual exclusion** — a dongle is held by one coder at a time. (Inherent to the problem.)
2. **Hold and wait** — a coder holds one dongle while waiting for the second. (True in our design.)
3. **No preemption** — you can't rip a dongle out of someone's hands. (True.)
4. **Circular wait** — coder 1 waits for coder 2's dongle, who waits for coder 3's, ... back to coder 1.

Break any one and deadlock is impossible. We break **#4** with **resource ordering**: every coder acquires the **lower-indexed** dongle first (`assign_dongles`). Proof sketch: in any wait cycle, some coder would have to hold a high dongle while waiting for a lower one — impossible, since everyone takes low before high. No cycle → no deadlock.

Classic failure you must be able to describe: everyone grabs their left dongle at t=0 → everyone holds one, waits for one, circle is closed, program hangs forever.

## Level 5 — Starvation and fairness

- **Starvation ≠ deadlock.** The system makes progress, but one particular coder never gets dongles and burns out. Deadlock-free code can still starve.
- Cause: raw mutex acquisition has no fairness — a fast coder can re-grab a dongle before a slow waiter is even scheduled.
- Our fix: a dongle is never taken by "whoever locks first"; every request enters a **queue** and only the queue's top may take it. That's the arbitration the subject calls the scheduler.

## Level 6 — Scheduling policies

- **FIFO** — serve requests in arrival order. Needs a global order of arrivals → the `seq` counter (a monotonically increasing ticket, protected by `seq_mtx`). Guarantees bounded waiting: at most (queue length ahead of you) turns.
- **EDF (Earliest Deadline First)** — serve the coder whose burnout is soonest: `deadline = last_compile_start + time_to_burnout`. This is a classic real-time scheduling policy (Liu & Layland): if any policy can meet all deadlines, EDF can.
- **Tie-breaker** — two deadlines can be equal (ms precision). Compare coder id second, so ordering is total and deterministic (the subject explicitly requires this).

## Level 7 — Binary min-heap

You wrote the priority queue by hand (subject requirement), so know it:

- Array-encoded complete binary tree: children of `i` are `2i+1`, `2i+2`; parent is `(i-1)/2`.
- **Invariant**: every node ≤ its children (by `heap_less`). Root = minimum = next to serve.
- **push** = append at end, **sift up**: swap with parent while smaller. O(log n).
- **pop** = move last element to root, **sift down**: swap with the smaller child while larger. O(log n).
- In practice each dongle is only ever wanted by its 2 neighbors, so the heap holds ≤2 requests — but the structure is general and required.

## Level 8 — Time handling

- `gettimeofday(&tv, NULL)` → seconds + microseconds since epoch. Convert: `tv_sec * 1000 + tv_usec / 1000` = milliseconds.
- Why **smart sleep** instead of one `usleep(duration * 1000)`: (1) `usleep` can oversleep, and its error accumulates; sleeping in 200 µs slices and re-checking the wall clock bounds the error. (2) A single long sleep can't be interrupted — a coder mid-`usleep(200000)` couldn't notice the simulation stopped. The slice loop checks `sim_stopped` every 200 µs.
- Timestamps in logs are **relative**: `now - sim->start`.

## Level 9 — The monitor pattern

- One dedicated thread whose only job is watching state: burnout (`now - last_compile > t_burnout`) and completion (`compiles >= must_compile` for everyone).
- Why coders can't self-detect burnout: a coder blocked waiting for a dongle isn't running its own code — it can't announce anything on time. The monitor is always running.
- Sampling every 500 µs → detection latency ≪ the 10 ms requirement.
- Shutdown broadcast: after setting `stopped`, the monitor broadcasts every dongle's cond so blocked coders wake immediately instead of waiting out their 1 ms timeout.

## Level 10 — Memory and shutdown discipline

- Symmetry: every `malloc` ↔ `free`, every `_init` ↔ `_destroy`, every `create` ↔ `join`.
- **Partial-init unwinding**: if dongle #3 fails to init, destroy exactly #0–#2, not all n (destroying an uninitialized mutex is undefined behavior). That's why `destroy_n_dongles(sim, count)` takes a count.
- Order at exit: join ALL threads first, only then destroy mutexes and free memory. Destroying a mutex a thread still uses = UB/crash.

## Level 11 — Verification tools (know what each proves)

- `-Wall -Wextra -Werror` — compile-time issues.
- **valgrind** `--leak-check=full` — leaks and invalid memory accesses (single-threaded-ish evidence).
- **ThreadSanitizer** (`-fsanitize=thread`) — data races: unsynchronized concurrent accesses. Passing both end conditions (completion AND burnout) matters because shutdown paths differ.
- **norminette** — Norm compliance.
- Log auditing — grep that every line matches the 5 allowed formats, and count 2 "has taken a dongle" per "is compiling".

---

# PART B — Project Structure Roadmap (phase by phase)

Build/read order. Each phase compiles on top of the previous.

```
codexion/
├── Makefile
├── README.md
├── includes/codexion.h        Phase 0
└── src/
    ├── parse.c                Phase 0
    ├── init.c                 Phase 1
    ├── cleanup.c              Phase 1
    ├── time.c                 Phase 2
    ├── state.c                Phase 2
    ├── heap.c                 Phase 2
    ├── dongle.c               Phase 3
    ├── dongle_utils.c         Phase 3
    ├── coder.c                Phase 4
    ├── monitor.c              Phase 4
    └── main.c                 Phase 5
```

## Phase 0 — Data model & input validation

**Goal:** define every structure, and reject any invalid input before a single thread exists.

### `includes/codexion.h`
- `t_req` — one queue entry: `key` (FIFO ticket or EDF deadline) + coder `id` (tie-breaker).
- `t_heap` — `arr` (malloc'd, capacity n) + `size`. The priority queue.
- `t_dongle` — `mtx` (owns all fields below), `cond` (waiting queue), `held` (0/1), `ready_at` (cooldown expiry timestamp), `queue` (the heap).
- `t_coder` — `id` (1..n), `compiles`, `last_compile`, its `thread`, its `mtx` (owns compiles/last_compile), `first`/`second` (dongle pointers, **already ordered** low→high), back-pointer `sim`.
- `t_sim` — all 8 parsed arguments, `start` timestamp, `stopped` flag, `seq` counter, the 3 global mutexes (`stop_mtx`, `print_mtx`, `seq_mtx`), the `dongles` and `coders` arrays, the `monitor` thread. This struct lives on `main`'s stack and replaces globals.

### `src/parse.c`
- `parse_pos(s)` — strict non-negative integer parser: optional `+`, digits only, running overflow check against `INT_MAX`, returns -1 on any violation. (Why not raw `atoi`: atoi accepts `12abc`, negatives, and silently overflows.)
- `parse_scheduler(sim, s)` — `strcmp` against exactly `"fifo"` / `"edf"`, sets `sim->edf`.
- `parse_args(sim, argc, argv)` — zeroes the whole struct (`memset` → every pointer NULL, safe cleanup even on early failure), requires `argc == 9`, parses all 7 numbers, enforces `n ≥ 1`, `must_compile ≥ 1`, all times ≥ 0.

**Checkpoint:** `./codexion` with bad args → usage on stderr, exit 1, nothing else happens.

## Phase 1 — Construction & destruction (symmetric pair)

**Goal:** allocate and initialize everything; be able to tear it all down from ANY failure point.

### `src/init.c`
- `init_dongle(d, n)` — mallocs the heap array, inits mutex then cond; on each failure, undoes exactly what succeeded and returns 0.
- `init_dongles(sim)` — mallocs the dongle array, zeroes it, loops `init_dongle`; on failure at index i calls `destroy_n_dongles(sim, i)` — only the i that succeeded.
- `assign_dongles(c)` — computes left = dongle `id-1`, right = dongle `id % n`, then stores them **sorted by address** into `first`/`second`. This one function is the entire deadlock prevention. Side effect: with n = 1, left == right, so `first == second` — the lone-coder marker.
- `init_coders(sim)` — mallocs coder array, sets `id`, `sim`, inits each coder's mutex, assigns dongles; same unwinding pattern.

### `src/cleanup.c`
- `destroy_n_dongles(sim, count)` — destroys mutex + cond and frees heap array for the first `count` dongles, frees the array, NULLs the pointer (guarded: returns if already NULL — safe to call twice).
- `destroy_n_coders(sim, count)` — same for coder mutexes.
- `destroy_sim_mutexes(sim)` — the 3 global mutexes.
- `destroy_sim(sim)` — full teardown = the three above with `count = n`.
- `join_threads(sim, count)` — joins the first `count` coder threads + the monitor. Takes a count so `start_threads` can join only what it actually created if creation fails halfway.

**Checkpoint:** init then immediately destroy → valgrind clean.

## Phase 2 — Primitives (no threads yet, pure tools)

**Goal:** the three toolboxes everything else uses: time, shared state, priority queue.

### `src/time.c`
- `now_ms()` — `gettimeofday` → epoch milliseconds.
- `elapsed_ms(sim)` — `now - sim->start`; every log timestamp.
- `smart_sleep(sim, duration)` — compute absolute `end`, then loop `usleep(200)` while not stopped and not past `end`. Interruptible, drift-bounded.

### `src/state.c`
- `sim_stopped(sim)` / `set_stop(sim)` — the only reads/writes of `stopped`, both under `stop_mtx`. The stop signal every loop in the program polls.
- `next_seq(sim)` — atomic ticket dispenser under `seq_mtx`; total arrival order for FIFO.
- `print_state(c, msg)` — under `print_mtx`: if not stopped, `printf("%ld %d %s\n", ...)`. The stop check inside the lock is what guarantees nothing prints after a burnout line.
- `print_burnout(sim, id)` — same lock, but no stop check: the burnout line is the one message that must print after stop.

### `src/heap.c`
- `heap_less(a, b)` — the total order: key first, id as tie-breaker.
- `heap_swap(a, b)` — swap two `t_req`.
- `heap_push(h, r)` — append + sift up.
- `heap_pop(h)` — last→root + sift down.
- `heap_top_id(h)` — id at root, or -1 if empty. The only question `can_take` asks.

**Checkpoint:** push {5,1}, {3,2}, {3,3}, {1,4} → pop order 4, 2, 3, 1.

## Phase 3 — Dongle protocol (the heart of the project)

**Goal:** fair, cooldown-aware, stop-aware acquisition and release.

### `src/dongle.c`
- `deadline_of(c)` — reads `last_compile` under the coder mutex, returns `last_compile + t_burnout`. The EDF key.
- `make_req(c)` — builds the request: key = deadline (edf) or next ticket (fifo), id = coder id.
- `can_take(c, d)` — the wait predicate, three conditions, all under the dongle mutex: not held, I am `heap_top_id`, `now >= ready_at` (cooldown over).
- `wait_turn(c, d)` — the `while (!stopped && !can_take)` loop around `pthread_cond_timedwait` with a 1 ms absolute timeout (built via `clock_gettime` + nanosecond carry handling). The timeout is what lets a waiter notice cooldown expiry and shutdown without any signal.
- `take_dongle(c, d)` — lock → push request → `wait_turn` → if stopped: unlock, return 0 (caller cleans up) → else pop myself, `held = 1`, unlock, log "has taken a dongle", return 1.

### `src/dongle_utils.c`
- `release_dongle(sim, d)` — lock → `held = 0`, `ready_at = now + cooldown` → **broadcast** (all waiters recheck; only the heap top passes) → unlock.
- `wake_all_dongles(sim)` — broadcast every dongle's cond under its mutex; called by the monitor at shutdown so nobody waits out a timeout.

**Checkpoint:** the cooldown test — release at t=100 with cooldown 500 → next "has taken" at t=601, exact.

## Phase 4 — The two thread routines

**Goal:** the coder lifecycle and the referee.

### `src/coder.c`
- `lone_coder(c)` — n = 1 path: take the single dongle, then idle in 200 µs slices until the monitor declares burnout. (One dongle can never become two.)
- `do_compile(c)` — take `first`; take `second` (if that fails on stop: **release first**, return 0 — no dongle may leak); under the coder mutex set `last_compile = now` and `compiles++` (deadline reset + progress count, atomically vs the monitor); log "is compiling"; `smart_sleep(t_compile)`; release both.
- `coder_routine(arg)` — cast arg to `t_coder *`; branch to `lone_coder` if `first == second`; else loop while not stopped: `do_compile` → "is debugging" + sleep → "is refactoring" + sleep. Returns NULL, thread ends, `join` collects it.

### `src/monitor.c`
- `coder_burned(c, sim)` — snapshot `last_compile` under the coder mutex, compare `now - last > t_burnout`.
- `all_compiled(sim)` — true iff every coder's `compiles >= must_compile` (each read under that coder's mutex).
- `check_coders(sim)` — scan for burnout (first hit: `set_stop` + `print_burnout`, return 1); else if `all_compiled`: `set_stop`, return 1; else 0.
- `monitor_routine(arg)` — loop: `check_coders`; on stop → `wake_all_dongles`, exit; else `usleep(500)`.

**Checkpoint:** burnout at deadline+1 ms, ten runs out of ten; feasible params → everyone reaches the quota, zero burnouts.

## Phase 5 — Orchestration

### `src/main.c`
- `init_sim_mutexes(sim)` — the 3 globals, with partial unwinding.
- `init_sim(sim)` — mutexes → dongles → coders; any failure unwinds everything before it.
- `start_threads(sim)` — stamp `sim->start = now_ms()` and every coder's `last_compile = start` (everyone's timer begins at t=0) **before** any thread exists; create the monitor; create the coders; if creation fails at i: `set_stop` + `join_threads(sim, i)` so already-running threads exit and are collected.
- `main` — the whole life: `parse_args` → `init_sim` → `start_threads` → `join_threads(n)` → `destroy_sim` → 0. Any failure: message on stderr, clean up whatever exists, exit 1.

**Execution flow of one full run:**
```
main
 ├─ parse_args            reject bad input
 ├─ init_sim              mutexes → dongles(+heaps) → coders(ordered dongles)
 ├─ start_threads         t=0 stamped → monitor → n coders
 │    coder i loop:  take low dongle → take high dongle → compile
 │                   (last_compile=now, compiles++) → release both
 │                   → debug → refactor → repeat
 │    monitor loop:  every 500µs scan burnout / completion
 │                   → set_stop → (burnout log) → broadcast all conds
 ├─ join_threads          wait for every thread to exit
 └─ destroy_sim           free heaps/arrays, destroy every mutex & cond
```

## Phase 6 — Verification protocol (run before every push)

```
make re                                      # -Wall -Wextra -Werror -pthread, no relink
norminette src includes                      # all OK
./codexion 5 2000 200 200 200 3 50 edf       # completes, 3 compiles each
./codexion 1 800 200 200 200 5 0 fifo        # burned out at ~800
./codexion 4 310 200 100 100 10 0 fifo       # burnout within 10ms of 310
valgrind --leak-check=full ./codexion 4 2000 100 50 50 2 30 edf
cc -Wall -Wextra -Werror -pthread -fsanitize=thread -Iincludes src/*.c -o tsan_bin
./tsan_bin 5 3000 100 50 50 3 20 edf         # 0 race warnings, both end paths
```

---

# Defense quick answers (the three likely recode targets)

1. **"Why doesn't it deadlock?"** — Resource ordering in `assign_dongles`: everyone acquires the lower-indexed dongle first, so a circular wait would need someone holding high while waiting for low — contradiction. Coffman condition #4 broken.
2. **"Why `while` around the wait, and why timedwait?"** — `while`: spurious wakeups + broadcast wakes losers who must go back to sleep. `timedwait`: cooldown expiry and the stop flag are events with no signaler; the 1 ms timeout turns them into observable rechecks.
3. **"Why the id tie-breaker in the heap?"** — ms-precision deadlines collide; without a second key the order between equal deadlines is unspecified → non-deterministic EDF. Id makes `heap_less` a total order, as the subject demands.

Likely recode asks: change the log format (touch `state.c` only), add a third scheduler like LIFO (touch `heap_less` / `make_req`), track total wait time per coder (add a field to `t_coder` + stamp in `take_dongle`). Notice each lands in exactly one file — that's the payoff of this structure.
