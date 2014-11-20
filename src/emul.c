//Base de la machine virtuelle et chargement d'un fichier ELF




#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include "define.h"
#include "common/bits.h"
#include "common/notify.h"
#include "elf/elf.h"
#include "elf/syms.h"
#include "mem.h"
#include "emul.h"
#include "fonctions.h"
#include "elf/relocator.h"




//init des registres RAJOUTER HI LOW PC
int reg_mips[35];

//init de la mémoire
mem memory=NULL;
stab symtab;

//init du dictionnaire
dico_info* dico_data=NULL;




// On fixe ici une adresse basse dans la mémoire virtuelle. Le premier segment
// ira se loger à cette adresse.
// nombre max de sections que l'on extraira du fichier ELF
#define NB_SECTIONS 4

// nom de chaque section
#define TEXT_SECTION_STR ".text"
#define RODATA_SECTION_STR ".rodata"
#define DATA_SECTION_STR ".data"
#define BSS_SECTION_STR ".bss"

//nom du prefix à appliquer pour la section
#define RELOC_PREFIX_STR ".rel"

int is_in_symbols(char* name, stab symtab) {
    int i;
    for (i=0; i<symtab.size; i++) {
        if (!strcmp(symtab.sym[i].name,name)) return 1;
    }
    return 0;
}

// Cette fonction calcule le nombre de segments à prevoir
// Elle cherche dans les symboles si les sections predefinies
// s'y trouve
// parametres :
//  symtab : la table des symboles
//
// retourne le nombre de sections trouvées

unsigned int get_nsegments(stab symtab,char* section_names[],int nb_sections) {
    unsigned int n=0;
    int i;
    for (i=0; i<nb_sections; i++) {
        if (is_in_symbols(section_names[i],symtab)) n++;
    }
    return n;
}



int elf_load_section_in_memory(FILE* fp, mem memory, char* scn,unsigned int permissions,unsigned long long add_start) {
    byte *ehdr    = __elf_get_ehdr( fp );
    byte *content = NULL;
    uint  textsz  = 0;
    vsize sz;
    vaddr addr;

    byte *useless = elf_extract_section_header_table( ehdr, fp );
    free( useless );

    if ( NULL == ehdr ) {
        WARNING_MSG( "Can't read ELF file" );
        return 1;
    }

    if ( 1 == attach_scn_to_mem(memory, scn, SCN_ATTR( WIDTH_FROM_EHDR( ehdr ), permissions ) ) ) {
        WARNING_MSG( "Unable to create %s section", scn );
        free( ehdr );
        return 1;
    }

    content = elf_extract_scn_by_name( ehdr, fp, scn, &textsz, NULL );
    if ( NULL == content ) {
        WARNING_MSG( "Corrupted ELF file" );
        free( ehdr );
        return 1;
    }

    switch( WIDTH_FROM_EHDR(ehdr) ) {
    case 32 :
        sz._32   = textsz/*+8*/; /* +8: In case adding a final sys_exit is needed */
        addr._32 = add_start;
        break;
    case 64 :
        sz._64   = textsz/*+8*/; /* +8: In case adding a final sys_exit is needed */
        addr._64 = add_start;
        break;
    default :
        WARNING_MSG( "Wrong machine width" );
        return 1;
    }

    if ( 1 == fill_mem_scn(memory, scn, sz, addr, content ) ) {
        free( ehdr );
        free( content );
        WARNING_MSG( "Unable to fill in %s segment", scn );
        return 1;
    }

    free( content );
    free( ehdr );

    return 0;
}


/*--------------------------------------------------------------------------  */
/**
 * @param fp le fichier elf original
 * @param seg le segment à reloger
 * @param mem l'ensemble des segments
 *
 * @brief Cette fonction effectue la relocation du segment passé en parametres
 * @brief l'ensemble des segments doit déjà avoir été chargé en memoire.
 *
 * VOUS DEVEZ COMPLETER CETTE FONCTION POUR METTRE EN OEUVRE LA RELOCATION !!
 */
void reloc_segment(FILE* fp, segment seg, mem memory,unsigned int endianness,stab symtab) {
    byte *ehdr    = __elf_get_ehdr( fp );
    uint32_t  scnsz  = 0;
    Elf32_Rel *rel = NULL;
    char* reloc_name = malloc(strlen(seg.name)+strlen(RELOC_PREFIX_STR)+1);
    scntab section_tab;

    // on recompose le nom de la section
    memcpy(reloc_name,RELOC_PREFIX_STR,strlen(RELOC_PREFIX_STR)+1);
    strcat(reloc_name,seg.name);

    // on récupere le tableau de relocation et la table des sections
    rel = (Elf32_Rel *)elf_extract_scn_by_name( ehdr, fp, reloc_name, &scnsz, NULL );
    elf_load_scntab(fp ,32, &section_tab);


    if (rel != NULL &&seg.content!=NULL && seg.size._32!=0) {

        INFO_MSG("--------------Relocation de %s-------------------\n",seg.name) ;
        INFO_MSG("Nombre de symboles a reloger: %ld\n",scnsz/sizeof(*rel)) ;


        //------------------------------------------------------

        int i=0;
        uint sym;
        uint type;
        uint info;
        uint offset;
        //display :

        //printf("scnsz=%d\n", scnsz);
        //print_scntab(section_tab);
        printf("Offset    Info      Type            Sym.Value  Sym.Name\n");
        while(i<scnsz/sizeof(*rel)) {
            info=rel[i].r_info;
            offset=rel[i].r_offset;
            FLIP_ENDIANNESS(info) ;
            FLIP_ENDIANNESS(offset) ;
            sym=ELF32_R_SYM(info);
            type=ELF32_R_TYPE(info);
            if (type>32) {
                WARNING_MSG("Unknown type : %d",type);
            }
            else {
                printf("%08X  %08X  %-14s  %08X   %s\n",offset,info,MIPS32_REL[type],sym,symtab.sym[sym].name);
                i++;
            }

        }

        i=0;
        //------------------------------------------------------
        //Reloc :
        int A,V,P;
        //int segnum;
        uint32_t S;
        while(i<scnsz/sizeof(*rel)) {
            info=rel[i].r_info;
            offset=rel[i].r_offset;
            FLIP_ENDIANNESS(info) ;
            FLIP_ENDIANNESS(offset) ;
            sym=ELF32_R_SYM(info);
            type=ELF32_R_TYPE(info);
            //segnum=seg_from_scnidx(symtab.sym[sym].scnidx,symtab,memory);
            //if(segnum==-1){
            //    WARNING_MSG("Couldn't resolve scndix correspondance");
            //    break;
            //}
            //S=memory->seg[segnum].start._32+symtab.sym[sym].addr._32;//a vérif
            if(addr_from_symnb(sym, symtab, memory,&S)==-1){WARNING_MSG("Couldn't resolve scndix correspondance");break;}
            P=seg.start._32+offset;
            memRead(P,1,&A);
            //printf("Relocation type %s\n",MIPS32_REL[type] );
            switch (type) {
            case 2:
            V=S+A;

            //printf("V= %X,S=%X,A=%X,P=%X\n",V,S,A,P);
            memWrite(P,1,V);
                break;
            case 4:
            V=((A<<2)|((P&0xF0000000)+S))>>2;

            //printf("V= %X,S=%X,A=%X,P=%X\n",V,S,A,P);
            memWrite(P,1,V);
                break;
            case 5:;
            uint nexttype=rel[i+1].r_info;
            uint nextoffset=rel[i+1].r_offset;
            FLIP_ENDIANNESS(nexttype);
            FLIP_ENDIANNESS(nextoffset);
            nexttype=ELF32_R_TYPE(nexttype);
            if(nexttype!=6){WARNING_MSG("R_MIPS_HI16 not followed by R_MIIPS_LO16 : %s",MIPS32_REL[nexttype]);}
            else{

                int P2=seg.start._32+nextoffset,A2;
                memRead(P2,1,&A2);
                int AHL;
                AHL=(A<<16)+(short)(A2);
                //printf("A2=%X short A2=%X\n",A2, (short)A2 );
                //printf("AHL : %X\n",AHL );
                //printf("Total=%X AHL+S=%X, short=%X, diff=%X\n",((AHL+S-(short)AHL+S)>>16),AHL+S,(short)AHL+S,AHL+S-(short)AHL+S) ;
                V=(A & 0xFFFF0000)|((AHL+S-(short)AHL+S)>>16);

                //printf("V= %X,S=%X,A=%X,A2=%X,P=%X,P2=%X, AHL=%X\n",V,S,A,A2,P,P2,AHL);
                memWrite(P,1,V);
             }
                break;
            case 6:;
                int previoustype=rel[i-1].r_info;
                int previousoffset=rel[i-1].r_offset;
                FLIP_ENDIANNESS(previoustype);
                FLIP_ENDIANNESS(previousoffset);
                previoustype=ELF32_R_TYPE(previoustype);
                if(previoustype!=5){WARNING_MSG("R_MIPS_LO16 not preceded by R_MIIPS_HI16 : %s",MIPS32_REL[previoustype]);}
                else{

                    int32_t P2=seg.start._32+previousoffset,A2;
                    memRead(P2,1,&A2);
                    int32_t AHL=(A2<<16)+(short)(A);
                    V=(A&0xFFFF0000)|(short)(AHL+S);

                    //printf("V= %X,S=%X,A=%X,P=%X\n",V,S,A,P);
                    memWrite(P,1,V);
                 }
                break;
            default:
                if (type>32) {
                    WARNING_MSG("Unknown type : %d, relocation impossible for element %d",type,i);
                }
                else {
                    WARNING_MSG("Unknown type : %s(code : %d), relocation impossible for element %d",MIPS32_REL[type],type,i);
                }
                break;
            }
            i++;
        }
        //------------------------------------------------------

    }
    del_scntab(section_tab);
    free( rel );
    free( reloc_name );
    free( ehdr );

}












int memRead(uint32_t start_addr,int type, int* value) {             //Lit la memoire et stock dans value
    if(memory==NULL) {
        WARNING_MSG("No memory loaded");
        return -1;
    }
    int j=0;

    while(start_addr>=memory->seg[j].start._32 && j < memory->nseg)
    {
        j++;
    }
//printf("%d, current addr = 0x%X, start addr = 0x%X, size = %d",j,start_addr,memory->seg[j-1].start._32,memory->seg[j-1].size._32);
    if(type==0) { //type 0 pour byte
        if(j>0 && (start_addr < memory->seg[j-1].start._32+memory->seg[j-1].size._32)) {
            *value=memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32];
            // printf(" \nécriture\n");

        }
        else {
            return -1;
        }
    } else {
        if (j>0 && (start_addr < memory->seg[j-1].start._32+memory->seg[j-1].size._32) && (start_addr+3 < memory->seg[j-1].start._32+memory->seg[j-1].size._32))
        {
            struct_word temp; //copier directement le tableau dans le utint ?
            temp.b1=memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32];
            temp.b2=memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32+1];
            temp.b3=memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32+2];
            temp.b4=memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32+3];
            memcpy(value,&temp,4);
        }
        else {
            return -1;
        }
    }
    //printf("0x%x\n",*value );
    return 0;
}

int memWrite(uint32_t start_addr,int type, int32_t value) {         // Ecrit value dans la memoire
    if(memory==NULL) {
        WARNING_MSG("No memory loaded");
        return -1;
    }
    int j=0;

    while(start_addr>=memory->seg[j].start._32 && j < memory->nseg)
    {
        j++;
    }

    if(type==0) {
        if(j>0 && (start_addr < memory->seg[j-1].start._32+memory->seg[j-1].size._32)) {
            memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32]=value;
        }
        else {
            return -1;
        }
    } else {
        if (j>0 && (start_addr < memory->seg[j-1].start._32+memory->seg[j-1].size._32) && (start_addr+3 < memory->seg[j-1].start._32+memory->seg[j-1].size._32))
        {
            struct_word temp;
            memcpy(&temp,&value,4); //copier directement le uint dans le tableau ?
            memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32]=temp.b1;
            memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32+1]=temp.b2;
            memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32+2]=temp.b3;
            memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32+3]=temp.b4;
        }
        else {
            return -1;
        }
    }
    return 0;
}

int memWriteChecked(uint32_t start_addr,int type, int32_t value) {         // Ecrit value dans la memoire
    if(memory==NULL) {
        WARNING_MSG("No memory loaded");
        return -1;
    }
    int j=0;

    while(start_addr>=memory->seg[j].start._32 && j < memory->nseg)
    {
        j++;
    }

    if(type==0) {
        if(j>0 && (start_addr < memory->seg[j-1].start._32+memory->seg[j-1].size._32) && (memory->seg[j-1].attr==2 || memory->seg[j-1].attr==7 )) {
            memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32]=value;
        }
        else {
            return -1;
        }
    } else {
        if (j>0 && (start_addr < memory->seg[j-1].start._32+memory->seg[j-1].size._32) && (start_addr+3 < memory->seg[j-1].start._32+memory->seg[j-1].size._32) && (memory->seg[j-1].attr==2 || memory->seg[j-1].attr==7 ))
        {
            struct_word temp;
            memcpy(&temp,&value,4); //copier directement le uint dans le tableau ?
            memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32]=temp.b1;
            memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32+1]=temp.b2;
            memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32+2]=temp.b3;
            memory->seg[j-1].content[start_addr-memory->seg[j-1].start._32+3]=temp.b4;
        }
        else {
            return -1;
        }
    }
    return 0;
}




void print_segment_raw_content(segment* seg) {      //Affiche un segment donné
    int k;
    int word =0;
    if (seg!=NULL && seg->size._32>0) {
        for(k=0; k<seg->size._32; k+=4) {
            if(k%16==0) printf("\n  0x%08x ",k);
            word = *((unsigned int *) (seg->content+k));
            FLIP_ENDIANNESS(word);
            printf("%08x ",	word);
        }
    }
}


int loadELF (char* name,int nbparam,...) {
    va_list ap;
    va_start(ap, nbparam);
    if(nbparam>1) {
        va_arg(ap, uint32_t); //dirty
        textStart = va_arg(ap, uint32_t);
    }
    else {
        textStart=DEFAULT_S_ADDR;
    }
    if(textStart%0x1000>0) {
        textStart = textStart+0x1000-textStart%0x1000;
    }

    va_end(ap);

    char* section_names[NB_SECTIONS]= {TEXT_SECTION_STR,RODATA_SECTION_STR,DATA_SECTION_STR,BSS_SECTION_STR};
    unsigned int segment_permissions[NB_SECTIONS]= {R_X,R__,RW_,RW_};
    unsigned int nsegments;
    int i=0,j=0;
    unsigned int type_machine;
    unsigned int endianness;   //little ou big endian
    unsigned int bus_width;    // 32 bits ou 64bits
    unsigned int next_segment_start = textStart; // compteur pour designer le début de la prochaine section

    symtab=new_stab(0);

    FILE * pf_elf;


    if ((pf_elf = fopen(name,"r")) == NULL) {
        WARNING_MSG("cannot open file '%s'", name);
        return -1;
    }

    if (!assert_elf_file(pf_elf)) {
        WARNING_MSG("file %s is not an ELF file", name);
        return -1;
    }

    // recuperation des info de l'architecture
    elf_get_arch_info(pf_elf, &type_machine, &endianness, &bus_width);
    // et des symboles
    elf_load_symtab(pf_elf, bus_width, endianness, &symtab);


    nsegments = get_nsegments(symtab,section_names,NB_SECTIONS);

    // allouer la memoire virtuelle
    memory=init_mem(nsegments);

    // Ne pas oublier d'allouer les differentes sections
    j=0;
    for (i=0; i<NB_SECTIONS; i++) {
        if (is_in_symbols(section_names[i],symtab)) {
            elf_load_section_in_memory(pf_elf,memory, section_names[i],segment_permissions[i],next_segment_start);
            next_segment_start+= ((memory->seg[j].size._32+0x1000)>>12 )<<12; // on arrondit au 1k suppérieur
//            print_segment_raw_content(&memory->seg[j]);
            j++;
        }
    }

    for (i=0; i<nsegments; i++) {
        reloc_segment(pf_elf, memory->seg[i], memory,endianness,symtab);

    }

    //TODO allouer la pile (et donc modifier le nb de segments)

    // printf("\n------ Fichier ELF \"%s\" : sections lues lors du chargement ------\n", name) ;
    // print_mem(memory);
    //stab32_print( symtab);

    // on fait le ménage avant de partir
    //del_mem(memory);
    //del_stab(symtab);

    initprog();



    INFO_MSG("Loading file :'%s'",name);
    fclose(pf_elf);
    //puts("");
    return 0;
}


int dispmemPlage(uint32_t start_addr,uint32_t size) {           //Affiche une plage memoire
    if(memory==NULL) {
        WARNING_MSG("No memory loaded");
        return -1;
    }
    uint32_t i=0;
    uint32_t current_addr=start_addr;
    int value;
    while (i<size) {
        if (i%16==0) {
            printf("\n0x%8.8X ",current_addr);
        }


//    printf("j= %d i=%d, current_addr= %d ,start= %d, size = %d, diff=%d \n",j,i,current_addr,memory->seg[j-1].start._32,memory->seg[j-1].size._32,current_addr-memory->seg[j-1].start._32+memory->seg[j-1].size._32);
        if(memRead(current_addr,0,&value)==0) { //on vérifie qu'il soit dans une plage mémoire valide
            printf("%2.2X ",value);
        }
        else printf("XX ");

        current_addr++;
        i++;
    }
    printf("\n");
    return 0;
}

