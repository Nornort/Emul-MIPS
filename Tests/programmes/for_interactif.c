#include <stdio.h>

int tri_tab(int * tab,int nbelement );
int main(int argc, char const *argv[])
{
	int tabnotes[10];
	int note=0,tot=0,i=0,j;
	int moyenne;
	printf("Entrez des notes (moins de 10,<0 pour terminer) :\n");
	for (i=0;i<9;i++){
		scanf("%d",&note);
		if (note>0){
			printf("Vous avez entré %d\n",note );
			tabnotes[i]=note;
			tot=tot+note;
		}
	}
	moyenne=tot/(i);
	printf("Votre moyenne est : %d\n",moyenne);
	tri_tab(tabnotes,i-1);
	printf("notes triées : \n");
	for (j = 0; j < i; ++j)
	{
		printf(" %d \n",tabnotes[j] );
	}
	return 0;
}


int tri_tab(int * tab,int nbelement ){
	int i=0,temp;
	while(i<nbelement){
		if (tab[i+1]<tab[i])
		{
			temp=tab[i+1];
			tab[i+1]=tab[i];
			tab[i]=temp;

			i=0;
		}
		else i++;
	}
	return 0;
}
