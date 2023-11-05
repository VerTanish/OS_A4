#OS Assignment 4
Group 60
Tanish Verma(2022532)
Kirti Jain(2022250)

This is a simple smart loader where the memory allocations while executing an elf file are done after receiving a segmentation fault. The segment in question exists, but the memory for it is allocated using paging. Page size is 4KB.
_start() function is called without memory allocation and 

# Segfault signal handler
If a signal received is a segmentation fault, it retrieves the faulting address (segfaddr) from the siginfo_t structure. The total page faults are incremented by one to keep track of the segmentation faults encountered. “phdrarray” is iterated through to check if the faulting address falls within the memory range specified by the program header. If it does, the size of the memory to be allocated is calculated using the ceil function which gives the greatest integer function multiple of the page size. 
Memory is allocated using mmap and total page allocation count is increased. Contents of the program header are read and memory is allocated for a mapped_phdr structure. This structure keeps track of allocated memory.

# With bonus 
For with bonus, the working of the segfault handler is almost the same as wihtout bonus, only that the memory is allocated only one page at a time, after receiving a seg fault. 

# Contributions
Tanish Verma(2022532), Kirti Jain(2022250) contributed equally

repo link: https://github.com/VerTanish/OS_A4