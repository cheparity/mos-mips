#ifndef _MMU_H_
#define _MMU_H_

#include <error.h>

#define BY2PG 4096		// Bytes To a Page = 4KB, 页面大小4KB,页面偏移12位
#define PDMAP (4 * 1024 * 1024) // Page Directory Map大小, 4MB
#define PGSHIFT 12
#define PDSHIFT 22
/* Page Directory Index, PDX(va)就是 va 的高10位,页目录位*/
#define PDX(va) ((((u_long)(va)) >> 22) & 0x03FF)
/*Page Table Index, PTX(va)是va的中间10位,即页表位(因为&0011 11111111,所以只取了右移之后10位的值).*/
#define PTX(va) ((((u_long)(va)) >> 12) & 0x03FF)
/*Page Table Entry Address,获取页表项中的物理地址（读取
 * pte时常用）,也可以用于获取二级页表的物理页号. 这个宏把低12位清零(&0000 0000 0000) 了,
 * 因为低12位是权限位
 */
#define PTE_ADDR(pte) ((u_long)(pte) & ~0xFFF)
// Page number field of an address
#define PPN(va) (((u_long)(va)) >> 12) // Page Physical Number, 物理页号
#define VPN(va) (((u_long)(va)) >> 12) // Virtual Page Number

/* Page Table/Directory Entry flags */

// Global bit. When the G bit in a TLB entry is set, that TLB entry will match solely on the VPN
// field, regardless of whether the TLB entry’s ASID field matches the value in EntryHi.
#define PTE_G 0x0100 // 0001 0000 0000

// Valid bit. If 0 any address matching this entry will cause a tlb miss exception (TLBL/TLBS).
#define PTE_V 0x0200 // 0010 0000 0000

// Dirty bit, but really a write-enable bit. 1 to allow writes, 0 and any store using this
// translation will cause a tlb mod exception (TLB Mod).
#define PTE_D 0x0400 // 0100 0000 0000

// Uncacheable bit. 0 to make the access cacheable, 1 for uncacheable.
#define PTE_N 0x0800 // 100000000000

// Copy On Write. Reserved for software, used by fork.
#define PTE_COW 0x0001

// Shared memmory. Reserved for software, used by fork.
#define PTE_LIBRARY 0x0004

// Memory segments (32-bit kernel mode addresses)
#define KUSEG 0x00000000U
#define KSEG0 0x80000000U
#define KSEG1 0xA0000000U
#define KSEG2 0xC0000000U

/*
 * Part 2.  Our conventions.
 */

/*
 o     4G ----------->  +----------------------------+------------0x100000000
 o                      |       ...                  |  kseg2
 o      KSEG2    -----> +----------------------------+------------0xc000 0000
 o                      |          Devices           |  kseg1
 o      KSEG1    -----> +----------------------------+------------0xa000 0000
 o                      |      Invalid Memory        |   /|\
 o                      +----------------------------+----|-------Physical Memory Max
 o                      |       ...                  |  kseg0
 o      KSTACKTOP-----> +----------------------------+----|-------0x8040 0000-------end
 o                      |       Kernel Stack         |    | KSTKSIZE            /|\
 o                      +----------------------------+----|------                |
 o                      |       Kernel Text          |    |                    PDMAP
 o      KERNBASE -----> +----------------------------+----|-------0x8001 0000    |
 o                      |      Exception Entry       |   \|/                    \|/
 o      ULIM     -----> +----------------------------+------------0x8000 0000-------
 o                      |         User VPT           |     PDMAP                /|\
 o      UVPT     -----> +----------------------------+------------0x7fc0 0000    |
 o                      |           pages            |     PDMAP                 |
 o      UPAGES   -----> +----------------------------+------------0x7f80 0000    |
 o                      |           envs             |     PDMAP                 |
 o  UTOP,UENVS   -----> +----------------------------+------------0x7f40 0000    |
 o  UXSTACKTOP -/       |     user exception stack   |     BY2PG                 |
 o                      +----------------------------+------------0x7f3f f000    |
 o                      |                            |     BY2PG                 |
 o      USTACKTOP ----> +----------------------------+------------0x7f3f e000    |
 o                      |     normal user stack      |     BY2PG                 |
 o                      +----------------------------+------------0x7f3f d000    |
 a                      |                            |                           |
 a                      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                           |
 a                      .                            .                           |
 a                      .                            .                         kuseg
 a                      .                            .                           |
 a                      |~~~~~~~~~~~~~~~~~~~~~~~~~~~~|                           |
 a                      |                            |                           |
 o       UTEXT   -----> +----------------------------+------------0x0040 0000    |
 o                      |      reserved for COW      |     BY2PG                 |
 o       UCOW    -----> +----------------------------+------------0x003f f000    |
 o                      |   reversed for temporary   |     BY2PG                 |
 o       UTEMP   -----> +----------------------------+------------0x003f e000    |
 o                      |       invalid memory       |                          \|/
 a     0 ------------>  +----------------------------+ ----------------------------
 o
*/

#define KERNBASE 0x80010000

#define KSTACKTOP (ULIM + PDMAP)
#define ULIM 0x80000000

#define UVPT (ULIM - PDMAP)
#define UPAGES (UVPT - PDMAP)
#define UENVS (UPAGES - PDMAP)

#define UTOP UENVS
#define UXSTACKTOP UTOP

#define USTACKTOP (UTOP - 2 * BY2PG)
#define UTEXT PDMAP
#define UCOW (UTEXT - BY2PG)
#define UTEMP (UCOW - BY2PG)

#ifndef __ASSEMBLER__

/*
 * Part 3.  Our helper functions.
 */
#include <string.h>
#include <types.h>

extern u_long npage;

typedef u_long Pde;
typedef u_long Pte;

/*
这个宏的作用是将一个kseg0的虚拟地址(Kernal Virtual Address)转换为物理地址Physical
Address。这个宏的实现是通过将虚拟地址减去ULIM（用户空间的上限）来得到物理地址。如果虚拟地址小于ULIM，那么就会触发panic。
*/
#define PADDR(kva)                                                                                 \
	({                                                                                         \
		u_long a = (u_long)(kva);                                                          \
		if (a < ULIM)                                                                      \
			panic("PADDR called with invalid kva %08lx", a);                           \
		a - ULIM;                                                                          \
	})

// translates from physical address to kernel virtual address
/*
这个宏是将物理地址(Physical Address)转化为kernal kseg0的虚拟地址Kernal virtual
Address,与上面的宏恰好相反
*/
#define KADDR(pa)                                                                                  \
	({                                                                                         \
		u_long ppn = PPN(pa);                                                              \
		if (ppn >= npage) {                                                                \
			panic("KADDR called with invalid pa %08lx", (u_long)pa);                   \
		}                                                                                  \
		(pa) + ULIM;                                                                       \
	})

#define assert(x)                                                                                  \
	do {                                                                                       \
		if (!(x)) {                                                                        \
			panic("assertion failed: %s", #x);                                         \
		}                                                                                  \
	} while (0)

#define TRUP(_p)                                                                                   \
	({                                                                                         \
		typeof((_p)) __m_p = (_p);                                                         \
		(u_int) __m_p > ULIM ? (typeof(_p))ULIM : __m_p;                                   \
	})

extern void tlb_out(u_int entryhi);
#endif //!__ASSEMBLER__
#endif // !_MMU_H_
