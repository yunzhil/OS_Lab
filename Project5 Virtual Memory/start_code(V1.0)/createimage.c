#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

int write_bootblock();
int write_kernel();
int write_process1();
int write_process2();

int main(int argc, char *argv[])
{
	write_bootblock();
	write_kernel();
	write_process1();
	write_process2();
	
	
	return 0;
}

int write_bootblock()
{
	//write bootblock
	//open elf file "bootblock"
	FILE *fp = fopen("bootblock", "rb");
	Elf32_Ehdr *elf;
	Elf32_Phdr *ph = NULL;

	//read "bootblock" and get phdr
	uint8_t buf[65536];
	fread(buf, 65536, 1, fp);
	elf = (void *)buf;
	ph = (void *)buf + elf->e_phoff;

	//read bootblock program
	uint8_t program[65536] = {0};
	fseek(fp, ph->p_offset, SEEK_SET);
	fread(program, 1, ph->p_filesz, fp);
	fclose(fp);

	//open imagefile "image" and write bootblock
	fp = fopen("image", "wb");
	fwrite(program, 0x200, 1, fp);
	fclose(fp);

	return 0;
}

int write_kernel()
{
	//write kernel
	//open elf file "kernel"
	FILE *fp = fopen("kernel", "rb");
	Elf32_Ehdr *elf;
	Elf32_Phdr *ph = NULL;

	//read "kernel" and get phdr
	uint8_t buf[65536];
	fread(buf, 65536, 1, fp);
	elf = (void *)buf;
	ph = (void *)buf + elf->e_phoff;

	//read kernel program
	uint8_t program[65536] = {0};
	fseek(fp, ph->p_offset, SEEK_SET);
	fread(program, 1, ph->p_filesz, fp);
	fclose(fp);

	//open imagefile "image" and write bootblock
	fp = fopen("image", "a+");
	fwrite(program, 0x9e00, 1, fp);
	fclose(fp);

	return 0;
}

int write_process1()
{
	//write process1
	//open elf file "process1"
	FILE *fp = fopen("process1", "rb");
	Elf32_Ehdr *elf;
	Elf32_Phdr *ph = NULL;

	//read "process1" and get phdr
	uint8_t buf[65536];
	fread(buf, 65536, 1, fp);
	elf = (void *)buf;
	ph = (void *)buf + elf->e_phoff;

	//read kernel program
	uint8_t program[65536] = {0};
	fseek(fp, ph->p_offset, SEEK_SET);
	fread(program, 1, ph->p_filesz, fp);
	fclose(fp);

	//open imagefile "image" and write bootblock
	fp = fopen("image", "a+");
	fwrite(program, 0x1000, 1, fp);
	fclose(fp);

	return 0;
}

int write_process2()
{
	//write process2
	//open elf file "process2"
	FILE *fp = fopen("process2", "rb");
	Elf32_Ehdr *elf;
	Elf32_Phdr *ph = NULL;

	//read "process2" and get phdr
	uint8_t buf[65536];
	fread(buf, 65536, 1, fp);
	elf = (void *)buf;
	ph = (void *)buf + elf->e_phoff;

	//read kernel program
	uint8_t program[65536] = {0};
	fseek(fp, ph->p_offset, SEEK_SET);
	fread(program, 1, ph->p_filesz, fp);
	fclose(fp);

	//open imagefile "image" and write bootblock
	fp = fopen("image", "a+");
	fwrite(program, 0x1000, 1, fp);
	fclose(fp);

	return 0;
}
