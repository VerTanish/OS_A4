#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr = NULL;
int fd;
int phdr_size;
Elf32_Phdr **phdrarray;


int number_of_phdrs, phdr_offset, entryaddr;
int total_page_faults = 0;
int total_page_allocations = 0;
int internal_fragmentation = 0;
int PAGE_SIZE = 4096;
int globalcount = 1;
void* final_addr;

typedef struct mapped_phdr{
    Elf32_Phdr* m_phdr;
    int size;
}mapped_phdr;

mapped_phdr **deletionarray;
int globalindex = 0;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
    free(ehdr);
    for (int i=0; i<number_of_phdrs; i++){
        free(phdrarray[i]);
    }
    free(phdrarray);

    for (int i =0 ; i<globalindex; i++){
        if (deletionarray[i] != NULL){
            if (munmap(deletionarray[i]->m_phdr, deletionarray[i]->size) == -1){
                perror("Error: munmap");
                exit(0);
            }
            free(deletionarray[i]);
        }
    }
    free(deletionarray);
}

int Check_Magic_Num(Elf32_Ehdr* ehdr){
    if (!ehdr)
        return 1;
    else if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 || ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3)
        return 1;    
}

void ELF_checker(char** exe){
    fd = open(*exe, O_RDONLY);
    if (fd == -1){
        perror("Error while opening");
        exit(0);
    }
    int esize = sizeof(Elf32_Ehdr);
    ehdr = (Elf32_Ehdr*)malloc(esize * sizeof(char));                
    if (ehdr == NULL){
        perror("Error: malloc");
        exit(0);
    }
    lseek(fd, 0 , SEEK_SET);
    read(fd, ehdr, esize * sizeof(char));
    int a = Check_Magic_Num(ehdr);
    if (a == 1){
        printf("Invalid ELF file.\n");
        free(ehdr);
        exit(0);
    }
    free(ehdr);
    int closeerr = close(fd);
    if (closeerr == -1){
        perror("Error while closing");
        exit(0);
    }
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
    fd = open(*exe, O_RDONLY);
    if (fd == -1){
        perror("Error while opening");
        exit(0);
    }
    // 1. Load entire binary content into the memory from the ELF file.
    int buff_size = lseek(fd, 0, SEEK_END);
    if (buff_size == -1){
        perror("Error: lseek");
        exit(0);
    }
    // 2. Iterate through the PHDR table and find the section of PT_LOAD 
    //    type that contains the address of the entrypoint method in fib.c
    int ehdrsize = sizeof(Elf32_Ehdr);
    ehdr = (Elf32_Ehdr*) malloc(ehdrsize * sizeof(char));
    if (ehdr == NULL){
        perror("Error: malloc");
        exit(0);
    }
    if (lseek(fd, 0, SEEK_SET) == -1){
        perror("Error: lseek");
        exit(0);
    }
    if (read(fd, ehdr, ehdrsize * sizeof(char)) == -1){
        perror("Error: read");
        exit(0);
    }

    number_of_phdrs = ehdr->e_phnum;
    phdr_offset = ehdr->e_phoff;
    phdr_size = ehdr->e_phentsize;
    entryaddr = ehdr->e_entry;

    phdrarray = (Elf32_Phdr**)malloc(phdr_size*sizeof(char)*number_of_phdrs);
    if (phdrarray == NULL){
        perror("Error: malloc");
        exit(0);
    }
    deletionarray = (mapped_phdr**)malloc(sizeof(mapped_phdr)*number_of_phdrs*32);
    if (deletionarray == NULL){
        perror("Error: malloc");
        exit(0);
    }

    for (int i=0; i<number_of_phdrs; i++){
        Elf32_Phdr* tempphdr = (Elf32_Phdr*)malloc(phdr_size*sizeof(char));
        if (tempphdr == NULL){
            perror("Error: malloc");
            exit(0);
        } 
        if (lseek(fd, phdr_offset + i * phdr_size, SEEK_SET) == -1){
            perror("Error: lseek");
            exit(0);
        }
        if (read(fd, tempphdr, phdr_size) == -1){
            perror("Error: read");
            exit(0);
        }
        phdrarray[i] = tempphdr;
    }
    // for (int i=0; i<number_of_phdrs; i++){
    //         int vadr = phdrarray[i]->p_vaddr;
    //         printf("%x\n", vadr);
    //     }

    final_addr = (void*)entryaddr;
    int (*_start)()= (int(*)())(final_addr);

    // 6. Call the "_start" method and print the value returned from the "_start"
    int result = _start();
    printf("User _start return value = %d\n",result);
    printf("Total number of page faults = %d\n", total_page_faults);
    printf("Total number of page allocations = %d\n", total_page_allocations);
    printf("Total amount of internal fragmentation in KB = %.2f\n", (double)internal_fragmentation/1024);
    if (close(fd) == -1){
        perror("Error while closing");
        exit(0);
    }

}

static void personal_handler(int signum, siginfo_t *info, void *context){
    if (signum == SIGSEGV){
        void* segfaddr = info->si_addr;
        total_page_faults++;
        // phdr = (Elf32_Phdr*)mmap(info->si_addr, phdr_size*sizeof(char), PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        for (int i=0; i<number_of_phdrs; i++){
            int vadr = phdrarray[i]->p_vaddr;
            if (vadr <= (int)segfaddr && (vadr + phdrarray[i]->p_memsz) > (int)segfaddr){
                int tempsize = phdrarray[i]->p_memsz;
                int ceilval = (tempsize + (int)PAGE_SIZE - 1) / (int) PAGE_SIZE;      //ceil val of division (number of pages)
                if (globalcount > 1){
                    phdr = (Elf32_Phdr*)mmap((void*)(vadr + (void*)((globalcount-1)*4096)), (int)PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
                }
                else{
                    phdr = (Elf32_Phdr*)mmap((void*)vadr, (int)PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);

                }
                if ((int)phdr == -1){
                    perror("Error: mmap");
                    exit(0);
                }

                total_page_allocations++;
                if (globalcount == 1){
                    internal_fragmentation += (ceilval*(int)PAGE_SIZE) - tempsize;
                }
                if (lseek(fd, phdrarray[i]->p_offset + (globalcount-1)*4096, SEEK_SET) == -1){
                    perror("Error: lseek");
                    exit(0);
                }
                if (read(fd, phdr, phdrarray[i]->p_memsz - (globalcount-1)*4096) == -1){
                    perror("Error: read");
                    exit(0);
                }

                mapped_phdr* temp = NULL;
                temp = (mapped_phdr*)malloc(sizeof(mapped_phdr));
                if (temp == NULL){
                    perror("Error: malloc");
                    exit(0);
                }
                temp->m_phdr = phdr;
                temp->size = ceilval*(int)PAGE_SIZE;
                deletionarray[globalindex++] = temp;
                if (globalcount < ceilval){
                    globalcount++;
                }
                else{
                    globalcount = 1;
                }
                break;
            }
        }

        }
}

int main(int argc, char** argv) 
{
    if(argc != 2) {
        printf("Usage: %s <ELF Executable> \n",argv[0]);
        exit(1);
    }
    // 1. carry out necessary checks on the input ELF file
    ELF_checker(&argv[1]);

    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_sigaction = personal_handler;
    sig.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &sig, NULL) == -1){
        perror("Error: sigaction");
        exit(0);
    }


    // 2. passing it to the loader for carrying out the loading/execution
    load_and_run_elf(&argv[1]);
    // 3. invoke the cleanup routine inside the loader  
    loader_cleanup();
    return 0;
}