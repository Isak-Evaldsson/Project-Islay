# Function to invalidate a certain address in the TLB
.section .text
.global tlb_invalid_page
tlb_invalid_page:
	movl	4(%esp),%eax
	invlpg	(%eax) # This techincally breaks the i386 compatibility since the instruction was introduced for i486, however for a hobby kernel it wont be a problem :)
	ret
