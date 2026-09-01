# Vendored SOEM patches

Applied to the `SOEM` submodule at CMake configure time (see the block above
`add_subdirectory(SOEM)` in the top-level `CMakeLists.txt`). Re-running
configure is a no-op — the patch is skipped if it is already applied — and a
patch that fails to apply is a hard configure error rather than a silently
unpatched build. `git` therefore has to be on `PATH` to configure.

`.gitattributes` marks `patches/*.patch` as `-text` so the patch files stay LF
on every platform. That matters on Windows: with `core.autocrlf=true` (git's
default there) the SOEM submodule is checked out with CRLF line endings, and
`git apply` reconciles an LF patch against that CRLF tree correctly, but only
as long as the patch itself was not *also* mangled on checkout.

To refresh a patch after bumping the submodule: apply it by hand, fix the
rejects, then regenerate with `git -C SOEM diff -- <file> > patches/<name>.patch`
and reset the submodule working tree.

## soem-mbxreceive-null-deref.patch

`ecx_mbxreceive()` (`src/ec_main.c`) fetches its mailbox buffer from the pool
**once, before** the read-retry loop:

```c
mbxin = ecx_getmbx(context);
do {
   wkc = ecx_FPRD(..., mbxro, mbxl, mbxin, EC_TIMEOUTRET);
   ...
   ecx_dropmbx(context, mbxin);   /* mailbox error response, or CoE Emergency */
   mbxin = NULL;
   wkc = 0;
} while ((wkc <= 0) && !expired);
```

When the mailbox holds a **CoE Emergency** or a **mailbox error response**, SOEM
handles it, returns the buffer to the pool, nulls the pointer and sets
`wkc = 0` — which is exactly the loop's repeat condition. The next pass reads
the following mailbox into `mbxin == NULL`, so `ecx_FPRD()` `memcpy`s `mbx_rl`
bytes to address 0 and the process dies with SIGSEGV. `mbxh` is left dangling
into the returned buffer as well.

Every `ecx_SDOread()` opens with a mailbox drain, so the first CoE diagnostic
read against a drive with an emergency queued is enough to hit it. In this tool
that is the **Novanta diagnostics** path in `EtherCATWorker::scanSlaves()`: a
scan with diagnostics enabled against faulted drives crashes the GUI outright.
It was diagnosed on the sibling `ethercat-alias-tui` (aarch64, robot
mk20000003) from two apport cores, both with the stack
`__memcpy_sve <- ecx_FPRD <- ecx_mbxreceive <- ecx_SDOread <- scanSlaves`,
`x0 = 0` and `x2 = 0x80` (= the drives' `mbx_rl`). This repo pins the same SOEM
commit (`b410bf6`) and calls the same API, so it has the same bug.

The patch re-acquires the buffer at the top of every pass instead, and treats an
exhausted pool as `wkc = 0` — the same contract as upstream
[PR #976](https://github.com/OpenEtherCATsociety/SOEM/pull/976), which guards
only the original pre-loop `ecx_getmbx()` and so does not cover this path.
Unfixed in upstream `master` as of 2026-08-29; drop this patch if it lands.
