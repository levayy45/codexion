*This project has been created as part of the 42 curriculum by tzidini.*

# Codexion

## Description

Codexion is a concurrency simulation inspired by the dining philosophers
problem. `number_of_coders` coders sit in a circle around a shared Quantum
Compiler. Between each pair of coders lies one USB dongle. To compile, a coder
must hold both adjacent dongles; afterwards they debug, then refactor, then
try to compile again. A coder who does not start a new compile within
`time_to_burnout` milliseconds of their last compile start burns out, and the
simulation stops.

The project adds two twists over the classic problem:

- **Dongle cooldown**: after being released, a dongle is unavailable for
  `dongle_cooldown` milliseconds.
- **Pluggable arbitration**: when several coders want the same dongle, a
  per-dongle priority queue (a hand-written binary min-heap) decides who gets
  it, using either **FIFO** (arrival order, via a global monotonic sequence
  number) or **EDF** (earliest burnout deadline first, deadline =
  `last_compile_start + time_to_burnout`, ties broken deterministically by
  coder id).

Each coder is a POSIX thread. A separate monitor thread detects burnout
(within 10 ms) and the "everyone compiled enough" end condition. No global
variables are used; all state lives in a `t_sim` structure passed to threads.

## Instructions

Compilation (requires `cc` and pthreads):

```
make
```

Usage:

```
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
           time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

- All 8 arguments are mandatory. Times are in milliseconds.
- `scheduler` must be exactly `fifo` or `edf`.
- Invalid input (negative numbers, non-integers, overflow, unknown scheduler)
  is rejected with an error message and exit code 1.

Examples:

```
./codexion 5 2000 200 200 200 3 50 edf     # completes: 3 compiles each
./codexion 1 800 200 200 200 5 0 fifo      # lone coder burns out at ~800
./codexion 4 310 200 100 100 10 0 fifo     # infeasible: someone burns out
```

Other rules: `make clean` removes objects, `make fclean` also removes the
binary, `make re` rebuilds from scratch. The Makefile never relinks
needlessly.

## Blocking cases handled

- **Deadlock prevention (Coffman's conditions)**: circular wait is broken by
  resource ordering. Each coder always requests the lower-indexed of its two
  dongles first (`assign_dongles` in `src/init.c`). Since every thread
  acquires dongles in strictly increasing index order, a cycle in the wait
  graph is impossible, so the four Coffman conditions can never hold
  simultaneously.
- **Starvation prevention**: dongles are not grabbed on a first-come race.
  Every request enters the dongle's priority queue, and only the coder at the
  top of the heap may take it. FIFO guarantees bounded waiting by arrival
  order; EDF always serves the coder closest to burnout, preserving liveness
  whenever the parameters are feasible.
- **Cooldown handling**: `release_dongle` stamps `ready_at = now + cooldown`.
  `can_take` refuses acquisition before `ready_at`; waiters use
  `pthread_cond_timedwait` with a 1 ms timeout so they re-check the clock
  without busy-waiting and without missing wakeups.
- **Precise burnout detection**: the monitor thread samples every 500 µs and
  compares `now - last_compile_start` against `time_to_burnout`, so the
  burnout log appears within 1–2 ms of the real deadline (requirement:
  10 ms).
- **Log serialization**: every `printf` happens under a dedicated print
  mutex, so two messages can never interleave on one line, and no state
  message is printed after the stop flag is raised (the burnout line is the
  final one).
- **Single coder**: with one coder there is a single dongle; the coder takes
  it, can never obtain a second, and burns out at `time_to_burnout`.
- **Clean shutdown**: on stop, the monitor broadcasts every dongle condition
  variable; blocked coders wake, abandon their request, release any held
  dongle, and exit. `main` joins every thread, then destroys all mutexes,
  condition variables, and heap allocations (valgrind: 0 leaks, 0 errors;
  ThreadSanitizer: 0 races).

## Thread synchronization mechanisms

- **`pthread_mutex_t` per dongle** protects the dongle's state (`held`,
  `ready_at`, request heap). All reads/writes of that state happen with the
  mutex held, so two coders can never take the same dongle simultaneously.
- **`pthread_cond_t` per dongle** implements the waiting queue.
  `take_dongle` pushes a request into the heap, then loops on
  `pthread_cond_timedwait` until it is at the top of the heap, the dongle is
  free, and the cooldown has expired (checked in `can_take`, always under the
  mutex — the classic "recheck the predicate in a while loop" pattern, which
  also makes spurious wakeups harmless). `release_dongle` broadcasts to wake
  waiters. The timed wait doubles as a custom event mechanism: it lets
  waiters observe cooldown expiry and the global stop flag even when nobody
  signals.
- **`pthread_mutex_t` per coder** protects `last_compile` and `compiles`.
  These fields are written by the coder thread and read by the monitor (for
  burnout/end detection) and by `take_dongle` (for EDF deadlines); the mutex
  makes this cross-thread communication race-free.
- **Stop mutex + flag** (`sim_stopped` / `set_stop`) is the thread-safe
  channel from the monitor to the coders: every loop, sleep slice, and wait
  predicate checks it, so the whole simulation stops promptly and safely.
- **Print mutex** serializes all output.
- **Sequence mutex** makes the FIFO ticket counter atomic, giving a total
  arrival order across all dongles.

Example of a prevented race: without the coder mutex, the monitor could read
`last_compile` while the coder writes it at compile start and falsely declare
a burnout; with it, the monitor always sees either the old or the new value
atomically. Verified with `-fsanitize=thread` on both end conditions.

## Resources

- `man pthread_create`, `pthread_mutex_lock`, `pthread_cond_timedwait`,
  `gettimeofday`, `clock_gettime`
- E. W. Dijkstra — the dining philosophers problem (resource hierarchy
  solution)
- Coffman, Elphick, Shoshani — *System Deadlocks* (the four deadlock
  conditions)
- Liu & Layland — *Scheduling Algorithms for Multiprogramming in a Hard
  Real-Time Environment* (EDF scheduling)
- Butenhof — *Programming with POSIX Threads* (condition variable predicate
  pattern)

**AI usage**: AI (Anthropic's Claude) was used to draft the initial project
skeleton (argument parsing, thread/mutex boilerplate, the binary heap, and
this README's structure) and to cross-check the design against the subject's
requirements. All concurrency decisions (resource ordering for deadlock
freedom, per-dongle heap arbitration, timed-wait cooldown handling, monitor
timing) were reviewed, tested, and are fully understood by the author; the
code was validated manually with valgrind, ThreadSanitizer, norminette, and
stress tests (up to 200 coders) before submission.
