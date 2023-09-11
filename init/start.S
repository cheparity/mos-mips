#include <asm/asm.h>
#include <mmu.h>

.text
EXPORT(_start)
.set at
.set reorder
	/* disable interrupts */
	/*将通用寄存器zero(值为0)的值移动到CP0_STATUS寄存器,也就是将CP0_STATUS寄存器清零.这样做的目的可能是为了屏蔽中断或者恢复正常模式*/
	mtc0    zero, CP0_STATUS 

	/* hint: you can reference the memory layout in include/mmu.h */
	/* set up the kernel stack */
	/* Exercise 1.3: Your code here. (1/2) */
	
	li sp,0x80400000

	/* jump to mips_init */
	/* Exercise 1.3: Your code here. (2/2) */
	jal mips_init