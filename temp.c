#include "loader.h"

Elf32_Ehdr *ehdr;
int fd;
int number_of_phdrs, phdr_offset, phdr_size;
void* entry_point;

void loader_cleanup(void *mapped_memory, size_t size) {
    munmap(mapped_memory, size);
    free(ehdr);
}

static void personal_handler(int signum, siginfo_t *info, void *context) {
    if (signum == SIGSEGV) {
        // Find the program header entry associated with the faulting address
        Elf32_Phdr *phdr = (Elf32_Phdr*)((uintptr_t)ehdr + ehdr->e_phoff);
        Elf32_Addr faulting_addr = (Elf32_Addr)info->si_addr;
        for (int i = 0; i < ehdr->e_phnum; ++i) {
            if (phdr[i].p_type == PT_LOAD) {
                Elf32_Addr segment_start = phdr[i].p_vaddr;
                Elf32_Addr segment_end = segment_start + phdr[i].p_memsz;
                if (faulting_addr >= segment_start && faulting_addr < segment_end) {
                    // Map the segment containing the faulting address
                    void *mapped_memory = mmap((void*)segment_start, segment_end - segment_start, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
                    if (mapped_memory == MAP_FAILED) {
                        perror("mmap");
                        exit(EXIT_FAILURE);
                    }
                    // Read segment data from the ELF file into the mapped memory
                    lseek(fd, phdr[i].p_offset, SEEK_SET);
                    read(fd, mapped_memory, phdr[i].p_filesz);
                    return;
                }
            }
        }
        // If the segment is not found, handle the error appropriately
        printf("Segmentation fault at address: %p\n", info->si_addr);
        exit(EXIT_FAILURE);
    }
}

void load_elf(char** exe){
    fd = open(*exe, O_RDONLY);
    int buff_size = lseek(fd, 0, SEEK_END);
    // 2. Iterate through the PHDR table and find the section of PT_LOAD 
    //    type that contains the address of the entrypoint method in fib.c
    int ehdrsize = sizeof(Elf32_Ehdr);
    ehdr = (Elf32_Ehdr*) malloc(ehdrsize * sizeof(char));
    lseek(fd, 0, SEEK_SET);
    read(fd, ehdr, ehdrsize * sizeof(char));

    number_of_phdrs = ehdr->e_phnum;
    phdr_offset = ehdr->e_phoff;
    phdr_size = ehdr->e_phentsize;
    entry_point = ehdr->e_entry;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Check ELF validity and open the file
    load_elf(&argv[1]);

    // Set up signal handler for SIGSEGV
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = personal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);

    // Get the entry point address
    entry_point = (void*)((uintptr_t)ehdr + ehdr->e_entry);

    // Call the entry point function
    int (*_start)() = (int(*)())entry_point;
    int result = _start();

    printf("User _start return value = %d\n", result);

    // Cleanup
    //loader_cleanup(entry_point, phdr_size * sizeof(char));
    close(fd);

    return 0;
}
