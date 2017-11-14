Threadripper SpinLock tester
============================

General information
-------------------

This project demonstrates difference in behavior of pthread and own spin
lock implementation. Namely, the difference is: pthread's implementation
causes the whole system to hang, while own implementation works much
better.

This is only visible when running Windows 10 on AMD Ryzen Threadripper
1950X, and it is NOT visible when running Windows 10 on Intel Xeon
E5-2699 v4 CPU.

How to see the issue
--------------------

This repository contains MSVC 2017 project which contains very simplistic
code: it creates thread pool, each worker thread increases value of a global
variable from inside the spin-lock section.

There is one crucial define  "USE_PTHREADS" which is to be found in main.c.
When this symbols is defined, spin lock implementation will come from
pthread library. When it's not defined, own implementation will be used.

Building release version of the project and running it from the console will
show the following results:
 - When USE_PTHREADS is defined, the whole system starts to hang.
 - When USE_PTHREADS is not defined, everything works fine.

Expected result
---------------

One would really expect both implementations to behave same exact way,
otherwise it means that binary compatibility on a new Threadripper CPU is
broken, while latest CPUs from Intel are not affected by this issue.

There is clearly something to be dig into specifics of atomics and/or memory
barriers implementations of new AMD CPUs.
