#ifndef _user_int_h
#define _user_int_h

extern int scriptmode;

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
    UNKNOWN
} command ;


int decrypt(char input []);//doit decrypter mot par mot, faudrait une fonction get-word qui ne renvoi rien si c'est des comment




#endif
