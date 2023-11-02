#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
int phdr_size;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
    free(ehdr);
    free(phdr);
}

int Check_Magic_Num(Elf32_Ehdr* ehdr){
    if (!ehdr)
        return 1;
    else if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 || ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3)
        return 1;    
}

void ELF_checker(char** exe){
    fd = open(*exe, O_RDONLY);
    int esize = sizeof(Elf32_Ehdr);
    ehdr = (Elf32_Ehdr*)malloc(esize * sizeof(char));
    lseek(fd, 0 , SEEK_SET);
    read(fd, ehdr, esize * sizeof(char));
    int a = Check_Magic_Num(ehdr);
    if (a == 1){
        printf("Invalid ELF file.\n");
        exit(0);
    }
    free(ehdr);
    close(fd);
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
    fd = open(*exe, O_RDONLY);
    // 1. Load entire binary content into the memory from the ELF file.
    int buff_size = lseek(fd, 0, SEEK_END);
    char * buff;
    lseek(fd, 0 , SEEK_SET);
    buff = (char*) malloc (buff_size*sizeof(char));
    read(fd, buff, buff_size);
    // 2. Iterate through the PHDR table and find the section of PT_LOAD 
    //    type that contains the address of the entrypoint method in fib.c
    int ehdrsize = sizeof(Elf32_Ehdr);
    ehdr = (Elf32_Ehdr*) malloc(ehdrsize * sizeof(char));
    lseek(fd, 0, SEEK_SET);
    read(fd, ehdr, ehdrsize * sizeof(char));

    int number_of_phdrs = ehdr->e_phnum;
    int phdr_offset = ehdr->e_phoff;
    phdr_size = ehdr->e_phentsize;
    int entryaddr = ehdr->e_entry;

    int (*_start)()= (int(*)())(entryaddr);

    int virt=0, virtadd = 0, virtmemz = 0, virtoff = 0;
    for (int i = 0; i < number_of_phdrs; i++) {
        phdr = (Elf32_Phdr*) malloc(phdr_size * sizeof(char));
        lseek(fd, phdr_offset + i * phdr_size, SEEK_SET);
        read(fd, phdr, phdr_size);

        int ptype = phdr->p_type;
        int vadr = phdr->p_vaddr;
        if (ptype == PT_LOAD ){
            if (vadr <= entryaddr && vadr + phdr->p_memsz > entryaddr){
                virt = i;
                virtadd = vadr;
                virtmemz = phdr->p_memsz;
                virtoff = phdr->p_offset;
                break;
            }
            }
    }
    // 3. Allocate memory of the size "p_memsz" using mmap function 
    //    and then copy the segment content
    Elf32_Phdr* virtual_mem = mmap(NULL, virtmemz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
    lseek(fd, virtoff, SEEK_SET);
    read(fd, virtual_mem, virtmemz);

    // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
    void* final_addr = ((void*)(virtual_mem) + (entryaddr-virtadd));


    // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
    //int (*_start)()= (int(*)())(final_addr);

    // 6. Call the "_start" method and print the value returned from the "_start"
    int result = _start();
    printf("User _start return value = %d\n",result);
    free(buff);
    munmap(virtual_mem, virtmemz);
    close(fd);
}

static void personal_handler(int signum){
    if (signum == SIGSEGV){
        //phdr = (Elf32_Phdr*)mmap(NULL, phdr_size*sizeof(char), PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        printf("pakda gya na");
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
    sig.sa_handler = personal_handler;
    sigaction(SIGSEGV, &sig, NULL);


    // 2. passing it to the loader for carrying out the loading/execution
    load_and_run_elf(&argv[1]);
    // 3. invoke the cleanup routine inside the loader  
    loader_cleanup();
    return 0;
}