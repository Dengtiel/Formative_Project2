## Repository layout

```
project2/
├── q1_pipeline/
│   └── pipeline.c        # fork + execvp + pipe: "ps aux | grep <keyword>"
├── q2_filecopy/
│   ├── copy_syscall.c    # low-level read()/write() file copy
│   └── copy_stdio.c      # buffered fread()/fwrite() file copy
├── q3_primes/
│   └── primes.c          # 16-thread prime counter, pthread_mutex_t
├── q4_search/
│   └── search.c          # multithreaded keyword search across files
└── README.md
```

---

## Question 1 — Process Communication (`q1_pipeline`)

**Build**
```bash
cd q1_pipeline
gcc -Wall -Wextra -o pipeline pipeline.c
```

**Run**
```bash
./pipeline root pipeline_output.txt
# ./pipeline <keyword> <output_file>
```

**Trace with strace** (traces the parent and follows forked children)
```bash
strace -f -e trace=fork,clone,execve,pipe,pipe2,dup2,read,write,open,openat,close \
  -o strace_q1.log ./pipeline root pipeline_output.txt
```

What to look for in `strace_q1.log` for your report:
- Two `clone()` calls (fork under the hood) — one per child.
- `pipe()` or `pipe2()` creating the fd pair before either fork.
- `dup2()` calls in each child remapping stdin/stdout to the pipe or file.
- `execve()` replacing each child's image with `ps` and `grep`.
- `read()`/`write()` calls moving data through the pipe from `ps` to `grep`.
- `openat()`/`close()` around the output file in child 2.

---

## Question 2 — Syscalls vs Standard I/O (`q2_filecopy`)

**Build**
```bash
cd q2_filecopy
gcc -Wall -Wextra -O2 -o copy_syscall copy_syscall.c
gcc -Wall -Wextra -O2 -o copy_stdio copy_stdio.c
```

**Create a 100MB+ test file**
```bash
dd if=/dev/urandom of=testfile_100mb.bin bs=1M count=100 status=none
```

**Run and compare timing** (each program also prints its own timing)
```bash
./copy_syscall testfile_100mb.bin out_syscall.bin
./copy_stdio   testfile_100mb.bin out_stdio.bin
```

**Trace and count syscalls**
```bash
strace -c -o strace_syscall_summary.log ./copy_syscall testfile_100mb.bin out_syscall.bin
strace -c -o strace_stdio_summary.log   ./copy_stdio   testfile_100mb.bin out_stdio.bin
```
`strace -c` prints a summary table with total call counts per syscall —
use it directly to fill the "count number of system calls" requirement.

Expected pattern for your analysis: both versions here use a 64KB buffer,
so raw counts are similar; if you shrink `BUF_SIZE` in `copy_stdio.c` you'll
still see similar syscall counts because stdio simply flushes at the same
buffer boundary — the real difference case is when you make stdio's
internal buffer smaller than the syscall version's read/write chunk size,
or vice versa. Discuss this trade-off explicitly in the report: **stdio
buffers in user space, so it can issue fewer, larger syscalls than a naive
byte-at-a-time syscall version**, at the cost of an extra memcpy.

---

## Question 3 — Multithreaded Prime Counter (`q3_primes`)

**Build**
```bash
cd q3_primes
gcc -Wall -Wextra -O2 -o primes primes.c -lpthread -lm
```

**Run**
```bash
./primes
```
Expected final line:
```
The synchronized total number of prime numbers between 1 and 200000 is 17984
```

**Trace thread creation**
```bash
strace -f -e trace=clone,futex -o strace_q3.log ./primes
```
16 `clone()` calls (one per thread) plus `futex()` calls around the mutex
lock/unlock are what to point to in your report as evidence of
synchronization.

Design note for the report: the mutex is only locked **once per thread**
(after it finishes its whole segment), not once per number — this avoids
lock contention while still guaranteeing a correct shared total.

---

## Question 4 — Concurrent Keyword Search (`q4_search`)

**Build**
```bash
cd q4_search
gcc -Wall -Wextra -O2 -o search search.c -lpthread
```

**Run**
```bash
./search keyword output.txt file1.txt file2.txt file3.txt <number_of_threads>
```
- If there are more files than `<number_of_threads>`, the program processes
  them in batches of that size (so "2 threads" with 5 files runs 3
  batches: 2, 2, 1).

**Performance testing required by the rubric** — run three times and
record the elapsed time each program prints:
```bash
./search keyword out_2t.txt file1.txt file2.txt file3.txt 2
nproc                                             # find core count
./search keyword out_avgcores.txt file1.txt file2.txt file3.txt $(nproc)
./search keyword out_maxthreads.txt file1.txt file2.txt file3.txt $(ls file*.txt | wc -l)
```

**Trace synchronization**
```bash
strace -f -e trace=clone,futex,openat,write -o strace_q4.log \
  ./search keyword output.txt file1.txt file2.txt file3.txt 2
```
Point to the `futex()` calls around the mutex as evidence the shared
output file is never corrupted by interleaved writes.

---
