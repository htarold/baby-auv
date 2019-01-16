
Wed 16 Jan 11:07:23 +08 2019

## Source directory

Development was done on a Linux host using avr-gcc and avr-libc.
The target was flashed with the Arduino bootloader, but the
Arduino environment was not used anywhere else.  The Makefile
reflects this environment.  It's probably possible to develop on
a Windows rather than Linux host.

The binaries prefixed with `test_` are single threaded executables.
Most of them are for testing various aspects of the AUV on the
bench, including calibration.

The binaries prefixed with `tasks_` are cooperatively multitasking
executables, generally used for tank and field testing.

The multitasking consists of a simple round robin scheduler.
There are 3 hard-coded tasks: after `main()` is called, it turns
into the idle task; there is also a foreground task and a
background task.  The 3 tasks nominally take 10ms to execute in
total, the idle task soaking up any excess time to make up the
10ms.  In practice this is often the case, but nothing bad
happens if a task takes too much timer.

A task calls `yield()` to pass execution to the next task.  A
task must NOT `return`.  Extra tasks cannot be spawned, and
tasks are not allowed to die.  `yield()` can be used to delay
for approximately 10ms.
