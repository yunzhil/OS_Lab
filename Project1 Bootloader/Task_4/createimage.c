#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int extend;
int size_of_kernel=0;
uint32_t base;
uint32_t kernel_count=0;
uint32_t count_sector=0;


void print_info(int type,uint32_t num_section, Elf32_Off p_offset, Elf32_Off p_vaddr, Elf32_Word p_filesz, Elf32_Word p_memsz, int size)
{
    if(type==0)
    printf("bootblock image info\n");
    else
    printf("kernel image info\n");
    printf("sectors: %d\n",num_section);
    printf("offset of segment in the file: 0x%x\n",p_offset);
    printf("the image's virtural address of segment in memory: 0x%x\n",p_vaddr);
	printf("the file image size of segment: 0x%x\n",p_filesz);
	printf("the memory image size of segment: 0x%x\n",p_memsz);
    printf("the size of write to the OS image: 0x%x\n",p_filesz);
	printf("padding up to 0x%x\n",size*512);	
}

void loader_bootblock(FILE **image_file,FILE **boot_file)
{
    FILE *fp=*boot_file;
    FILE *image=*image_file;
    assert(fp);
    Elf32_Ehdr *elf;
    Elf32_Phdr *ph=NULL;
    int i;
    uint8_t buf[4096];

    fread(buf,4096,1,fp);
    elf = (void*) buf;
    const uint32_t elf_magic = 0x464c457f;
    uint32_t *p_magic = (void*)buf;
    assert(*p_magic==elf_magic);

    for(i=0,ph = (void*)buf+elf->e_phoff;i<elf->e_phnum;i++)
    {
        if(ph[i].p_type==PT_LOAD)
        {
            count_sector++;
            uint32_t addr = ph[i].p_vaddr;
            base=addr;
            fseek(fp,ph[i].p_offset,SEEK_SET);
            uint8_t newbuf[512];
            memset(newbuf,0,512);
            fread(newbuf,1,ph[i].p_filesz,fp);
            fwrite(newbuf,1,512,image);
            fflush(image);

            if(extend)
                print_info(0, count_sector, ph[i].p_offset, ph[i].p_vaddr, ph[i].p_filesz, ph[i].p_memsz,count_sector);
        }
    }
}

void loader_kernel(FILE **image_file,FILE **kernel_file)
{
    FILE *fp=*kernel_file;
    FILE *image=*image_file;
    assert(fp);
    Elf32_Ehdr *elf;
    Elf32_Phdr *ph=NULL;
    int i,j;
    uint8_t buf[4096];
    fread(buf,4096,1,fp);
    elf = (void*) buf;
    const uint32_t elf_magic = 0x464c457f;
    uint32_t *p_magic = (void*)buf;
    assert(*p_magic==elf_magic);

    for(i=0,ph = (void*)buf+elf->e_phoff;i<elf->e_phnum;i++)
    {
        if(ph[i].p_type==PT_LOAD)
        {
            uint32_t offset = ph[i].p_vaddr - base;
            uint32_t temp_for_while=ph[i].p_filesz;
            fseek(fp,ph[i].p_offset,SEEK_SET);
            uint8_t newbuf[512];
            memset(newbuf,0,512);
            if(offset+ph[i].p_memsz >(kernel_count+1)*512 )
            {
                for(j= count_sector;j*512<offset+ph[i].p_memsz;j++)
                {
                    fseek(fp,0,SEEK_END);
                    fwrite(newbuf,1,512,image);
                    fflush(image);
                }
            }

            uint8_t *write_buf = malloc(ph[i].p_filesz);
            fseek(fp, ph[i].p_offset, SEEK_SET);
            fread(write_buf, 1, ph[i].p_filesz, fp);
            fseek(image, offset, SEEK_SET);
            fwrite(write_buf, 1, ph[i].p_filesz, image);
            fflush(image);
            free(write_buf);
            kernel_count=j-count_sector;
            if(extend)
                print_info(1, kernel_count, ph[i].p_offset, ph[i].p_vaddr, ph[i].p_filesz, ph[i].p_memsz,(j-count_sector));
            count_sector = j;
        }
    }
}

int main(int argc, char *argv[]){

   if(strcmp(argv[1],"--extended")==0)
       extend=1;
    FILE *image=fopen("image","wb");

    FILE *boot=fopen(argv[extend+1],"rb");
    loader_bootblock(&image,&boot);
    fclose(boot);

    int i=0;
    for(i=extend+2;i<argc;i++)
    {
        FILE *kernel=fopen(argv[i],"rb");
        loader_kernel(&image,&kernel);
        fclose(kernel);
    }

    fseek(image,0,SEEK_SET);
    fwrite(&kernel_count,sizeof(kernel_count),1,image);

     if(strcmp(argv[1],"--extended")==0)
       extend=1;
     boot=fopen(argv[extend+1],"rb");
     fseek(boot,0,2);
     printf("length of bootblock = %d\n",ftell(boot));
     fclose(boot);
     for(i=extend+2;i<argc;i++)
     {
          FILE *kernel=fopen(argv[i],"rb");
          fseek(kernel,0,2);
          printf("length of kernel = %d\n",ftell(kernel));
          fclose(kernel);
     }  

    fflush(image);
    fclose(image);
    return 0;

}

