#OS Assignment 4
Group 60
Tanish Verma(2022532)
Kirti Jain(2022250)

Note: Bonus part is done in a separate file, so as to compare the results of the two approaches.

This is a simple smart loader where the memory allocations while executing an elf file are done after receiving a segmentation fault. The segment in question exists, but the memory for it is allocated using paging. Page size is 4KB.
_start() function is called without memory allocation and the segments where segfault is received get loaded.
Also, the elf header and phdr table are read and stored globally.

# Segfault signal handler
If a signal received is a segmentation fault, it retrieves the faulting address (segfaddr) from the siginfo_t structure. The total page faults are incremented by one to keep track of the segmentation faults encountered. “phdrarray” is iterated through to check if the faulting address falls within the memory range specified by the program header, and which segment it is contained in. If it does, the size of the memory to be allocated is calculated using the ceil function which gives the greatest integer function multiple of the page size, and that segment gets loaded using mmap in which the first parameter is the address of that segment, and size is specified as above. 
After memory is allocated using mmap and total page allocation count is increased. Contents of the program header are read and memory is allocated for a mapped_phdr structure. This structure keeps track of allocated memory, and is stored in a global deletion array so that it gets munmap during cleanup.

# With bonus 
For with bonus, the working of the segfault handler is almost the same as without bonus, only that the memory is allocated only one page at a time, after receiving a seg fault. This is done with the help of a global counter, the global counter will get updated to globalcount++ until it equals the number of pages needed, then it will simply be set as 1. Using this concept of global counter, the size allocated at each segfault is 1 page, and the address at which it is allocated is the vaddr + (globalcount-1)*4096, as if only 1 page needed, address is simply vaddr, otherwise it gets increased by size of page after each page gets allocated (note, here the address being talked about is the first parameter of mmap).
After this, for cleanup, an array of large size is used as done in without bonus part.

Note: error checks and cleanup is done for all required functions.

# Contributions
Tanish Verma(2022532), Kirti Jain(2022250)- both contributed equally

repo link: https://github.com/VerTanish/OS_A4