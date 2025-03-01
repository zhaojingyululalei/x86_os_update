#ifndef __ELF_H
#define __ELF_H



#include "types.h"

// ELF相关数据类型
typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef uint32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

#pragma pack(1)

// ELF Header
#define EI_NIDENT       16
#define ELF_MAGIC       0x7F
#define ET_EXEC     2
#define EM_386      3

typedef struct {
    char e_ident[EI_NIDENT];
    Elf32_Half e_type;           //EXEC
    Elf32_Half e_machine;        //80386
    Elf32_Word e_version;
    Elf32_Addr e_entry;         
    Elf32_Off e_phoff;        //program head offset
    Elf32_Off e_shoff;          //section head offset
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;      //elf head size
    Elf32_Half e_phentsize;    //program head size
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;    //section head size
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;       //section index
}Elf32_Ehdr;

#define PT_LOAD         1

typedef struct {
    Elf32_Word p_type;
    Elf32_Off p_offset;    //程序在文件中的偏移量(not program header but program)
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;   //不包含bss
    Elf32_Word p_memsz;    //包含bss大小
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr;

#pragma pack()

#endif