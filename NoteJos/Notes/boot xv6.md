
## Build xv6 on Ubuntu

$ cd xv6-public
$ make
...
gcc -O -nostdinc -I. -c bootmain.c
gcc -nostdinc -I. -c bootasm.S
ld -m    elf_i386 -N -e start -Ttext 0x7C00 -o bootblock.o bootasm.o bootmain.o
objdump -S bootblock.o > bootblock.asm
objcopy -S -O binary -j .text bootblock.o bootblock
...
$ 

## Finding and breaking at an address

➜  xv6-public git:(master) ✗ nm kernel | grep _start
8010a48c D _binary_entryother_start
8010a460 D _binary_initcode_start
0010000c T _start


In this case, the address is 0010000c.

Run the kernel inside QEMU GDB, setting a breakpoint at _start (i.e., the address you just found).


本题目要求解释内核启动时栈中的数据。由于PC启动顺序是 BIOS -> boot loader -> kernel, 要想知道内核启动时栈中数据的来源, 需要知道前面 BIOS 和 boot loader 如何使用栈。因此, 下面先解答文中提出的早期BIOS 和 boot loader 启动的问题, 再来解释内核启动时栈中的数据。

1. 问题1：栈指针的初始值是什么？

在地址 0x7c00 处设置断点, 使用 c 命令运行至此, 然后使用 si 命令执行一步, 最后查看寄存器信息, 结果如下所示。可知栈指针的初始值为 0x6f20, 并且地址 0x6f20 存的数据为 0xf000d239.

0x0000fff0 in ?? ()
+ symbol-file obj/kern/kernel
(gdb)  b *0x7c00
Breakpoint 1 at 0x7c00
(gdb) c
Continuing.
[   0:7c00] => 0x7c00:	cli    

Breakpoint 1, 0x00007c00 in ?? ()
(gdb) si
[   0:7c01] => 0x7c01:	cld    
0x00007c01 in ?? ()
(gdb) si
[   0:7c02] => 0x7c02:	xor    %ax,%ax
0x00007c02 in ?? ()
(gdb) info reg
eax            0xaa55              43605
ecx            0x0                 0
edx            0x80                128
ebx            0x0                 0
esp            0x6f20              0x6f20
ebp            0x0                 0x0
esi            0x0                 0
edi            0x0                 0
eip            0x7c02              0x7c02
eflags         0x2                 [ ]
cs             0x0                 0
ss             0x0                 0
ds             0x0                 0
es             0x0                 0
fs             0x0                 0
gs             0x0                 0
(gdb)  x/xw 0x6f20
0x6f20:	0xf000d239

ps:

x/xw

x 按十六进制格式显示变量
w 表示四字节


2.  问题2：当调用 bootmain 时栈中数据是什么？

单步执行到 call bootmain 处，发现 esp 寄存器的值为 0x7c00, 也就是 boot block 的起始地址。当执行完 call 指令后, esp 寄存器的值变为 0x7bfc, call 指令的下一条指令的地址, 也是 bootmain 函数的返回地址。

  # Set up the stack pointer and call into C.
  movl    $start, %esp
    7c40:	bc 00 7c 00 00       	mov    $0x7c00,%esp
  call bootmain
    7c45:	e8 cf 00 00 00       	call   7d19 <bootmain>

=> 0x7c38:	mov    %eax,%es
0x00007c38 in ?? ()
(gdb) si
=> 0x7c3a:	mov    %eax,%fs
0x00007c3a in ?? ()
(gdb) si
=> 0x7c3c:	mov    %eax,%gs
0x00007c3c in ?? ()
(gdb) si
=> 0x7c3e:	mov    %eax,%ss
0x00007c3e in ?? ()
(gdb) si
=> 0x7c40:	mov    $0x7c00,%esp

3. 问题3：bootmain的第一条指令做了什么？

从 bootblock.asm 文件可以看到bootmain的第一条指令将ebp寄存器的值压栈。

    7d3b:	55                   	push   %ebp


4. 问题4：那个修改 eip 的值为 0x10000c 的 call 语句对栈做了什么？

修改 eip 的值为 0x10000c 的语句是 call *0x10018, 其中地址 0x10018 处存储的内容为 0x10000c, 所以此语句做的事情是：先将返回地址 0x7d8d 压栈, 然后跳到 0x10000c 的位置。


