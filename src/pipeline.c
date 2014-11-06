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

int exceptionHandler(exception number) {
    //Permet de parser les erreurs et exceptions
    switch(number) {
    case OK:
        break;

    case EmptyPipe:
        WARNING_MSG("Pipeline vide");
        break;


    case InvalidInstruction:
        WARNING_MSG("Invalid instruction at adress %X",reg_mips[PC]);
        break;

    case InvalidExecution:
        WARNING_MSG("Invalid execution : %8.8X at adress %X", insEX.value, reg_mips[PC]-8);
        break;



    case IntegerOverflow:
        WARNING_MSG("IntegerOverflow : %8.8X",insEX.value);
        break;

    case BreakPoint:
        WARNING_MSG("Break : %8.8X",insEX.value);
        return BreakPoint;
        break;

    case SysCall:
        WARNING_MSG("Syscall : %8.8X",insEX.value);
        switch (reg_mips[2]) { //v0
        case 1:
            printf("%d\n",reg_mips[4]); //a0
            break;
        case 4:
            //printf("%s\n",reg_mips[4]);
            break;
        case 5:
            scanf("%d",&reg_mips[2]);
            break;
        case 8:
            //scanf("%s",&reg_mips[2]);
            //reg_mips[3]=sizeof
            break;
        case 10:
            return quit;
            break;
        default:
            WARNING_MSG("Unknown SysCall, %d",reg_mips[2]);
            break;
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
    int flag;
    if(affichage==1) {
        WARNING_MSG("Nouvelle iteration");
        printf("Pipe : ID %X\t EX %X\t MEM %X\t WB %X\n\n",insID.value,insEX.value,insMEM.value,insWB.value);
    }
//Clock
    int tick= clock();


//Test memoire chargee
    if(memory==NULL) {
        WARNING_MSG("No memory loaded");
        return -1;
    }

//Fetch
    instruction insIF; //Creation de la nouvelle instruction
    exceptionHandler(fetch(&insIF));

//Decode
    //Resolution des adresses registre ?
    int dico_entry=-1;
    exceptionHandler(decode(insID,&dico_entry));


//Execute
    flag = exceptionHandler(execute(insEX,EX,dico_entry,&EXtmp));

//Memory
    exceptionHandler(execute(insMEM,MEM,dico_entry,&MEMtmp));

//Write Back
    exceptionHandler(execute(insWB,WB,dico_entry,&WBtmp));


//Temporisation
    if(clocktime!=0) {
        tick=clock()-tick;
        printf("Temps mis: %fms\t", (double)tick/CLOCKS_PER_SEC*1000);
        printf("Attente de %ldms\n", clocktime-tick/CLOCKS_PER_SEC*1000);
        if((double)tick/CLOCKS_PER_SEC*1000<clocktime) DELAY((clocktime-tick/CLOCKS_PER_SEC*1000)*1000); //*1000 pour le passage en us->ms
    }
//avancement du pipeline
    insWB=insMEM;
    WBtmp=MEMtmp;
    insMEM=insEX;
    MEMtmp=EXtmp;
    insEX=insID;
    EXtmp=0;
    insID=insIF;



//Gestion fin de programme
    if(reg_mips[PC]>=end) {
        insID.value=-1;
    }
//Affichage
    if(affichage==1) {
        //disasm(reg_mips[PC]-4,1);
        printf("\nPC: %8.8X->%8.8X\t Fetched: %8.8X\n",reg_mips[PC]-4, reg_mips[PC], insIF.value);
        printf("Decoding: %8.8X  Dico: %d-> %s\n", insID.value, dico_entry, dico_data[dico_entry].name);
        printf("Executing: %8.8X\n", insEX.value);
    }
//Test de sortie
    if(flag==quit){initprog();return 0;}
    if(reg_mips[PC]>=end+16 || present(reg_mips[PC],breaklist)!=NULL || flag==BreakPoint) {
        printf("\nFin du run\n");
        return 0;
    } else {


        return pipeline(end,running,affichage);
    }
}