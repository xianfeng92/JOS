#include <inc/x86.h>
#include <inc/elf.h>

/**********************************************************************
 * This a dirt simple boot loader, whose sole job is to boot
 * an ELF kernel image from the first IDE hard disk.
 *
 * DISK LAYOUT
 *  * This program(boot.S and main.c) is the bootloader.  It should
 *    be stored in the first sector of the disk.
 *
 *  * The 2nd sector onward holds the kernel image.
 *
 *  * The kernel image must be in ELF format.
 *
 * BOOT UP STEPS
 *  * when the CPU boots it loads the BIOS into memory and executes it
 *
 *  * the BIOS intializes devices, sets of the interrupt routines, and
 *    reads the first sector of the boot device(e.g., hard-drive)
 *    into memory and jumps to it.
 *
 *  * Assuming this boot loader is stored in the first sector of the
 *    hard-drive, this code takes over...
 *
 *  * control starts in boot.S -- which sets up protected mode,
 *    and a stack so C code then run, then calls bootmain()
 *
 *  * bootmain() in this file takes over, reads in the kernel and jumps to it.
 **********************************************************************/

#define SECTSIZE	512
#define ELFHDR		((struct Elf *) 0x10000) // scratch space

void readsect(void*, uint32_t);
void readseg(uint32_t, uint32_t, uint32_t);

void
bootmain(void)
{
	struct Proghdr *ph, *eph;

	// read 1st page off disk
	// 把内核的第一个页(4MB = 4096 = SECTSIZE*8 = 512*8)的内容读取的内存地址 ELFHDR(0x10000)处。其实完成这些后相当于把操作系统映像文件的 elf 头部读取出来放入内存中
	readseg((uint32_t) ELFHDR, SECTSIZE*8, 0);

	// is this a valid ELF?
	// 读取完这个内核的 elf 头部信息后, 需要对这个 elf 头部信息进行验证, 并且也需要通过它获取一些重要信息。
	// elf 头部信息的 magic 字段是整个头部信息的开端。并且如果这个文件是格式是ELF格式的话，文件的 elf->magic 域应该是 ELF_MAGIC 的，所以这条语句就是判断这个输入文件是否是合法
	// 的 elf 可执行文件
	if (ELFHDR->e_magic != ELF_MAGIC)
		goto bad;

	// load each program segment (ignores ph flags)
	// 我们知道头部中一定包含 Program Header Table。这个表格存放着程序中所有段的信息, 通过这个表我们才能找到要执行的代码段、数据段等等
	// 首先 elf 是表头起址, 而 phoff 字段代表 Program Header Table 距离表头的偏移量
	ph = (struct Proghdr *) ((uint8_t *) ELFHDR + ELFHDR->e_phoff);

	// 由于 phnum 中存放的是 Program Header Table 表中表项的个数, 即段的个数。所以这步操作是将 eph 指向 Program Header Table 表末尾
	eph = ph + ELFHDR->e_phnum;
	// 这个 for 循环就是在加载所有的段到内存中。ph->paddr 根据参考文献中的说法指的是这个段在内存中的物理地址。ph->off 字段指的是这一段的开头相对于这个 elf 文件的开头的偏移量
	// ph->filesz 字段指的是这个段在 elf 文件中的大小。ph->memsz 则指的是这个段被实际装入内存后的大小。通常来说 memsz 一定大于等于 filesz, 因为段在文件中时许多未定义的变量并没有
	// 分配空间给它们. 所以这个循环就是在把操作系统内核的各个段从外存读入内存中
	for (; ph < eph; ph++)
		// p_pa is the load address of this segment (as well
		// as the physical address)
		readseg(ph->p_pa, ph->p_memsz, ph->p_offset);

	// call the entry point from the ELF header
	// note: does not return!
	// e_entry 字段指向的是这个文件的执行入口地址。所以这里相当于开始运行这个文件。也就是内核文件。 自此就把控制权从 boot loader 转交给了操作系统的内核
	((void (*)(void)) (ELFHDR->e_entry))();

bad:
	outw(0x8A00, 0x8A00);
	outw(0x8A00, 0x8E00);
	while (1)
		/* do nothing */;
}

// Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
// Might copy more than asked
void
readseg(uint32_t pa, uint32_t count, uint32_t offset)
{
	uint32_t end_pa;

	end_pa = pa + count;

	// round down to sector boundary
	pa &= ~(SECTSIZE - 1);

	// translate from bytes to sectors, and kernel starts at sector 1
	offset = (offset / SECTSIZE) + 1;

	// If this is too slow, we could read lots of sectors at a time.
	// We'd write more to memory than asked, but it doesn't matter --
	// we load in increasing order.
	while (pa < end_pa) {
		// Since we haven't enabled paging yet and we're using
		// an identity segment mapping (see boot.S), we can
		// use physical addresses directly.  This won't be the
		// case once JOS enables the MMU.
		readsect((uint8_t*) pa, offset);
		pa += SECTSIZE;
		offset++;
	}
}

void
waitdisk(void)
{
	// wait for disk reaady
	while ((inb(0x1F7) & 0xC0) != 0x40)
		/* do nothing */;
}

void
readsect(void *dst, uint32_t offset)
{
	// wait for disk to be ready
	waitdisk();

	outb(0x1F2, 1);		// count = 1
	outb(0x1F3, offset);
	outb(0x1F4, offset >> 8);
	outb(0x1F5, offset >> 16);
	outb(0x1F6, (offset >> 24) | 0xE0);
	outb(0x1F7, 0x20);	// cmd 0x20 - read sectors

	// wait for disk to be ready
	waitdisk();

	// read a sector
	insl(0x1F0, dst, SECTSIZE/4);
}

