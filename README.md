### About

A collection of tiny demo programs for Windows.
The goal is to make each program less than 4 kilobytes.

All programs in this repo may be built for both x86 and x86-64 architectures.


### How do build

CMake and Visual Studio compiler are required.
Generate a project using CMake and build it.
Alternatively you can just open the CMake file provided in this repo using Visual Studio.
Target build type should be `MinSizeRel` to achieve minimal size.


### Anti-malware software warning

Since several non-trivial techniques for executable size minimization are used, some antivirus programs may mistake programs from this repo for malware.
Such tiny executable are too suspicious.
So, you can mark these executables as false-positive.
Alternatively you can stay wary and avoid running them.
