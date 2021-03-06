#include <stdint.h>
#include "disasm.h"
#include "define.h"
#include "fonctions.h"
#include "common/notify.h"
#include "liste.h"

int disasm(uint32_t start_addr,uint32_t size) {
    if(memory==NULL) {
        WARNING_MSG("No memory loaded");
        return -1;
    }
    if (start_addr%4!=0) WARNING_MSG("%X mod 4 != 0, read instruction may not be alligned with real instruction",start_addr);
    uint32_t i=0;
    uint32_t current_addr=start_addr;
    instruction current_instr;
    uint32_t instr_value;
    int text_ident,libctext_ident;
    int seg,k;

    //get .text scnidx in order to show only .text label
    for (k = 1; k < symtab.size; ++k) {
        if(!strcmp(symtab.sym[k].name,".text")) {
            text_ident=symtab.sym[k].scnidx;
            break;
        }
    }
    for (k = 1; k < libcsymtab.size; ++k) {
        if(!strcmp(libcsymtab.sym[k].name,".text")) {
            libctext_ident=libcsymtab.sym[k].scnidx;
            break;
        }
    }




    while (i<size) {

        seg=get_seg_from_adress(current_addr,memory);
        //printf("%d\n",seg );
        if( seg>=0 && (!strcmp(memory->seg[seg].name,".text")||!strcmp(memory->seg[seg].name,"libc.text"))) {

            getInstr(current_addr,&current_instr);
            memcpy(&instr_value,&current_instr,4);
            //Affichage des breakpoints
            if(present(current_addr,breaklist)!=NULL) {
                printf("\n   X\t");
            } else {
                printf("\n\t");
            }
            //affichage de PC
            if(current_addr==reg_mips[PC]) {
                printf("-> ");
            } else {
                printf("   ");
            }

            //Afficahge de l'adresse
            printf("%X :: %8.8X	",current_addr,instr_value);


            int dico_entry=0;
            /*for (dico_entry = 0; dico_entry < 15; ++dico_entry)
            {
            printf("instr value : %X, mask : %X, comb : %X instr : %X\n",instr_value,dico_data[dico_entry].mask,instr_value & dico_data[dico_entry].mask,dico_data[dico_entry].instr );

            }*/
            while((instr_value & dico_data[dico_entry].mask)!= dico_data[dico_entry].instr) {
                dico_entry++;
            }
            if (dico_entry>=nbinstr)
            {
                WARNING_MSG("invalid instruction at adress %X",current_addr);
                printf("\n");
                return -1;
            }
            else {


                //décommenter si on veut exécuter en disasemblant
                //dico_data[dico_entry].exec(current_instr);


                //affichage des étiquettes en début de ligne dans .text
                for (k = 1; k < symtab.size; ++k)
                {
                    if(((current_addr-memory->seg[seg].start._32)==symtab.sym[k].addr._32) && (symtab.sym[k].type != section) && ( strcmp(memory->seg[get_seg_from_adress(current_addr,memory)].name,".text")==0) && ((symtab.sym[k].scnidx == text_ident) ) ) {
                        //printf("curr-start= %d size : %d %s\n", current_addr-memory->seg[seg].start._32,symtab.sym[k].size,symtab.sym[k].name);
                        printf("%s: ",symtab.sym[k].name);
                        break;
                    }
                }
                //affichage des étiquettes en début de ligne dans libc.text
                for (k = 1; k < libcsymtab.size; ++k)
                {
                    //printf("libcsymtab scnidx=%d, libc.textident=%d\n",libcsymtab.sym[k].scnidx,libctext_ident);
                    if(((current_addr-memory->seg[seg].start._32)==libcsymtab.sym[k].addr._32) && (libcsymtab.sym[k].type != section) && ( strcmp(memory->seg[get_seg_from_adress(current_addr,memory)].name,"libc.text")==0) && ((libcsymtab.sym[k].scnidx == libctext_ident)) ) {
                        printf("%s: ",libcsymtab.sym[k].name);
                        break;
                    }
                }

                printf("%s",dico_data[dico_entry].name );
            }
            switch (dico_data[dico_entry].type) {

            case 0: 									//R TYPE

                switch (dico_data[dico_entry].nb_arg) {
                case 3:
                    //3 ARGS
                    //printf("%s, %s, %s\n",dico_data[dico_entry].argname[0],dico_data[dico_entry].argname[1],dico_data[dico_entry].argname[2]);
                    // RS RT RD
                    if(!strcmp("RS",dico_data[dico_entry].argname[0]) && !strcmp("RT",dico_data[dico_entry].argname[1]) && !strcmp("RD",dico_data[dico_entry].argname[2])) {
                        char regname1[MAX_NAME_SIZE];
                        parseReg(current_instr.r.rd,regname1);
                        char regname2[MAX_NAME_SIZE];
                        parseReg(current_instr.r.rs,regname2);
                        char regname3[MAX_NAME_SIZE];
                        parseReg(current_instr.r.rt,regname3);

                        printf(" %s, %s, %s",regname1,regname2,regname3);
                        //dico_data[dico_entry].exec(current_instr);

                    }
                    //RT RD SA
                    else if(!strcmp("RT",dico_data[dico_entry].argname[0]) && !strcmp("RD",dico_data[dico_entry].argname[1]) && !strcmp("SA",dico_data[dico_entry].argname[2])) {
                        char regname1[MAX_NAME_SIZE];
                        parseReg(current_instr.r.rd,regname1);
                        char regname2[MAX_NAME_SIZE];
                        parseReg(current_instr.r.rt,regname2);
                        char regname3[MAX_NAME_SIZE];
                        parseReg(current_instr.r.sa,regname3);

                        printf(" %s, %s, %s",regname1,regname2,regname3);

                    }
                    else {
                        WARNING_MSG("Unknown arg 1-3 for R command");
                        printf("\n");
                        return -1;
                    }
                    break;
                case 2: 	//2 arg
                    // RS RT
                    if(!strcmp("RS",dico_data[dico_entry].argname[0]) && !strcmp("RT",dico_data[dico_entry].argname[1])) {
                        char regname1[MAX_NAME_SIZE];
                        parseReg(current_instr.r.rs,regname1);
                        char regname2[MAX_NAME_SIZE];
                        parseReg(current_instr.r.rt,regname2);

                        printf(" %s, %s",regname1,regname2);

                    }
                    else if(!strcmp("RD",dico_data[dico_entry].argname[0]) && !strcmp("RS",dico_data[dico_entry].argname[1])) {
                        char regname1[MAX_NAME_SIZE];
                        parseReg(current_instr.r.rd,regname1);
                        char regname2[MAX_NAME_SIZE];
                        parseReg(current_instr.r.rs,regname2);

                        printf(" %s, %s",regname1,regname2);

                    }
                    else if(!strcmp("RT",dico_data[dico_entry].argname[0]) && !strcmp("RD",dico_data[dico_entry].argname[1])) {
                        char regname1[MAX_NAME_SIZE];
                        parseReg(current_instr.r.rd,regname1);
                        char regname2[MAX_NAME_SIZE];
                        parseReg(current_instr.r.rt,regname2);

                        printf(" %s, %s",regname1,regname2);

                    }
                    else {
                        WARNING_MSG("Unknown arg 1 or 2 for R command");
                        printf("\n");
                        return -1;
                    }

                    break;

                case 1:
                    if(!strcmp("RS",dico_data[dico_entry].argname[0])) {
                        char regname1[MAX_NAME_SIZE];
                        parseReg(current_instr.r.rs,regname1);
                        printf(" %s",regname1);
                    }
                    else if(!strcmp("RD",dico_data[dico_entry].argname[0])) {
                        char regname1[MAX_NAME_SIZE];
                        parseReg(current_instr.r.rd,regname1);
                        printf(" %s",regname1);
                    }
                    else {
                        WARNING_MSG("Unknown arg for R command");
                        printf("\n");
                        return -1;
                    }

                    break;

                case 0:

                    break;

                default:
                    WARNING_MSG("Unknown R command");
                    printf("\n");
                    return -1;
                    break;
                }
                break;


            case 1:
                //i type
                switch (dico_data[dico_entry].nb_arg) {
                case 3:
                    //3arg
                    if(!strcmp("RS",dico_data[dico_entry].argname[0]) && !strcmp("RT",dico_data[dico_entry].argname[1]) && !strcmp("IM",dico_data[dico_entry].argname[2])) {
                        char regname1[MAX_NAME_SIZE];
                        parseReg(current_instr.i.rt,regname1);
                        char regname2[MAX_NAME_SIZE];
                        parseReg(current_instr.i.rs,regname2);

                        printf(" %s, %s, %d",regname1,regname2,current_instr.i.immediate);

                    }
                    else if(!strcmp("RS",dico_data[dico_entry].argname[0]) && !strcmp("RT",dico_data[dico_entry].argname[1]) && !strcmp("OFFSET",dico_data[dico_entry].argname[2])) {
                        char regname1[MAX_NAME_SIZE];
                        parseReg(current_instr.i.rs,regname1);
                        char regname2[MAX_NAME_SIZE];
                        parseReg(current_instr.i.rt,regname2);

                        printf(" %s, %s, %d",regname1,regname2,4*current_instr.i.immediate);

                        for (k = 1; k < symtab.size; ++k) {
                            if(((current_addr+ 4 + 4*current_instr.i.immediate-memory->seg[seg].start._32)==symtab.sym[k].addr._32)&&(symtab.sym[k].type != section)&& (symtab.sym[k].scnidx == text_ident) && ( strcmp(memory->seg[get_seg_from_adress(current_addr,memory)].name,".text")==0)) {
                                printf(" <%s>",symtab.sym[k].name);
                                break;
                            }
                        }
                        for (k = 1; k < libcsymtab.size; ++k) {
                            if(((current_addr+ 4 + 4*current_instr.i.immediate-memory->seg[seg].start._32)==libcsymtab.sym[k].addr._32)&&(libcsymtab.sym[k].type != section)&& (libcsymtab.sym[k].scnidx == libctext_ident) && ( strcmp(memory->seg[get_seg_from_adress(current_addr,memory)].name,"libc.text")==0)) {
                                printf(" <%s>",libcsymtab.sym[k].name);
                                break;
                            }
                        }
                    }
                    else if(!strcmp("BASE",dico_data[dico_entry].argname[0]) && !strcmp("RT",dico_data[dico_entry].argname[1]) && !strcmp("OFFSET",dico_data[dico_entry].argname[2])) {
                        char regname1[MAX_NAME_SIZE];
                        parseReg(current_instr.i.rt,regname1);
                        char regname2[MAX_NAME_SIZE];
                        parseReg(current_instr.i.rs,regname2);
                        //BASE est a l'addresse de rs
                        printf(" %s, %d(%s)",regname1,current_instr.i.immediate,regname2);

                    }
                    else {
                        WARNING_MSG("Unknown arg 1-3 for I command : %s %s %s", dico_data[dico_entry].argname[0],dico_data[dico_entry].argname[1],dico_data[dico_entry].argname[2]);
                        printf("\n");
                        return -1;
                    }

                    break;
                case 2:
                    if(!strcmp("RS",dico_data[dico_entry].argname[0]) && !strcmp("OFFSET",dico_data[dico_entry].argname[1])) {
                        char regname1[MAX_NAME_SIZE];
                        parseReg(current_instr.i.rs,regname1);

                        printf(" %s, %d",regname1,4*current_instr.i.immediate);

                        for (k = 1; k < symtab.size; ++k) {
                            if(((current_addr+ 4 + 4*current_instr.i.immediate-memory->seg[seg].start._32)==symtab.sym[k].addr._32)&&(symtab.sym[k].type != section)&& (symtab.sym[k].scnidx == text_ident) && ( strcmp(memory->seg[get_seg_from_adress(current_addr,memory)].name,".text")==0) ) {
                                printf(" <%s>",symtab.sym[k].name);
                                break;
                            }
                        }
                        for (k = 1; k < libcsymtab.size; ++k) {
                            if(((current_addr+ 4 + 4*current_instr.i.immediate-memory->seg[seg].start._32)==libcsymtab.sym[k].addr._32)&&(libcsymtab.sym[k].type != section)&& (libcsymtab.sym[k].scnidx == text_ident) && ( strcmp(memory->seg[get_seg_from_adress(current_addr,memory)].name,"libc.text")==0) ) {
                                printf(" <%s>",libcsymtab.sym[k].name);
                                break;
                            }
                        }

                    }
                    else if(!strcmp("RT",dico_data[dico_entry].argname[0]) && !strcmp("IM",dico_data[dico_entry].argname[1])) {
                        char regname1[MAX_NAME_SIZE];
                        parseReg(current_instr.i.rt,regname1);

                        printf(" %s, %d",regname1,current_instr.i.immediate);

                    }
                    else {
                        WARNING_MSG("Unknown arg 1 or 2 for I command");
                        printf("\n");
                        return -1;
                    }

                    break;
                default:
                    WARNING_MSG("Wrong arg number for I command");
                    printf("\n");
                    return -1;
                    break;


                }
                break;

            case 2:
                //type J
                if(!strcmp("TARGET",dico_data[dico_entry].argname[0])) {



                    printf(" 0x%4.8X",((current_addr & 0xF0000000) | 4*current_instr.j.target));
                    int32_t temp=((current_addr & 0xF0000000) | 4*current_instr.j.target);
                    for (k = 1; k < symtab.size; ++k) {
                        if(((temp-textStart) == symtab.sym[k].addr._32) && (symtab.sym[k].type != section) && (symtab.sym[k].scnidx == text_ident) && ( strcmp(memory->seg[get_seg_from_adress(current_addr,memory)].name,".text")==0) ) {
                            printf(" <%s>",symtab.sym[k].name);
                            break;
                        }
                    }
                    //affichage des étiquettes dans la libc
                    for (k = 1; k < libcsymtab.size; ++k) {
                        //printf("\nsymb adress : %d, total=%X,addr=%X",libcsymtab.sym[k].addr._32,((current_addr & 0xF0000000) | 4*current_instr.j.target-libcTextStart),((current_addr & 0xF0000000) | 4*current_instr.j.target));
                        //printf("addr %X\n",libcTextStart);
                        if(((temp - libcTextStart) == libcsymtab.sym[k].addr._32) && (libcsymtab.sym[k].type != section) && (libcsymtab.sym[k].scnidx == text_ident)&& ( strcmp(memory->seg[get_seg_from_adress(current_addr,memory)].name,"libc.text")==0)) {
                            printf(" <%s>",libcsymtab.sym[k].name);
                            break;
                        }
                    }


                }
                else {
                    WARNING_MSG("Unknown J command");
                    printf("\n");
                    return -1;
                }

                break;

            default:

                WARNING_MSG("Unknown type");
                printf("\n");
                return -1;
                break;

            }
        }
        current_addr+=4;
        i+=4;
    }
    printf("\n");
    return 0;
}
