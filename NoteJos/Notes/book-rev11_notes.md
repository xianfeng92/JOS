

## Kernel organization

### a monolithic kernel

the entire operating system resides in the kernel, so that the implementations of all system calls run in kernel mode. This organization is called `a monolithic kernel`.

1. A downside of the monolithic organization

`the interfaces between different parts of the operating system are often complex`, and therefore it is easy for an operating system developer to make a mistake. In a monolithic kernel, a mistake is fatal, because an error in kernel mode will often result in the kernel to fail. If the kernel fails, the computer stops working, and thus all applications fail too. The computer must reboot to start again.


### microkernel

To reduce the risk of mistakes in the kernel,OS designers can minimize the amount of operating system code that runs in kernel mode, and execute the bulk of the operating system in user mode. This kernel organization is called a microkernel.

In a microkernel, the kernel interface consists of a few low-level functions for starting applications, sending messages, accessing device hardware, etc. This organization allows the kernel to be relatively simple, `as most of the operating system resides in user-level servers`.


## Process overview

The unit of isolation in xv6 (as in other Unix operating systems) is a process. 

## the first address space


