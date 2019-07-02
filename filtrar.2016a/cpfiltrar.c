#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include "filtrar.h"

#define BUFSIZE 4096
/* ---------------- PROTOTIPOS ----------------- */ 
/* Esta funcion monta el filtro indicado y busca el simbolo "tratar"
   que debe contener, y aplica dicha funcion "tratar()" para filtrar
   toda la informacion que le llega por su entrada estandar antes
   de enviarla hacia su salida estandar. */
extern void filtrar_con_filtro(char* nombre_filtro);

/* Esta funcion lanza todos los procesos necesarios para ejecutar los filtros.
   Dichos procesos tendran que tener redirigida su entrada y su salida. */
void preparar_filtros(void);

/* Esta funcion recorrera el directorio pasado como argumento y por cada entrada
   que no sea un directorio o cuyo nombre comience por un punto '.' la lee y 
   la escribe por la salida estandar (que seria redirigida al primero de los 
   filtros, si existe). */
void recorrer_directorio(char* nombre_dir);

/* Esta funcion recorre los procesos arrancados para ejecutar los filtros, 
   esperando a su terminacion y recogiendo su estado de terminacion. */ 
void esperar_terminacion(void);

/* Desarrolle una funcion que permita controlar la temporizacion de la ejecucion
   de los filtros. */ 
extern void preparar_alarma(void);


/* ---------------- IMPLEMENTACIONES ----------------- */ 
char** filtros;   /* Lista de nombres de los filtros a aplicar */
int    n_filtros; /* Tama~no de dicha lista */
pid_t* pids;      /* Lista de los PIDs de los procesos que ejecutan los filtros */



/* Funcion principal */
int main(int argc, char* argv[]){
	/* Chequeo de argumentos */
	if(argc < 2){ 

		/* Invocacion sin argumentos  o con un numero de argumentos insuficiente */
		fprintf(stderr,"Uso: %s directorio [filtro...]\n",argv[0]);
		exit(1);
	}

	filtros = &(argv[2]);                             /* Lista de filtros a aplicar */
	n_filtros = argc-2;                               /* Numero de filtros a usar */
	pids = (pid_t*)malloc(sizeof(pid_t)*n_filtros);   /* Lista de pids */

	preparar_alarma();

	preparar_filtros();

	recorrer_directorio(argv[1]);

	esperar_terminacion();

	return 0;
}


void recorrer_directorio(char* nombre_dir){
	DIR* dir = NULL;
	struct dirent* ent;
	char fich[1024];
	char buff[4096];
	int fd;
	int total;
	struct stat atributos;
	struct sigaction act;
	act.sa_handler = SIG_IGN;
	/* Abrir el directorio */
	dir = opendir(nombre_dir);
	/* Tratamiento del error. */
	if(dir==NULL){
		fprintf(stderr,"Error al abrir el directorio '%s'\n",nombre_dir);
		exit(1);
	}

	/* Recorremos las entradas del directorio */
	while((ent=readdir(dir))!=NULL)
	{
		
		/* Nos saltamos las entradas que comienzan por un punto "." */
		if(ent->d_name[0]=='.')
			continue;

		/* fich debe contener la ruta completa al fichero */
		strcpy(fich,nombre_dir);
		strcat(fich,"/");
		strcat(fich,ent->d_name);

		if(stat(fich,&atributos)<0){
			fprintf(stderr,"AVISO: No se puede stat el fichero '%s'!\n",fich);
			break;	
		}
		/* Nos saltamos las rutas que sean directorios. */	
		if(S_ISDIR(atributos.st_mode)) continue;
		/* Abrir el archivo. */
		fd = open(fich,O_RDONLY);		
		/* Tratamiento del error. */
		if(fd<0){
			fprintf(stderr,"AVISO: No se puede abrir el fichero '%s'!\n",fich);
			break;
		}
		/* Cuidado con escribir en un pipe sin lectores! */
		sigaction(SIGPIPE,&act,NULL);
		/* Emitimos el contenido del archivo por la salida estandar. */
		while((total=write(1,buff,read(fd,buff,4096))) > 0){ // Está petando aquí!!!!!
			if(errno==EPIPE){
				break;
			}
			continue;
		}
		if(total<0 && errno!=EPIPE){
			fprintf(stderr,"Error al emitir el fichero '%s'\n",fich);
			exit(1);
		
		}else{
			/* Cerrar. */
			close(fd); //Cierra el fichero
		}

		
	}
	if(errno!=0 && ent==NULL && errno !=EPIPE){
		fprintf(stderr,"Error al leer el directorio '%s'\n",nombre_dir);
		exit(1);
	}
	/* Cerrar. */
	closedir(dir);//Cierra el directorio
	
	/* IMPORTANTE:
	 * Para que los lectores del pipe puedan terminar
	 * no deben quedar escritores al otro extremo. */
	// IMPORTANTE
	close(1);
}

	
void preparar_filtros(void)
{
	int p;
	int pp[2];	
	int i;
	for (i=n_filtros-1;i>=0;i--){	
		/* Tuberia hacia el hijo (que es el proceso que filtra). */	
		if((pipe(pp)<0)){ // Creamos el pipe
			fprintf(stderr,"Error al crear el pipe\n");
			exit(1);
		}
		/* Lanzar nuevo proceso */
		p=fork();//Creamos un proceso hijo
		switch(p){

			case -1:
				/* Error. Mostrar y terminar. */
				fprintf(stderr,"Error al crear proceso %d\n",p);
				exit(1);
			case  0:
				/* Hijo: Redireccion y Ejecuta el filtro. */
				close(pp[1]);
				dup2(pp[0],0);
				close(pp[0]);
				int v,v2; // Variables que se van a usar para sacar la longitud de extensión de fichero
				char * extF; //Variable para almacenar la extensión de fichero, si la hay
				extF = strstr(filtros[i],".so");
				if (extF!=NULL){/* El nombre termina en ".so" ? */
					v = strlen(extF);
					v2 = strlen(".so");
					if(v==v2){
					/* SI. Montar biblioteca y utilizar filtro. */
						filtrar_con_filtro(filtros[i]);
						exit(0);
					}else{	/* NO. Ejecutar como mandato estandar. */
						execlp(filtros[i],filtros[i],NULL);
						fprintf(stderr,"Error al ejecutar el mandato '%s'\n",filtros[i]);
						exit(1);
						break;
					}
				}else{ /* NO. Ejecutar como mandato estandar. */
					execlp(filtros[i],filtros[i],NULL);
					fprintf(stderr,"Error al ejecutar el mandato '%s'\n",filtros[i]);
					exit(1);
					break;
				}

			default:
				/* Padre: Redireccion */
				close(pp[0]);
				dup2(pp[1],1);
				close(pp[1]);
				break;
		}
	}

}


void imprimir_estado(char* filtro, int status)
{
	/* Imprimimos el nombre del filtro y su estado de terminacion */
	if(WIFEXITED(status))
		fprintf(stderr,"%s: %d\n",filtro,WEXITSTATUS(status));
	else
		fprintf(stderr,"%s: senyal %d\n",filtro,WTERMSIG(status)); // PONÍA senal, NO senyal
}


void esperar_terminacion(void)
{
	int p;
	int status;
	for(p=0;p<n_filtros;p++)
	{
		/* Espera al proceso pids[p] */
		if(waitpid(pids[p],&status, 0)<0){
			fprintf(stderr,"Error al esperar proceso %d\n",pids[p]);
			exit(1);
		}
		/* Muestra su estado. */
		imprimir_estado(filtros[p],status);

	}
}


extern void filtrar_con_filtro(char* nombre_filtro){

	void *handle;
	int (*tratar) (char*,char*,int);
	char *error;
	char buffEnt[BUFSIZE];
	char buffSal[BUFSIZE];
	int nReads;
	int filtrados;
	//Abrimos librería dinámica	
	handle = dlopen(nombre_filtro,RTLD_LAZY);
	//Tratamos el error
	if(!handle){
		fprintf(stderr,"Error al abrir la biblioteca '%s'\n",nombre_filtro);
		exit(1);
	}
	dlerror();    /* Eliminamos cualquier error existente */
	*(void **) (&tratar) = dlsym(handle,"tratar");
	if ((error = dlerror()) != NULL)  {
		fprintf(stderr,"Error al buscar el simbolo '%s' en '%s'\n" ,"tratar",nombre_filtro);
		exit(1);
	}
	while ((nReads=read(0,buffEnt,BUFSIZE))>0){
		filtrados = tratar(buffEnt,buffSal,nReads);
		if(write(1,buffSal,filtrados)<0){
			close(0);
			close(1);
			exit(1);
		}
	}
	close(0);
	close(1);
	if(nReads<0){
		fprintf(stderr,"Error al ejecutar el filtro '%s'\n",nombre_filtro);
		exit(1);
	}
	
	dlclose(handle); // Cerramos la librería dinámica
}
void tratarAlarma(int n){
	fprintf(stderr,"AVISO: La alarma ha saltado!\n");
	int i;
	for(i=0; i<n_filtros;i++){
		kill(pids[i],0); //Verificamos si el proceso i está activo
		if(errno==0){
			if(kill(pids[i],SIGKILL)<0){
				fprintf(stderr,"Error al intentar matar proceso %d\n",pids[i]);
				exit(1);
			}
			
		}
	}			
}

extern void preparar_alarma(){

	char * valor;
	int res;
	struct sigaction act;
        act.sa_handler = &tratarAlarma;
        act.sa_flags = SA_RESTART;
        sigaction(SIGALRM,&act,NULL);
	if((valor=getenv("FILTRAR_TIMEOUT"))!=NULL){
		res = strtol(valor,NULL,10); //atoi(valor); ---ALTERNATIVA---
		if(res<=0){
			fprintf(stderr,"Error FILTRAR_TIMEOUT no es entero positivo: '%s'\n",valor);
			exit(1);
		}	
		alarm(res);	
		fprintf(stderr,"AVISO: La alarma vencera tras %d segundos!\n",res);
		
	}
}
