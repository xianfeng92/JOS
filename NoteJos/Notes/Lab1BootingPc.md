# Lab1 MIT 6.828 Lab1: Booting a PC

## GDB

See the GDB manual for a full guide to GDB commands. Here are some particularly useful commands for 6.828, some of which don't typically come up outside of OS development.

Ctrl-c
Halt the machine and break in to GDB at the current instruction. If QEMU has multiple virtual CPUs, this halts all of them.

c (or continue)
Continue execution until the next breakpoint or Ctrl-c.

si (or stepi)
Execute one machine instruction.

b function or b file:line (or breakpoint)
Set a breakpoint at the given function or line.

b *addr (or breakpoint)
Set a breakpoint at the EIP addr.

set print pretty
Enable pretty-printing of arrays and structs.

info registers
Print the general purpose registers, eip, eflags, and the segment selectors. For a much more thorough dump of the machine register state, see QEMU's own info registers command.

x/Nx addr
Display a hex dump of N words starting at virtual address addr. If N is omitted, it defaults to 1. addr can be any expression.

x/Ni addr
Display the N assembly instructions starting at addr. Using $eip as addr will display the instructions at the current instruction pointer.

symbol-file file
(Lab 3+) Switch to symbol file file. When GDB attaches to QEMU, it has no notion of the process boundaries within the virtual machine, so we have to tell it which symbols to use. By default, we configure GDB to use the kernel symbol file, obj/kern/kernel. If the machine is running user code, say hello.c, you can switch to the hello symbol file using symbol-file obj/user/hello.
QEMU represents each virtual CPU as a thread in GDB, so you can use all of GDB's thread-related commands to view or manipulate QEMU's virtual CPUs.

thread n
GDB focuses on one thread (i.e., CPU) at a time. This command switches that focus to thread n, numbered from zero.

info threads
List all threads (i.e., CPUs), including their state (active or halted) and what function they're in.

## 实验

## 阅读汇编语言资料

1. PC Assembly Language Book 是一本学习 x86 汇编语言的好书，不过要注意此书的例子是为 NASM 汇编器而设计, 而我们课程使用的是 GNU 汇编器

2. NASM 汇编器使用 Intel 语法，而 GNU 汇编器使用 AT&T 语法

### AT&T语法 vs Intel语法

DJGPP 是基于 GCC 的, 因此它使用 AT&T/UNIT 语法, 这和 Intel 语法存在一些差异。下面将介绍其差异点:

1. 寄存器命名：AT&T 需要在寄存器名字前加 "%", 而 Intel 直呼其名。比如访问 eax 寄存器

            AT&T:  %eax
            Intel: eax

2. 源/目的书写顺序：AT&T 先写源再写目的， 而 Intel 先写目的再写源。比如将 eax 寄存器的值加载到 ebx 寄存器

            AT&T: mov %eax, %ebx
            Intel: mov ebx, eax

3. 常量/立即数格式：AT&T 需要在常量或立即数前加 "$", 而 Intel 直呼其名。比如将 C 语言变量 booga 的地址加载到 eax 寄存器

            AT&T:  movl $_booga, %eax
            Intel: mov eax, _booga

    将常量 0xd00d 的值加载到 ebx 寄存器：

            AT&T:  movl $0xd00d, %ebx
            Intel: mov ebx, d00dh

4. 操作符类型说明：AT&T 要求在指令后面带上 b、w 或 l 等后缀来说明目的寄存器的宽度, 而 Intel 无此要求

            AT&T:  movw %ax, %bx
            Intel: mov bx, ax

5. 内存访问：AT&T的内存访问语法与Intel的不一样

        AT&T:  immed32(basepointer, indexpointer, indexscale)
        Intel: [basepointer + indexpointer * indexscale + immed32]

## 使用 GDB 命令跟踪 BIOS 做了哪些事情

基本功能：设置 ss 和 esp 寄存器的值, 打开A20门（为了后向兼容老芯片）、进入保护模式（需要设置 cr0 寄存器的 PE 标志）

    [f000:fff0]    0xffff0: ljmp   $0xf000,$0xe05b
    [f000:e05b]    0xfe05b: cmpl   $0x0,%cs:0x6ac8
    [f000:e062]    0xfe062: jne    0xfd2e1
    [f000:e066]    0xfe066: xor    %dx,%dx
    [f000:e068]    0xfe068: mov    %dx,%ss
    [f000:e06a]    0xfe06a: mov    $0x7000,%esp
    [f000:e070]    0xfe070: mov    $0xf34c2,%edx
    [f000:e076]    0xfe076: jmp    0xfd15c
    [f000:d15c]    0xfd15c: mov    %eax,%ecx
    [f000:d15f]    0xfd15f: cli    
    [f000:d160]    0xfd160: cld    
    [f000:d161]    0xfd161: mov    $0x8f,%eax
    [f000:d167]    0xfd167: out    %al,$0x70
    [f000:d169]    0xfd169: in     $0x71,%al
    [f000:d16b]    0xfd16b: in     $0x92,%al
    [f000:d16d]    0xfd16d: or     $0x2,%al
    [f000:d16f]    0xfd16f: out    %al,$0x92
    [f000:d171]    0xfd171: lidtw  %cs:0x6ab8
    [f000:d177]    0xfd177: lgdtw  %cs:0x6a74
    [f000:d17d]    0xfd17d: mov    %cr0,%eax
    [f000:d180]    0xfd180: or     $0x1,%eax
    [f000:d184]    0xfd184: mov    %eax,%cr0
    [f000:d187]    0xfd187: ljmpl  $0x8,$0xfd18f

    1. 第一条指令：[f000:fff0] 0xffff0: ljmp $0xf000,$0xe05b

    CS（CodeSegment）和IP（Instruction Pointer）寄存器一起用于确定下一条指令的地址。计算公式： physical address = 16 * segment + offset.
    PC 开始运行时, CS = 0xf000，IP = 0xfff0 对应物理地址为 0xffff0. 第一条指令做了 jmp 操作，跳到物理地址为 0xfe05b 的位置。

    2. CLI：Clear Interupt 禁止中断发生。STL：Set Interupt 允许中断发生。CLI 和 STI 是用来屏蔽中断和恢复中断用的, 如设置栈基址 SS 和偏移地址 SP 时, 需要 CLI, 因为如果这两条指令被分开了, 那么
       很有可能 SS 被修改了, 但由于中断而代码跳去其它地方执行了, SP 还没来得及修改, 就有可能出错

    3. CLD: Clear Director。STD：Set Director。在字行块传送时使用的, 它们决定了块传送的方向。CLD 使得传送方向从低地址到高地址，而 STD则相反
   
    4. 汇编语言中, CPU 对外设的操作通过专门的端口读写指令来完成, 读端口用 IN 指令, 写端口用 OUT 指令
   
    5. LIDT: 加载中断描述符。LGDT：加载全局描述符

    6. 控制寄存器：控制寄存器（CR0～CR3）用于控制和确定处理器的操作模式以及当前执行任务的特性。CR0 中含有控制处理器操作模式和状态的系统控制标志
       CR0 的位 0 是 PE（Protection Enable）标志。当设置该位时即开启了保护模式；当复位时即进入实地址模式。这个标志仅开启段级保护, 而并没有启用分页机制。若要启用分页机制, 那么 PE 和 PG 标志都要置位。CR0 的位 31 是 PG（Paging，分页）标志。当设置该位时即开启了分页机制; 当复位时则禁止分页机制, 此时所有线性地址等同于物理地址。在开启这个标志之前必须已经或者同时开启 PE 标志。即若要启用
       分页机制， 那么 PE 和 PG 标志都要置位。

    7. 地址卷绕：用两个 16 位的寄存器左移相加来得到 20 位的内存地址这里还是有问题。那就是两个 16 位数相加所得的最大结果是超过 20 位的。例如段基址 0xffff 左移变成 0xffff0 和偏移量 0xffff 相加得到 0x10ffef 这个内存地址是“溢出”的，怎么办？这里 CPU 被设计出来一个“卷绕”机制, 当内存地址超过 20 位则绕回来。举个例子你拿 0x100001 来寻址, 我就拿你当作 0x00001 。你超出终点我就把你绕回起点。
    8. A20 gate：现代的 x86 计算机, 无论你是 32 位的还是 64 位的，在开机的那一刻 CPU 都是以模拟 16 位模式运行的, 地址卷绕机制也是有效的,所以无论你的电脑内存有多大, 开机的时候 CPU 的寻址能力只有 1MB, 就好像回到 8086 时代一样。那么什么时候才结束 CPU 的 16 位模式运行呢？这由你（操作系统）说了算, 现代的计算机都有个"开关"叫 A20 gate, 开机的时候 A20 gate 是关闭的, CPU 以 16 位模式运行, 当 A20 gate 打开的时候"卷绕"机制失效, 内存寻址突破 1MB 限制, 我们就可以切换到正常的模式下运行了。
   
    9. 分段式保护模式下的寻址
       1. "保护模式"实现的两种内存管理方式: 分段式和分页式
       2. 分段式简单来说就是将内存规划出不同的"片段"来分配给不同的程序（也包含操作系统自己）使用。分页式则是将内存规划成大小相同的"页", 再将这些页分配给各个程序使用。
       3. 在分段模式下, 内存里会有一个"表", 这个“表”里存放了每个内存"片段"的信息（如这个"片段"在内存中的地址,这个"片段"多长等）, 比如我们现在将内存分成 10 个片段, 则这时我们有一个"表", 这个"表"有 10 项分别存放着对应这 10 个内存片段的描述信息。我有个数据存放在第 5 个片段中, 在第 5 个片段的第 6 个位置上, 所以当我们想要读取这个数据的时候, 我们的数据段寄存器里存放的"段基址"是 5 这个数,代表要去第 5 个片段上找, 对应的这时候的"偏移量"就是 6 这样我们就可以顺利的找到我们想要的数据。
       4. 要想实现在分段式保护模式下成功的寻址, 操作系统需要做的就是在内存中建立这个"表", "表"里放好内存分段的描述信息, 然后把这个"表"在内存的什么位置, 以及这个"表"里有多少个内存分段的描述信息告诉 CPU。这个"表"有个学名叫 GDT 全局描述符表。
       5. 在分段式的保护模式下, 16 位的"段基址"不再表示内存的物理地址, 而是表示 GDT 表的下标, 用来根据"段基址"从 GDT 表中取得对应下标的"段描述符", 从"段描述符"中取得真实的内存物理地址后在配合"偏移量"来计算出最终的内存地址。
    
    10. 从给 x86 通电的一刻开始, CPU 执行的第一段指令是 BIOS 固化在 ROM 上的代码, 这个过程是硬件定死的规矩, 就是这样。而 BIOS 在硬件自检完成后（你会听到“滴”的一声）会根据你在 BIOS 里设置的启动顺序（硬盘、光驱、USB）读取每个引导设备的第一个扇区 512 字节的内容, 并判断这段内容的最后 2 字节是否为 0xAA55, 如果是说明这个设备是可引导的, 于是就将这 512 字节的内容放到内存的 0x7C00 位置, 然后告诉 CPU 去执行这个位置的指令。这个过程同样是硬件定死的规矩, 就是这样。

## 使用 GDB 命令跟踪 boot loader 做了哪些事情

分析 boot/boot.S 的代码

00007c00 <start>:
    7c00:   fa                   cli
    7c01:   fc                   cld
    7c02:   31 c0                xor    %eax,%eax
    7c04:   8e d8                mov    %eax,%ds
    7c06:   8e c0                mov    %eax,%es
    7c08:   8e d0                mov    %eax,%ss
00007c0a <seta20.1>:
    7c0a:   e4 64                in     $0x64,%al
    7c0c:   a8 02                test   $0x2,%al
    7c0e:   75 fa                jne    7c0a <seta20.1>
    7c10:   b0 d1                mov    $0xd1,%al
    7c12:   e6 64                out    %al,$0x64
00007c14 <seta20.2>:
    7c14:   e4 64                in     $0x64,%al
    7c16:   a8 02                test   $0x2,%al
    7c18:   75 fa                jne    7c14 <seta20.2>
    7c1a:   b0 df                mov    $0xdf,%al
    7c1c:   e6 60                out    %al,$0x60
    7c1e:   0f 01 16             lgdtl  (%esi)
    7c21:   64 7c 0f             fs jl  7c33 <protcseg+0x1>
    7c24:   20 c0                and    %al,%al
    7c26:   66 83 c8 01          or     $0x1,%ax
    7c2a:   0f 22 c0             mov    %eax,%cr0
    7c2d:   ea                   .byte 0xea
    7c2e:   32 7c 08 00          xor    0x0(%eax,%ecx,1),%bh
00007c32 <protcseg>:
    7c32:   66 b8 10 00          mov    $0x10,%ax
    7c36:   8e d8                mov    %eax,%ds
    7c38:   8e c0                mov    %eax,%es
    7c3a:   8e e0                mov    %eax,%fs
    7c3c:   8e e8                mov    %eax,%gs
    7c3e:   8e d0                mov    %eax,%ss
    7c40:   bc 00 7c 00 00       mov    $0x7c00,%esp
    7c45:   e8 cb 00 00 00       call   7d15 <bootmain>

1. 在地址 0x7c00 处设置断点, 这是 boot loader 第一条指令的位置

2. 使用 si 命令跟踪代码, 可见 boot.S 文件中主要做了以下事情：初始化段寄存器、打开 A20 门、从实模式跳到虚模式（需要设置 GDT 和 cr0 寄存器）, 最后调用 bootmain 函数

3. seta20.1 和 seta20.2 两段代码实现打开 A20 门的功能, 其中 seta20.1 是向键盘控制器的 0x64 端口发送 0x61 命令, 这个命令的意思是要向键盘控制器的 P2 写入数据; seta20.2 是向键盘控制器的 P2 端口写数据了。写数据的方法是把数据通过键盘控制器的 0x60 端口写进去。写入的数据是 0xdf, 因为 A20 gate 就包含在键盘控制器的 P2 端口中, 随着 0xdf 的写入, A20 gate 就被打开了。

4. test 对两个参数(目标，源)执行 AND 逻辑操作, 并根据结果设置标志寄存器

5. GDT是全局描述符表, GDTR 是全局描述符表寄存器。想要在"保护模式"下对内存进行寻址就先要有 GDT, GDT 表里每一项叫做"段描述符", 用来记录每个内存分段的一些属性信息, 每个段描述符占 8 字节。CPU 使用
   GDTR 寄存器来保存我们 GDT 在内存中的位置和 GDT 的长度。lgdt gdtdesc 将源操作数的值（存储在 gdtdesc 地址中）加载到全局描述符表寄存器中

6. x86 一共有 4 个控制寄存器, 分别为 CR0～CR3, 而控制进入"保护模式"的开关在 CR0 上, CR0上和保护模式有关的位是 PE（标识是否开启保护模式）和 PG（标识是否启用分页式）

7. .byte 在当前位置插入一个字节; .word 在当前位置插入一个字


### 回答问题

1. 处理器从哪里开始执行 32 位代码？是什么导致了 16 位代码到 32 位代码的切换？

处理器应该是从 boot.S 文件中的 .code32 伪指令开始执行 32 位代码。补充：ljmp 语句使得处理器从 real mode 切换到 protected mode, 地址长度从 16 位变为 32 位。

  ljmp    $PROT_MODE_CSEG, $protcseg
  .code32                     # Assemble for 32-bit mode
protcseg:
  movw    $PROT_MODE_DSEG, %ax    # Our data segment selector

2. boot loader 怎么知道为了从磁盘中读取整个内核的内容需要加载多少扇区？它从哪里获得这个信息？

ELF 文件头中包含有段数目、每个段的偏移和字节数。根据这些信息，boot loader 可以知道加载多少扇区

## 修改链接地址并观察 boot loader 运行情况

我们要在 boot/Makefrag 文件中修改它的链接地址，修改完成后运行  make clean, 然后通过 make 指令重新编译内核, 再找到那条指令看看会发生什么。

这道题希望我们能够去修改 boot loader 的链接地址, 在 Lab 1 中, 作者引入了两个概念: 一个是链接地址, 一个是加载地址。链接地址可以理解为通过编译器链接器处理形成的可执行程序中指令的地址, 即逻辑地址。加载地址则是可执行文件真正被装入内存
后运行的地址, 即物理地址。

## 为什么当 BIOS 进入 boot loader 时与当 boot loader 进入内核时， 这两个时刻在地址为0x00100000的内存中的内容不相同？

0x00100000 这个地址是内核加载到内存中的地址。当 BIOS 进入 boot loader 时， 还没将内核加载到这块内存， 其内容是随机的；而当 boot loader 进入内核时， 内核已经加载完成，其内容就是内核文件内容。因此这两个阶段对应的 0x00100000 地址的
内容是不相同的。

## 串口格式化打印

### 补全打印八进制整数的代码

case 'o':
    num = getuint(&ap, lflag);
    base = 8;
    goto number;

### 解释 printf.c 和 console.c 之间的接口关系

printf.c中的putch函数调用了console.c中的cputchar函数，具体调用关系：cprintf -> vcprintf -> putch -> cputchar

### 解释 console.c 的以下代码

1 if (crt_pos >= CRT_SIZE) {
2     int i;
3     memmove(crt_buf, crt_buf + CRT_COLS, (CRT_SIZE - CRT_COLS) * sizeof(uint16_t));
4     for (i = CRT_SIZE - CRT_COLS; i < CRT_SIZE; i++)
          crt_buf[i] = 0x0700 | ' ';
5     crt_pos -= CRT_COLS;
6 }

首先， CRT(cathode ray tube) 是阴极射线显示器。根据 console.h 文件中的定义， CRT_COLS 是显示器每行的字长（1个字占2字节），取值为80； CRT_ROWS 是显示器的行数取值为25; 而 #define CRT_SIZE (CRT_ROWS * CRT_COLS) 是显示器屏幕能够
容纳的字数, 即 2000。当 crt_pos 大于等于 CRT_SIZE 时, 说明显示器屏幕已写满, 因此将屏幕的内容上移一行。接下来, 将最后 1 行的内容用黑色的空格塞满。将空格字符、0x0700 进行或操作的目的是让空格的颜色为黑色。最后更新 crt_pos 的值。
总结：这段代码的作用是当屏幕写满内容时将其上移1行，并将最后一行用黑色空格塞满。

### 逐步跟踪以下代码并回答问题

   int x = 1, y = 3, z = 4;
   cprintf("x %d, y %x, z %d\n", x, y, z);

1. fmt 指向格式化字符串 "x %d, y %x, z %d\n" 的内存地址, ap 指向第一个要打印的参数的内存地址, 也就是 x 的地址

2. 列出每次调用 cons_putc, va_arg 和 vcprintf 的状态
   1. cprintf 首先调用 vcprintf, 调用时传入的第 1 个参数 fmt 的值为格式化字符串 "x %d, y %x, z %d\n" 的地址, 第 2 个参数 ap 指向 x 的地址
   2. vcprintf 调用 vprintfmt, vprintfmt 函数中多次调用 va_arg 和 putch, putch 调用 cputchar, 而 cputchar 调用 cons_putc, putch 的第一个参数最终会传到 cons_putc
   3. 第 1 次调用 cons_putc: printfmt.c 第 95 行, 参数为字符'x'
   4. 第 2 次调用 cons_putc: printfmt.c 第 95 行, 参数为字符' '
   5. 第 1 次调用 va_arg: printfmt.c 第75行, lflag=0，调用前 ap 指向 x, 调用后 ap 指向 y
   6. 第 3 次调用 cons_putc: printfmt.c 第 49 行, 参数为字符'1'. 此处传给 putch 的第一个参数的表达方式比较新奇、简洁: "0123456789abcdef"[num % base]。注意双引号及其内部实际上定义了一个数组, 其元素依次为 16 进制的 16 个字符
   7. 第 4 次调用 cons_putc: printfmt.c 第 95 行, 参数为字符','
   8. 第 5 次调用 cons_putc: printfmt.c 第 95 行, 参数为字符' '
   9. 第 6 次调用 cons_putc: printfmt.c 第 95 行, 参数为字符'y'
   10. 第 7 次调用 cons_putc: printfmt.c 第 95 行, 参数为字符' '
   11. 第 2 次调用 va_arg: printfmt.c 第 75 行, lflag=0, 调用前 ap 指向 y, 调用后 ap 指向 z
   12. 第 8 次调用 cons_putc: printfmt.c 第 49 行, 参数为字符 '3'
   13. 第 9 次调用 cons_putc: printfmt.c 第 95 行, 参数为字符','
   14. 第 10 次调用 cons_putc: printfmt.c 第 95 行, 参数为字符' '
   15. 第 11 次调用 cons_putc: printfmt.c 第 95 行, 参数为字符'z'
   16. 第 12 次调用 cons_putc: printfmt.c 第 95 行, 参数为字符' '
   17. 第 3 次调用 va_arg: printfmt.c 第 75 行, lflag=0, 调用前 ap 指向 z, 调用后 ap 指向 z 的地址加 4 的位置
   18. 第 13 次调用 cons_putc: printfmt.c 第 49 行, 参数为字符'4'
   19. 第 14 次调用 cons_putc: printfmt.c 第 95 行, 参数为字符'\n'

### 判断以下代码的输出

    unsigned int i = 0x00646c72;
    cprintf("H%x Wo%s", 57616, &i);

1. 57616 转换成 16 进制就是 0xe110.根据 ASCII 码, i 的 4 个字节从低到高依次为 'r', 'l', 'd', '\0'. 这里要求主机是小端序才能正常打印出 “World” 这个单词

### 当打印的参数数目小于格式化字符串中需要的参数个数时会怎样？

cprintf("x=%d y=%d", 3);

1. 打印出来的 y 的值应该是栈中存储 x 的位置后面 4 字节代表的值。因为当打印出 x 的值后, va_arg 函数的 ap 指针指向 x 的最后一个字节的下一个字节。因此, 不管调用 cprintf 传入几个参数, 在解析到 "%d" 时, va_arg 函数就会取当前指针指向的地址作为 int 型整数的指针返回

### 如果将 GCC 的调用约定改为参数从左到右压栈, 为支持参数数目可变需要怎样修改 cprintf 函数？

有两种方法。一种是程序员调用 cprintf 函数时按照从右到左的顺序来传递参数, 这种方法不符合我们的阅读习惯、可读性较差。第二种方法是在原接口的最后增加一个 int 型参数, 用来记录所有参数的总长度, 这样我们可以根据栈顶元素找到格式化字符串的位置。这种方法需要计算所有参数的总长度，也比较麻烦...

## 分析内核栈初始化

判断一下操作系统内核是从哪条指令开始初始化它的堆栈空间的, 以及这个堆栈坐落在内存的哪个地方? 内核是如何给它的堆栈保留一块内存空间的? 堆栈指针又是指向这块被保留的区域的哪一端的呢?

1. 首先需要判断操作系统内核是从哪条指令开始初始化它的堆栈空间的

在 entry.S 中我们可以看到它最后一条指令是要调用 i386_init() 子程序, 这个子程序位于 init.c 文件之中。在这个程序中已经开始对操作系统进行一些初始化工作, 并且进入 mointor 函数。可见到 i386_init 子程序时, 内核的堆栈应该已经设置好了。所以设置内核堆栈的指令就应该是 entry.S 中位于 call i386_init 指令之前的两条语句：

        # Clear the frame pointer register (EBP)
        # so that once we get into debugging C code,
        # stack backtraces will be terminated properly.
        movl    $0x0,%ebp   # nuke frame pointer

        # Set the stack pointer
        movl    $(bootstacktop),%esp

        # now to C code
        call    i386_init

这两条指令修改了 %ebp，%esp 两个寄存器的值, 而这两个寄存器的值是和堆栈息息相关的。

2. 这个堆栈坐落在内存的什么地方？

打开路径为 lab/obj/kernel.asm 的文件, 这是内核的反汇编文件, 其中的第 58 行指出了对 esp 的赋值:

f0100034:   bc 00 00 11 f0       mov    $0xf0110000,%esp

则内核栈加载到主存 0xf0110000 的位置。这个逻辑地址的物理地址映像是 0x00110000

查找 entry.s, 得到如下代码

relocated:

    # Clear the frame pointer register (EBP)
    # so that once we get into debugging C code,
    # stack backtraces will be terminated properly.
    movl    $0x0,%ebp           # nuke frame pointer

    # Set the stack pointer
    movl    $(bootstacktop),%esp

    # now to C code
    call    i386_init

    # Should never get here, but in case we do, just spin.
spin:   jmp spin
.data
###################################################################
boot stack
###################################################################
    .p2align    PGSHIFT     # force page alignment
    .globl      bootstack
bootstack:
    .space      KSTKSIZE
    .globl      bootstacktop
bootstacktop:

可以看到 line 9 也是将栈顶位置赋给 esp。下面是栈的声明。分配了一块空间大小 KSTKSIZE 给从 bootstack 地址开始的栈, 已经知道当前栈顶是0xf0110000. 按执行顺序看, 是先执行 bootstack 再执行bootstacktop, 则后者高, 虽然当前栈为空。则栈是向低地址生长的, 栈的最低位置为 0xf0110000-KSTKSIZE。

## 研究 test_backtrace 函数

// Test the stack backtrace function (lab 1 only)
void
test_backtrace(int x)
{
    cprintf("entering test_backtrace %d\n", x);
    if (x > 0)
        test_backtrace(x-1);
    else
        mon_backtrace(0, 0, 0);
    cprintf("leaving test_backtrace %d\n", x);
}

test_backtrace 函数对应的汇编代码

// Test the stack backtrace function (lab 1 only)
void
test_backtrace(int x)
{
    ...
}

### 观察test_backtrace函数调用栈

下面开始观察 test_backtrace 函数的调用栈。%esp 存储栈顶的位置, %ebp 存储调用者栈顶的位置, %eax 存储 x 的值, 这几个寄存器需要重点关注, 因此我使用 gdb 的 display 命令设置每次运行完成后自动打印它们的值, 此外我也设置了自动打印栈内被用到的那段内存的数据，以便清楚观察栈的变化情况。

f01000d1:   c7 04 24 05 00 00 00    movl   $0x5,(%esp)
f01000d8:   e8 63 ff ff ff          call   f0100040 <test\_backtrace>
f01000dd:   83 c4 10                add    $0x10,%esp

test_backtrace 函数的调用发生在 i386_init 函数中, 传入的参数 x=5.我们将从这里开始跟踪栈内数据的变化情况。各寄存器及栈内的数据如下所示。可见, 共有两个 4 字节的整数被压入栈:

1. 输入参数的值（也就是5）
2. call 指令的下一条指令的地址（也就是f01000dd）

%esp = 0xf010ffdc
%ebp = 0xf010fff8
// stack info
0xf010ffe0: 0x00000005  // 第1次调用时的输入参数：5
0xf010ffdc: 0xf01000dd  // 第1次调用时的返回地址

进入 test_backtrace 函数后, 涉及栈内数据修改的指令可以分为三部分:

1. 函数开头，将部分寄存器的值压栈，以便函数结束前可以恢复
2. 调用cprintf前，将输入参数压入栈
3. 在第2次调用test_backtrace前，将输入参数压入栈

// function start
f0100040:   55                   push   %ebp
f0100041:   89 e5                mov    %esp,%ebp
f0100043:   56                   push   %esi
f0100044:   53                   push   %ebx
// call cprintf
f0100053:   83 ec 08                sub    $0x8,%esp
f0100056:   56                      push   %esi
f0100057:   8d 83 18 07 ff ff       lea    -0xf8e8(%ebx),%eax
f010005d:   50                      push   %eax
f010005e:   e8 cf 09 00 00          call   f0100a32 <cprintf>
f0100063:   83 c4 10                add    $0x10,%esp
// call test_backtrace(x-1)
f0100095:   83 ec 0c                sub    $0xc,%esp
f0100098:   8d 46 ff                lea    -0x1(%esi),%eax
f010009b:   50                      push   %eax
f010009c:   e8 9f ff ff ff          call   f0100040 <test_backtrace>

### 进入test_backtrace(4)

在即将进入test_backtrace(4)前，栈内数据如下所示

%esp = 0xf010ffc0
%ebp = 0xf010ffd8
// stack info
0xf010ffe0: 0x00000005  // 第1次调用时的输入参数：5
0xf010ffdc: 0xf01000dd  // 第1次调用时的返回地址
0xf010ffd8: 0xf010fff8  // 第1次调用时寄存器%ebp的值
0xf010ffd4: 0x10094     // 第1次调用时寄存器%esi的值
0xf010ffd0: 0xf0111308  // 第1次调用时寄存器%ebx的值
0xf010ffcc: 0xf010004a  // 残留数据，不需关注
0xf010ffc8: 0x00000000  // 残留数据，不需关注
0xf010ffc4: 0x00000005  // 残留数据，不需关注
0xf010ffc0: 0x00000004  // 第2次调用时的输入参数

### 进入mon_backtrace(0, 0, 0)

在即将进入mon_backtrace(0, 0, 0)前，栈内数据如下所示

%esp = 0xf010ff20
%ebp = 0xf010ff38
// stack info
0xf010ffe0: 0x00000005  // 第1次调用时的输入参数：5
0xf010ffdc: 0xf01000dd  // 第1次调用时的返回地址
0xf010ffd8: 0xf010fff8  // 第1次调用开始时寄存器%ebp的值
0xf010ffd4: 0x10094     // 第1次调用开始时寄存器%esi的值
0xf010ffd0: 0xf0111308  // 第1次调用开始时寄存器%ebx的值
0xf010ffcc: 0xf010004a  // 预留空间，不需关注
0xf010ffc8: 0x00000000  // 预留空间，不需关注
0xf010ffc4: 0x00000005  // 预留空间，不需关注
0xf010ffc0: 0x00000004  // 第2次调用时的输入参数：4
0xf010ffbc: 0xf01000a1  // 第2次调用时的返回地址
0xf010ffb8: 0xf010ffd8  // 第2次调用开始时寄存器%ebp的值
0xf010ffb4: 0x00000005  // 第2次调用开始时寄存器%esi的值
0xf010ffb0: 0xf0111308  // 第2次调用开始时寄存器%ebx的值
0xf010ffac: 0xf010004a  // 预留空间，不需关注
0xf010ffa8: 0x00000000  // 预留空间，不需关注
0xf010ffa4: 0x00000004  // 预留空间，不需关注
0xf010ffa0: 0x00000003  // 第3次调用时的输入参数：3
0xf010ff9c: 0xf01000a1  // 第3次调用时的返回地址
0xf010ff98: 0xf010ffb8  // 第3次调用开始时寄存器%ebp的值
0xf010ff94: 0x00000004  // 第3次调用开始时寄存器%esi的值
0xf010ff90: 0xf0111308  // 第3次调用开始时寄存器%ebx的值
0xf010ff8c: 0xf010004a  // 预留空间，不需关注
0xf010ff88: 0xf010ffb8  // 预留空间，不需关注
0xf010ff84: 0x00000003  // 预留空间，不需关注
0xf010ff80: 0x00000002  // 第4次调用时的输入参数：2
0xf010ff7c: 0xf01000a1  // 第4次调用时的返回地址
0xf010ff78: 0xf010ff98  // 第4次调用开始时寄存器%ebp的值
0xf010ff74: 0x00000003  // 第4次调用开始时寄存器%esi的值
0xf010ff70: 0xf0111308  // 第4次调用开始时寄存器%ebx的值
0xf010ff6c: 0xf010004a  // 预留空间，不需关注
0xf010ff68: 0xf010ff98  // 预留空间，不需关注
0xf010ff64: 0x00000002  // 预留空间，不需关注
0xf010ff60: 0x00000001  // 第5次调用时的输入参数：1
0xf010ff5c: 0xf01000a1  // 第5次调用时的返回地址
0xf010ff58: 0xf010ff78  // 第5次调用开始时寄存器%ebp的值
0xf010ff54: 0x00000002  // 第5次调用开始时寄存器%esi的值
0xf010ff50: 0xf0111308  // 第5次调用开始时寄存器%ebx的值
0xf010ff4c: 0xf010004a  // 预留空间，不需关注
0xf010ff48: 0xf010ff78  // 预留空间，不需关注
0xf010ff44: 0x00000001  // 预留空间，不需关注
0xf010ff40: 0x00000000  // 第6次调用时的输入参数：0
0xf010ff3c: 0xf01000a1  // 第6次调用时的返回地址
0xf010ff38: 0xf010ff58  // 第6次调用开始时寄存器%ebp的值
0xf010ff34: 0x00000001  // 第6次调用开始时寄存器%esi的值
0xf010ff30: 0xf0111308  // 第6次调用开始时寄存器%ebx的值
0xf010ff2c: 0xf010004a  // 预留空间，不需关注
0xf010ff28: 0x00000000  // 第7次调用时的第1个输入参数：0
0xf010ff24: 0x00000000  // 第7次调用时的第2个输入参数：0
0xf010ff20: 0x00000000  // 第7次调用时的第3个输入参数：0

### 退出mon_backtrace(0, 0, 0)

通过 add $0x10, %esp 语句，将输入参数及预留的4字节从栈中清除。此时 %esp = 0xf010ff30, %ebp = 0xf010ff38.

### 退出test_backtrace(0)

连续 3 个 pop 语句将 ebx, esi 和 ebp 寄存器依次出栈, 然后通过 ret 语句返回。其他1~5的退出过程类似，不再赘述。

### 实现 mon_backtrace 函数

要求打印调用栈的信息, 包括 ebp 和 eip 寄存器的值、输入参数的值等

1. 按照提示，我们首先可以调用 read_ebp 函数来获取当前 ebp 寄存器的值。ebp 寄存器的值实际上是一个指针，指向当前函数的栈帧的底部（而 esp 寄存器指向当前函数的栈顶）。我们可以把整个调用栈看做一个数组，其中每个元素均为 4 字节的整数，并以 ebp 指针的值为数组起始地址，那么 ebp[1] 存储的就是函数返回地址，也就是题目中要求的 eip 的值，ebp[2] 以后存储的是输入参数的值。由于题目要求打印5个输入参数，因此需要获取ebp[2]～ebp[6]的值。这样第一条栈信息便可打印出来

2. 那么怎么打印下一条栈信息呢？ 还得从 ebp 入手。当前 ebp 指针存储的恰好是调用者的 ebp 寄存器的值，因此当前 ebp 指针又可以看做是一个链表头，我们通过链表头就可以遍历整个链表。举个例子：假设有A、B、C三个函数，A 调用 B，B 调用 C，每个函数都对应有一个栈帧，栈帧的底部地址均存储在当时的ebp寄存器中，不妨记为 a_ebp, b_ebp 和 c_ebp，那么将有：a_ebp = (uint32_t *)*b_ebp 和 b_ebp = (uint32_t *)*c_ebp。

3. 还有一个问题：怎么知道遍历何时结束呢？题目中提示可以参考 kern/entry.S, 于是我打开此文件，果然找打答案：内核初始化时会将 ebp 设置为 0, 因此当我们检查到ebp为0后就应该结束了

### backtrace 函数增加打印文件名、函数名及行号等信息

