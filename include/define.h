#ifndef _define_h
#define _define_h
#include "mem.h"
#include "liste.h"
#include "elf/elf.h"
#include "elf/syms.h"


//Defines :
#define INPUT_SIZE 1024                 //Maximum size of an input string
#define PROMPT_STRING "EmulMips : > "
#define MAX_INSTR_ARG 3                 //Max argument an instruction can have
#define MAX_NAME_SIZE 10                //Maximum size of an instruction argument name string
#define DICONAME "dico.dico"            //name of the loaded dictionnary
#define NBREG 32                        //Number of register in the MIPS (not counting HI, LO, PC)
#define HI 32                           //index of HI, LO and PC register
#define LO 33
#define PC 34

//Typedef :


typedef struct{
uint function:6,sa:5,rd:5,rt:5,rs:5,opcode:6;
} r_type;
typedef struct{
int immediate:16; uint rt:5,rs:5,opcode:6;
} i_type;
typedef struct{
uint target:26,opcode:6;
} j_type;

typedef union{
r_type r;
i_type i;
j_type j;
uint32_t value;
}instruction;                           //union allowing easy access of each member of R, I or J instr, and also the int value

typedef struct {
	uint b4:8,
	b3:8,
	b2:8,
	b1:8;
} struct_word;                          //word of 4 bytes

typedef struct {
char name[MAX_NAME_SIZE];
uint32_t mask;
uint32_t instr;
short type;                             //0=R, 1=I,2=J
short nb_arg;
char argname[MAX_INSTR_ARG][MAX_NAME_SIZE];
int (*exec)(instruction,int,int*);
}dico_info;                             //Structure containing everything about an instruction


typedef enum {
    LOAD,
    EXIT,
    DISP,
    DISASM,
    SET,
    ASSERT,
    DEBUG,
    RESUME,
    RUN,
    STEP,
    BREAK,
    CLOCK,
    UNKNOWN
} command ;

//Global datas :
extern int32_t reg_mips[NBREG+3];       //All mips registers
extern mem memory;                      //Virtual machine memory
extern stab symtab;                     //loaded ELF symtab
extern int scriptmode;                  //allow easy switch between interactive and script mode
extern dico_info* dico_data;            //contain all info from the dictionnary
extern int nbinstr;                     //contain the number of instruction in the dictionnary
extern list breaklist;                  //Liste des points d'arrets
extern int clocktime;                       //Vitesse d'execution : 0 pour max
extern instruction insID, insEX, insMEM, insWB;
extern int EXtmp,MEMtmp,WBtmp;
extern int EXdic,MEMdic,WBdic;


#endif