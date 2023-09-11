#include "elf.h"
#include <stdio.h>

/* Overview:
 *   Check whether specified buffer is valid ELF data.
 *
 * Pre-Condition:
 *   The memory within [binary, binary+size) must be valid to read.
 *
 * Post-Condition:
 *   Returns 0 if 'binary' isn't an ELF, otherwise returns 1.
 */
int is_elf_format(const void *binary, size_t size) {
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;
	return size >= sizeof(Elf32_Ehdr) && ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
	       ehdr->e_ident[EI_MAG1] == ELFMAG1 && ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
	       ehdr->e_ident[EI_MAG3] == ELFMAG3;
}

/* Overview:
 *   Parse the sections from an ELF binary.
 *
 * Pre-Condition:
 *   The memory within [binary, binary+size) must be valid to read.
 *
 * Post-Condition:
 *   Return 0 if success. Otherwise return < 0.
 *   If success, output the address of every section in ELF.
 */

int readelf(const void *binary, size_t size) {
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;

	// Check whether `binary` is a ELF file.
	if (!is_elf_format(binary, size)) {
		fputs("not an elf file\n", stderr);
		return -1;
	}

	// Get the address of the section table, the number of section headers and the size of a
	// section header.
	const void *sh_table;
	Elf32_Half sh_entry_count;
	Elf32_Half sh_entry_size;
	/* Exercise 1.1: Your code here. (1/2) */
	
	sh_entry_count = ehdr -> e_shnum; //节的个数,也是节头表项的个数
	sh_entry_size = ehdr -> e_shentsize; //节头表项的大小

	sh_table = (Elf32_Shdr *)(binary + ehdr -> e_shoff); //节头表项的第一项地址

	// For each section header, output its index and the section address.
	// The index should start from 0.
	for (int i = 0; i < sh_entry_count; i++) {
		const Elf32_Shdr *shdr; 
		unsigned int addr;
		/* Exercise 1.1: Your code here. (2/2) */
		
		shdr = sh_table + i * ehdr -> e_shentsize; //这是每个节头表项的地址

		/*
		shdr = (Elf32_Shdr *)(binary + ehdr -> e_shoff) + i; 
		shdr = sh_table + i; //为什么这样不对,而上面是对的?
		*/
		
		addr = shdr -> sh_addr; //这才是每个节的地址
		printf("%d:0x%x\n", i, addr);
	}

	return 0;
}
