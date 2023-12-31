#include <env.h>
#include <lib.h>
#include <mmu.h>

/* Overview:
 *   Map the faulting page to a private writable copy.
 *
 * Pre-Condition:
 * 	'va' is the address which led to the TLB Mod exception.
 *
 * Post-Condition:
 *  - Launch a 'user_panic' if 'va' is not a copy-on-write page.
 *  - Otherwise, this handler should map a private writable copy of
 *    the faulting page at the same address.
 */
static void __attribute__((noreturn)) cow_entry(struct Trapframe *tf) {
	u_int va = tf->cp0_badvaddr;
	u_int perm;

	/* Step 1: Find the 'perm' in which the faulting address 'va' is mapped. */
	/* Hint: Use 'vpt' and 'VPN' to find the page table entry. If the 'perm' doesn't have
	 * 'PTE_COW', launch a 'user_panic'. */
	/* Exercise 4.13: Your code here. (1/6) */
	int r = 0;
	// Pte *pte = vpt[VPN(va)];
	// perm = *pte & 0xfff;
	perm = vpt[VPN(va)] & 0xfff; // 取出权限位
	if (!(perm & PTE_COW)) {
		user_panic("In user/lib/fork.c/duppage, cow_entry failed with value 1, %d\n", perm);
	}
	/* Step 2: Remove 'PTE_COW' from the 'perm', and add 'PTE_D' to it. */
	/* Exercise 4.13: Your code here. (2/6) */
	perm &= (!PTE_COW);
	perm |= PTE_D;
	/* Step 3: Allocate a new page at 'UCOW'. */
	/* Exercise 4.13: Your code here. (3/6) */
	if (r = syscall_mem_alloc(0, UCOW, perm) < 0) {
		user_panic("In user/lib/fork.c/duppage, cow_entry failed with value 2, %d\n", r);
	}

	/* Step 4: Copy the content of the faulting page at 'va' to 'UCOW'. */
	/* Hint: 'va' may not be aligned to a page! */
	/* Exercise 4.13: Your code here. (4/6) */
	memcpy(UCOW, PTE_ADDR(va), BY2PG); // va低12位清零

	// Step 5: Map the page at 'UCOW' to 'va' with the new 'perm'.
	/* Exercise 4.13: Your code here. (5/6) */
	if (r = syscall_mem_map(0, UCOW, 0, va, perm) < 0) {
		user_panic("In user/lib/fork.c/duppage, cow_entry failed with value 3, %d\n", r);
	}
	// Step 6: Unmap the page at 'UCOW'.
	/* Exercise 4.13: Your code here. (6/6) */
	if (r = syscall_mem_unmap(0, UCOW) < 0) {
		user_panic("In user/lib/fork.c/duppage, cow_entry failed with value 4, %d\n", r);
	}
	// Step 7: Return to the faulting routine.
	r = syscall_set_trapframe(0, tf);
	user_panic("syscall_set_trapframe returned %d", r);
}

/* Overview:
 *   Grant our child 'envid' access to the virtual page 'vpn' (with address 'vpn' * 'BY2PG') in our
 *   (current env's) address space.
 *   'PTE_COW' should be used to isolate the modifications on unshared memory from a parent and its
 *   children.
 *
 * Post-Condition:
 *   If the virtual page 'vpn' has 'PTE_D' and doesn't has 'PTE_LIBRARY', both our original virtual
 *   page and 'envid''s newly-mapped virtual page should be marked 'PTE_COW' and without 'PTE_D',
 *   while the other permission bits are kept.
 *
 *   If not, the newly-mapped virtual page in 'envid' should have the exact same permission as our
 *   original virtual page.
 *
 * Hint:
 *   - 'PTE_LIBRARY' indicates that the page should be shared among a parent and its children.
 *   - A page with 'PTE_LIBRARY' may have 'PTE_D' at the same time, you should handle it correctly.
 *   - You can pass '0' as an 'envid' in arguments of 'syscall_*' to indicate current env because
 *     kernel 'envid2env' converts '0' to 'curenv').
 *   - You should use 'syscall_mem_map', the user space wrapper around 'msyscall' to invoke
 *     'sys_mem_map' in kernel.
 */
static void duppage(u_int envid, u_int vpn) {
	int r;
	u_int addr;
	u_int perm;

	/* Step 1: Get the permission of the page. */
	/* Hint: Use 'vpt' to find the page table entry. */
	/* Exercise 4.10: Your code here. (1/2) */
	addr = vpn * BY2PG;
	// Pte *pte = &vpt[vpn];
	// perm = *pte & 0xfff; // 取出权限位

	perm = vpt[vpn] & 0xfff; // 取出权限位

	/* Step 2: If the page is writable, and not shared with children, and not marked as COW yet,
	 * then map it as copy-on-write, both in the parent (0) and the child (envid). */
	/* Hint: The page should be first mapped to the child before remapped in the parent. (Why?)
	 */
	/* Exercise 4.10: Your code here. (2/2) */
	if ((perm & PTE_D) && !(perm & PTE_LIBRARY) && !(perm & PTE_COW)) {
		perm |= PTE_COW;
		r = syscall_mem_map(0, (void *)addr, envid, (void *)addr, (perm & ~PTE_D));
		r = syscall_mem_map(0, (void *)addr, 0, (void *)addr, (perm & ~PTE_D));
	} else {
		syscall_mem_map(0, (void *)addr, envid, (void *)addr, perm);
	}

	if (r < 0) {
		user_panic("In user/lib/fork.c/duppage, syscall_mem_map failed on return "
			   "value %d\n",
			   r);
	}
}

/* Overview:
 *   User-level 'fork'. Create a child and then copy our address space.
 *   Set up ours and its TLB Mod user exception entry to 'cow_entry'.
 *
 * Post-Conditon:
 *   Child's 'env' is properly set.
 *
 * Hint:
 *   Use global symbols 'env', 'vpt' and 'vpd'.
 *   Use 'syscall_set_tlb_mod_entry', 'syscall_getenvid', 'syscall_exofork',  and 'duppage'.
 */
int fork(void) {
	u_int child;
	u_int i;
	extern volatile struct Env *env;
	int ret = 0;
	/* Step 1: Set our TLB Mod user exception entry to 'cow_entry' if not done yet. */
	if (env->env_user_tlb_mod_entry != (u_int)cow_entry) {
		try(syscall_set_tlb_mod_entry(0, cow_entry));
	}

	/* Step 2: Create a child env that's not ready to be scheduled. */
	// Hint: 'env' should always point to the current env itself, so we should fix it to the
	// correct value.
	child = syscall_exofork();
	if (child == 0) {
		// debugf("这里成功进去了\n");
		env = envs + ENVX(syscall_getenvid());
		// debugf("child ra is %ld", env->env_tf.regs[31]);
		return 0;
	}

	/* Step 3: Map all mapped pages below 'USTACKTOP' into the child's address space. */
	// Hint: You should use 'duppage'.
	/* Exercise 4.15: Your code here. (1/2) */
	for (i = 0; i < VPN(USTACKTOP); i++) {
		if ((vpd[i >> 10]) && (vpt[i])) {
			duppage(child, i);
		}
	}

	/* Step 4: Set up the child's tlb mod handler and set child's 'env_status' to
	 * 'ENV_RUNNABLE'. */
	/* Hint:
	 *   You may use 'syscall_set_tlb_mod_entry' and 'syscall_set_env_status'
	 *   Child's TLB Mod user exception entry should handle COW, so set it to
	 * 'cow_entry'
	 */
	/* Exercise 4.15: Your code here. (2/2) */

	if (ret = syscall_set_tlb_mod_entry(child, cow_entry)) {
		user_panic("In user/lib/fork.c/fork, fork failed on return value 1, %d", ret);
		return ret;
	}

	if (ret = syscall_set_env_status(child, ENV_RUNNABLE) < 0) {
		user_panic("In user/lib/fork.c/fork, fork failed on return value 3, %d", ret);
		return ret;
	}
	// debugf("father ra is %ld", env->env_tf.regs[31]);

	return child;
}
