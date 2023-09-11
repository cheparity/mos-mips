#ifndef _PMAP_H_
#define _PMAP_H_

#include <mmu.h>
#include <printk.h>
#include <queue.h>
#include <types.h>

extern Pde* cur_pgdir;

LIST_HEAD(Page_list, Page);
/*
struct Page_list {
    // struct Page *lh_first;
    struct {
        //Page_LIST_entry_t pp_link
        struct {
            struct Page *le_next;
            struct Page **le_prev;
        } pp_link;

        u_short pp_ref;
    } *lh_first;
};
*/
typedef LIST_ENTRY(Page) Page_LIST_entry_t;
/*
typedef struct {
    struct Page *le_next;
    struct Page **le_prev;
}Page_LIST_entry_t;
*/
struct Page {
    /* free list link */
    Page_LIST_entry_t pp_link;
    /*
        struct Page_LIST_entry_t {
            struct Page *le_next;
            struct Page **le_prev;
        } pp_link;
    */
    // Ref is the count of pointers (usually in page table entries)
    // to this page.  This only holds for pages allocated using
    // page_alloc.  Pages allocated at boot time using pmap.c's "alloc"
    // do not have valid reference count fields.

    u_short pp_ref;
};

extern struct Page* pages;
extern struct Page_list page_free_list;

/*以下这几个函数非常重要!
注意,页控制块&页表项并不是在同一地址!*/

// 页地址->物理页号.请注意*两个指针相减得到的是偏移量,而不是绝对地址之差*.
static inline u_long page2ppn(struct Page* pp) {
    return pp - pages;  // 返回一个[0,npage]之间的值
}
// 页地址->物理地址.我们写的所有代码都在kseg0中,直接线性变换就行,并不是*pp来取得物理地址(这个是kuseg的取址方式)
static inline u_long page2pa(struct Page* pp) {
    return page2ppn(pp) << PGSHIFT;
}
// 物理地址->页地址.先获得物理页号PPN(pa),然后在pages数组里偏移物理页号PPN(pa),就得到了pages[i]的地址.
static inline struct Page* pa2page(u_long pa) {
    if (PPN(pa) >= npage) {
        panic("pa2page called with invalid pa: %x", pa);
    }
    return &pages[PPN(pa)];
}
// 页地址->内核虚拟地址.先把页地址转成物理地址,然后再通过KADDR获得物理地址对应的kseg0段的虚拟地址.
static inline u_long page2kva(struct Page* pp) {
    return KADDR(page2pa(pp));
}
// 虚拟地址->物理地址.给一个虚拟地址,查两级页表,找到物理地址
static inline u_long va2pa(Pde* pgdir, u_long va) {
    Pte* p;

    pgdir = &pgdir[PDX(va)];
    if (!(*pgdir & PTE_V)) {
        return ~0;
    }
    p = (Pte*)KADDR(PTE_ADDR(*pgdir));
    if (!(p[PTX(va)] & PTE_V)) {
        return ~0;
    }
    return PTE_ADDR(p[PTX(va)]);
}

void mips_detect_memory(void);
void mips_vm_init(void);
void mips_init(void);
void page_init(void);
void* alloc(u_int n, u_int align, int clear);

int page_alloc(struct Page** pp);
void page_free(struct Page* pp);
void page_decref(struct Page* pp);
int page_insert(Pde* pgdir, u_int asid, struct Page* pp, u_long va, u_int perm);
struct Page* page_lookup(Pde* pgdir, u_long va, Pte** ppte);
void page_remove(Pde* pgdir, u_int asid, u_long va);
void tlb_invalidate(u_int asid, u_long va);

extern struct Page* pages;

void physical_memory_manage_check(void);
void page_check(void);

#endif /* _PMAP_H_ */
