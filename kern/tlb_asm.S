#include <asm/asm.h>

LEAF(tlb_out) #页函数定义,用于从TLB中删除一个页表项
.set noreorder #不允许优化改变执行顺序
	mfc0    t0, CP0_ENTRYHI #将CP0_ENTRYHI寄存器的值保存到t0寄存器中
	mtc0    a0, CP0_ENTRYHI #将a0寄存器的值(即要删除的虚拟地址)写入CP0_ENTRYHI 寄存器
	nop
	/* Step 1: Use 'tlbp' to probe TLB entry */
	/* Exercise 2.8: Your code here. (1/2) */
	tlbp #根据EntryHi 中的 Key（包含 VPN 与 ASID），查找 TLB 中与之对应的表项，并将表项的索引存入Index 寄存器（若未找到匹配项，则Index最高位被置1）
	nop 
	/* Step 2: Fetch the probe result from CP0.Index */
	mfc0    t1, CP0_INDEX #将查找结果保存到 CP0_INDEX 寄存器中
.set reorder #允许优化改变执行顺序
	bltz    t1, NO_SUCH_ENTRY #如果t1<0即最高位为1(没找到TLB项),则跳到.NO_SUCH_ENTRY
.set noreorder
	mtc0    zero, CP0_ENTRYHI #如果找到,则将CP0_ENTRYHI和CP0_ENTRYLO0清零(删掉)
	mtc0    zero, CP0_ENTRYLO0
	nop
	/* Step 3: Use 'tlbwi' to write CP0.EntryHi/Lo into TLB at CP0.Index  */
	/* Exercise 2.8: Your code here. (2/2) */
	tlbwi #以 Index 寄存器中的值为索引，将此时 EntryHi 与 EntryLo 的值写到索引指定的TLB表项中。
.set reorder 

NO_SUCH_ENTRY: #没找到或者找到运行完,恢复上一个值,函数返回
	mtc0    t0, CP0_ENTRYHI
	j       ra
END(tlb_out)

.data
tlb_refill_ra: #函数用于处理 TLB 缺失异常
.word 0
.text
NESTED(do_tlb_refill, 0, zero) #定义一个宏
	mfc0    a0, CP0_BADVADDR #保存引发地址异常的虚拟地址
	mfc0    a1, CP0_ENTRYHI #把CP0_ENTRYHI存到a1
	srl     a1, a1, 6 #CP0_ENTRYHI右移6位(把低6位0给扔了)
	andi    a1, a1, 0b111111  #取出ASID,VPN清零
	sw      ra, tlb_refill_ra #存ra到tlb_refill_ra
	jal     _do_tlb_refill #跳转到_do_tlb_refill
	lw      ra, tlb_refill_ra #取返回值
	mtc0    v0, CP0_ENTRYLO0 #把函数返回值存到CP0_ENTRYLO0
	// See <IDT R30xx Family Software Reference Manual> Chapter 6-8
	nop
	/* Hint: use 'tlbwr' to write CP0.EntryHi/Lo into a random tlb entry. */
	/* Exercise 2.10: Your code here. */
	tlbwr #将 EntryHi 与 EntryLo 的数据随机写到一个 TLB 表项中（此处使用 Random 寄存器来“随机”指定表项，Random寄存器本质上是一个不停运行的循环计数器）。
	jr      ra # return
END(do_tlb_refill)
