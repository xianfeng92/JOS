# 6.828 2018 Lecture 1: O/S overview

## 6.828 goals

1. Understand operating system design and implementation
2. Hands-on experience by building small O/S

## What is the purpose of an O/S?

1. Support applications
2. Abstract the hardware for convenience and portability
3. Multiplex the hardware among multiple applications
4. Isolate applications in order to contain bugs
5. Allow sharing among applications
6. Provide high performance

## What is the O/S design approach?

1. the small view: a h/w management library
2. the big view: physical machine -> abstract one w/ better properties

## Organization: layered picture

1. h/w: CPU, mem, disk, &c
2. kernel services
3. user applications: vi, gcc, &c

we care a lot about the interfaces and internal kernel structure

## What services does an O/S kernel typically provide?

1. processes
2. memory allocation
3. file contents
4. directories and file names
5. security
6. many others: users, IPC, network, time, terminals

## What does an O/S abstraction look like?

1. Applications see them only via system calls
2. Examples, from UNIX (e.g. Linux, OSX, FreeBSD):

            fd = open("out", 1);
            write(fd, "hello\n", 6);
            pid = fork();

## Why is O/S design/implementation hard/interesting?

1. the environment is unforgiving: quirky h/w, weak debugger
2. it must be efficient (thus low-level?)
3. powerful (thus many features?)
4. features interact: `fd = open(); ...; fork()`
5. behaviors interact: CPU priority vs memory allocator
6. open problems: security; performance; exploiting new hardware

## You'll be glad you learned about operating systems if you

1. want to work on the above problems
2. care about what's going on under the hood
3. have to build high-performance systems
4. need to diagnose bugs or security problems

## Introduction to system calls

1. 6.828 is largely about design and implementation of system call interface. let's look at how programs use that interface.
we'll focus on UNIX (Linux, Mac, POSIX, &c).

a simple example: what system calls does "ls" call?

Trace system calls:

    * On OSX: sudo dtruss /bin/ls
    * On Linux: strace /bin/ls

a more interesting program: the Unix shell.

     * it's the Unix command-line user interface
     * it's a good illustration of the UNIX system call API
     * some example commands:
        ls
        ls > junk
        ls | wc -l
        ls | wc -l > junk

     * the shell is also a programming/scripting language
     * Let's look at source for a simple shell, sh.c