#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>			//Gestion du temps
#include "define.h"
#include "liste.h"			//Gestion des breakpoints
#include "disasm.h"
#include "execute.h"
#include "fonctions.h"
#include "pipeline.h"
#include "common/notify.h"
#include "emul.h"

int exceptionHandler(exception number) {
    //Permet de parser les erreurs et exceptions
    switch(number) {
    case OK:
        break;

    case EmptyPipe:
        //WARNING_MSG("Empty pipe");
        break;


    case InvalidInstruction:
        if (verbose==1||verbose>2) {
            WARNING_MSG("Invalid instruction at adress %X",reg_mips[PC]);
        }
        break;

    case InvalidExecution:
        if (verbose==1||verbose>2) {
            WARNING_MSG("Invalid execution in pipe");
        }
        break;



    case IntegerOverflow:
        if (verbose==1||verbose>2) {
            WARNING_MSG("IntegerOverflow : %8.8X",vpipeline[EX].ins.value);
        }
        break;

    case BreakPoint:
        if (verbose==1||verbose>2) {
            WARNING_MSG("Break : %8.8X",vpipeline[EX].ins.value);
        }
        return BreakPoint;
        break;

    case SysCall:
        if (verbose==1||verbose>2) {
            INFO_MSG("Syscall : %8.8X",vpipeline[WB].ins.value);
        }
        switch (reg_mips[2]) { //v0
        case 1:
            printf("%d\n",reg_mips[4]); //a0
            break;
        case 4:
            ;
            int i=0,c;
            do {
                memRead(reg_mips[4]+i,0,&c);
                printf("%c",(char)c);
                i+=1;
            }
            while(((char)c)!='\0');
            printf("\n");
            break;
        case 5:
            scanf("%d",&reg_mips[2]);
            break;
        case 8:
            ;
            int j=0;
            char *input=calloc(reg_mips[5],sizeof(char));
            fgets(input,reg_mips[5],stdin);
            do
            {
                j++;
                memWrite(reg_mips[4]+j,0,input[j]);
            } while((input[j-1])!='\0' && j <= reg_mips[5]);
            break;
        case 10:
            INFO_MSG("Exit called by program");
            return quit;
            break;
        default:
            if (verbose==1||verbose>2) {
                WARNING_MSG("Unknown SysCall, %d",reg_mips[2]);
            }
            break;
        }
        break;

    case flush:
        return flush;
        break;
    case memFail:
        if (verbose==1||verbose>2) {
            WARNING_MSG("Invalid memory write");
        }
        break;

    default :
        WARNING_MSG("Unknown error - Number %d", number);
        break;
    }
    return OK;
}


int fetch(instruction* pinsIF) {	//Manque appartenance au .text    -> Probleme de pipe en fin de programme
    if(getInstr(reg_mips[PC],pinsIF)==1) {
        reg_mips[PC]+=4;
        return OK;
    } else {
        return InvalidInstruction;
    }
}


int decode(instruction insID, int* res) {
    if(insID.value==-1) return EmptyPipe; //On ne decode pas au demarrage

    int dico_entry=0;

    while((insID.value & dico_data[dico_entry].mask)!= dico_data[dico_entry].instr) {
        dico_entry++;
    }
    if (dico_entry>=nbinstr) return InvalidInstruction;
    else *res=dico_entry;
    return OK;
}

int execute(instruction insEX, pipestep EX, int dico_entry, int* tmp) {
    if (dico_entry==-1) return InvalidExecution;
    return dico_data[dico_entry].exec(insEX,EX,tmp);
}










int pipeline(uint32_t end, state running, int affichage) {
//Initialisation des variables internes
    int flag[5];
    int stall=0;

    if(verbose>3) {
        WARNING_MSG("Nouvelle iteration");
        printf("Pipe : ID %X\t EX %X\t MEM %X\t WB %X\n\n",vpipeline[ID].ins.value,vpipeline[EX].ins.value,vpipeline[MEM].ins.value,vpipeline[WB].ins.value);
    }
//Clock
    int tick= clock();


//Test memoire chargee
    if(memory==NULL) {
        WARNING_MSG("No memory loaded");
        return -1;
    }

//Test RegStall
    //printList(listUsedReg(vpipeline[WB].ins,vpipeline[WB].dico_entry));
    //printList(listUsedReg(vpipeline[EX].ins,vpipeline[EX].dico_entry));
    if(overlap(listWritedReg(vpipeline[MEM].ins,vpipeline[MEM].dico_entry),listReadedReg(vpipeline[EX].ins,vpipeline[EX].dico_entry))==1) {
        //Si les registres utilisés par WB et EX coincident
        if(verbose>4) {
            WARNING_MSG("Need to regStall");
        }
        stall=1;
    }
//Test MemStall



//Write Back
    flag[WB] = exceptionHandler(execute(vpipeline[WB].ins,vpipeline[WB].step,vpipeline[WB].dico_entry,&(vpipeline[WB].tmp)));
//Memory
    exceptionHandler(execute(vpipeline[MEM].ins,vpipeline[MEM].step,vpipeline[MEM].dico_entry,&(vpipeline[MEM].tmp)));
//Execute
    flag[EX] = exceptionHandler(execute(vpipeline[EX].ins,vpipeline[EX].step,vpipeline[EX].dico_entry,&(vpipeline[EX].tmp)));
//Decode
    //Resolution des adresses registre ?
    int dico_entry=-1;
    exceptionHandler(decode(vpipeline[ID].ins,&dico_entry));
//Fetch
    exceptionHandler(fetch(&(vpipeline[IF].ins)));
    if (stall==1) {
        reg_mips[PC]-=4;
    }

//Temporisation
    if(clocktime!=0) {
        tick=clock()-tick;
        if(verbose>0) {
            printf("Temps mis: %fms\t", (double)tick/CLOCKS_PER_SEC*1000);
            printf("Attente de %ldms\n", clocktime-tick/CLOCKS_PER_SEC*1000);
        }
        if((double)tick/CLOCKS_PER_SEC*1000<clocktime) DELAY((clocktime-tick/CLOCKS_PER_SEC*1000)*1000); //*1000 pour le passage en us->ms
    }

//Affichage
    if(verbose>3) {
        //disasm(reg_mips[PC]-4,1);
        printf("PC: %8.8X->%8.8X\t Fetched: %8.8X\n",reg_mips[PC]-4, reg_mips[PC], vpipeline[IF].ins.value);
        printf("Decoding: %8.8X  Dico: %d-> %s\n", vpipeline[ID].ins.value, dico_entry, dico_data[dico_entry].name);
        printf("Executing: %8.8X\n", vpipeline[EX].ins.value);
        printf("MEM writing: %8.8X\n", vpipeline[MEM].ins.value);
        printf("REG writing: %8.8X\n", vpipeline[WB].ins.value);
        printf("\n\n");
    }

//flush
    if(flag[EX]==flush) {
        if(verbose>4) {
            printf("*\nFlush IF\n*");
        }
        addNOP(&vpipeline[IF]);
    }
//avancement du pipeline
    pipecpy(&vpipeline[WB],vpipeline[MEM]);
    addNOP(&vpipeline[MEM]);
    if(stall==0) {
        pipecpy(&vpipeline[MEM],vpipeline[EX]);
        pipecpy(&vpipeline[EX],vpipeline[ID]);

        vpipeline[EX].tmp=0;
        vpipeline[EX].dico_entry=dico_entry;

        pipecpy(&vpipeline[ID],vpipeline[IF]);
    }
//Step

    if (running==step_return && reg_mips[PC]==return_addr) {
        printf("\nBreak\n");
        return 0;
    }
    if (running==stepinto) {
        running=stop;
    }
    if (running==step) {
        if (strcmp(dico_data[vpipeline[EX].dico_entry].name,"JAL")==0 || strcmp(dico_data[vpipeline[EX].dico_entry].name,"JALR")==0) {
            running=step_return;
            return_addr=reg_mips[PC];
            if(verbose>0) {
                printf("RETURN ADRESS SET TO %d\n",return_addr);
            }
        }
        else {
            running=stop;
        }
    }

//Gestion fin de programme
    if(reg_mips[PC]>=end+16) {
        vpipeline[ID].ins.value=-1;
    }

//Test de sortie
    if(flag[WB]==quit) {
        return 0;
    }
    if(reg_mips[PC]<textStart||reg_mips[PC]>end+16) {
        WARNING_MSG("PC out of .text, halt");
        return 0;
    }
    if (reg_mips[PC]==end+16) {
        INFO_MSG("END OF PROGRAM, NEXT STEP WILL START IT AGAIN");
        return 0;
    }
    else if(present(reg_mips[PC],breaklist)!=NULL || flag[WB]==BreakPoint || running==stop) {
        printf("\nBreak\n");
        return 0;
    }
    else {


        return pipeline(end,running,affichage);
    }
}