#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "filtrar.h"

/* Este filtro deja pasar los caracteres NO alfabeticos. */
/* Devuelve el numero de caracteres que han pasado el filtro. */
int tratar(char* buff_in, char* buff_out, int tam)
{
	int o = 0; //Contador del número de caracteres
	int i=0; //Índice de buff_in 
	int j=0; //Índice de buff_out
	while(buff_in[i]){
		if(!isalpha(buff_in[i])){
			buff_out[j++]=buff_in[i];
			o++;
		}
		i++;
	} 
	return o;
}
