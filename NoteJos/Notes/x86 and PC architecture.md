# 6.828 Lecture Notes: x86 and PC architecture

## Outline

PC architecture
x86 instruction set
gcc calling conventions
PC emulation

### PC architecture

1. A full PC has:
    1. an x86 CPU with registers, execution unit, and memory management
    2. CPU chip pins include address and data signals
    3. memory
    4. disk
    5. keyboard
    6. display
    7. other resources: BIOS ROM, clock, ...

2. We will start with the original 16-bit 8086 CPU (1978)

3. CPU runs instructions:
    for(;;){
    run next instruction
    }

4. Needs work space: registers
   1. four 16-bit data registers: AX, BX, CX, DX
   2. each in two 8-bit halves, e.g. AH and AL
   3. very fast, very few

5. More work space: memory
   1. CPU sends out address on address lines (wires, one bit per wire)
   2. Data comes back on data lines
   3. or data is written to data lines

6. Add address registers: pointers into memory
   1. SP - stack pointer
   2. BP - frame base pointer
   3. I - source index
   4. DI - destination index

7. Instructions are in memory too!
   1. IP - instruction pointer (PC on PDP-11, everything else)
   2. increment after running each instruction
   3. can be modified by CALL, RET, JMP, conditional jumps

8. Want conditional jumps
   1. FLAGS - various condition codes
      1. whether last arithmetic operation overflowed
      2. ... was positive/negative
      3. ... was [not] zero
      4. ... carry/borrow on add/subtract
      5. whether interrupts are enabled
      6. direction of data copy instructions

9. Still not interesting - need I/O to interact with outside world
    1. Original PC architecture: use dedicated I/O space
        1. Works same as memory accesses but set I/O signal
        2. Only 1024 I/O addresses
        3. Accessed with special instructions (IN, OUT)

### x86 Physical Memory Map

1. The physical address space mostly looks like ordinary RAM

2. Except some low-memory addresses actually refer to other things

3. Writes to VGA memory appear on the screen

4. Reset or power-on jumps to ROM at 0xfffffff0 (so must be ROM at top...)

+------------------+  <- 0xFFFFFFFF (4GB)
|      32-bit      |
|  memory mapped   |
|     devices      |
|                  |
/\/\/\/\/\/\/\/\/\/\

/\/\/\/\/\/\/\/\/\/\
|                  |
|      Unused      |
|                  |
+------------------+  <- depends on amount of RAM
|                  |
|                  |
| Extended Memory  |
|                  |
|                  |
+------------------+  <- 0x00100000 (1MB)
|     BIOS ROM     |
+------------------+  <- 0x000F0000 (960KB)
|  16-bit devices, |
|  expansion ROMs  |
+------------------+  <- 0x000C0000 (768KB)
|   VGA Display    |
+------------------+  <- 0x000A0000 (640KB)
|                  |
|    Low Memory    |
|                  |
+------------------+  <- 0x00000000

### x86 Instruction Set

1. Intel syntax: op dst, src (Intel manuals!)

2. AT&T (gcc/gas) syntax: op src, dst (labs, xv6)
   1. uses b, w, l suffix on instructions to specify size of operands

3. Operands are registers, constant, memory via register, memory via constant

4. Examples:

AT&T syntax "C"-ish equivalent
movl %eax, %edx edx = eax;  register mode
movl $0x123, %edx   edx = 0x123;    immediate
movl 0x123, %edx    edx = *(int32_t*)0x123; direct
movl (%ebx), %edx   edx = *(int32_t*)ebx;   indirect
movl 4(%ebx), %edx  edx = *(int32_t*)(ebx+4);   displaced

5. Instruction classes
   1. data movement: MOV, PUSH, POP, ...
   2. arithmetic: TEST, SHL, ADD, AND, ...
   3. i/o: IN, OUT, ...
   4. control: JMP, JZ, JNZ, CALL, RET
   5. string: REP MOVSB, ...
   6. system: IRET, INT

### gcc x86 calling conventions

1. x86 dictates that stack grows down:

    Example instruction What it does
    pushl %eax subl $4, %esp
                movl %eax, (%esp)

    popl %eax movl (%esp), %eax
                  addl $4, %esp

    call 0x12345  pushl %eip (*)
                  movl $0x12345, %eip (*)
    ret           popl %eip (*)


2. GCC dictates how the stack is used. Contract between caller and callee on x86:
   1. at entry to a function (i.e. just after call):
      1. %eip points at first instruction of function
      2. %esp+4 points at first argument
      3. %esp points at return address
   
   2. after ret instruction:
      1. %eip contains return address
      2. called function may have trashed arguments
      3. %eax (and %edx, if return type is 64-bit) contains return value (or trash if function is void)
      4. %eax, %edx (above), and %ecx may be trashed
      5. %ebp, %ebx, %esi, %edi must contain contents from time of call
   
   3. Functions can do anything that doesn't violate contract. By convention, GCC does more:
      1. each function has a stack frame marked by %ebp, %esp
   
		       +------------+   |
		       | arg 2      |   \
		       +------------+    >- previous function's stack frame
		       | arg 1      |   /
		       +------------+   |
		       | ret %eip   |   /
		       +============+   
		       | saved %ebp |   \
		%ebp-> +------------+   |
		       |            |   |
		       |   local    |   \
		       | variables, |    >- current function's stack frame
		       |    etc.    |   /
		       |            |   |
		       |            |   |
		%esp-> +------------+   /

      2. %esp can move to make stack frame bigger, smaller
      3. %ebp points at saved %ebp from previous function, chain to walk stack
      4. 


### PC emulation
