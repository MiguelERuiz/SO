#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "message.h"

void escribir_puerto (int puerto);
int abrirSocketUDP(struct sockaddr_in dirUDPSer);
int abrirSocketTCP(struct sockaddr_in dirTCPSer,int nPeticiones);
int abrirPuertoUDP(int socketUDP,struct sockaddr_in dirUDPSer);
int abrirPuertoTCP(int socketTCP,struct sockaddr_in dirTCPSer);
void tratarPeticiones(int socketUDP,int socketTCP,int puertoTCP,UDP_Msg mensaje,struct sockaddr_in dirUDPCli,struct sockaddr_in dirTCPCli,size_t tam_dir);
void enviarFichero(int sock, int fich);


/* FUNCION MAIN DEL PROGRAMA SERVIDOR */
int main(int argc,char* argv[])
{
	/* El esqueleto de la funcion principal muestra, de forma esquematica la secuencia 
	   de operaciones y la correspondiente traza que se sugiere */
	UDP_Msg mensaje;
	int socketUDP;/*Socket UDP para conectarse con el cliente */
	int puertoUDP;/*Puerto UDP del servidor */
	int socketTCP;/*Socket TCP para conectarse con el cliente */
	int puertoTCP;/*Puerto TCP del servidor */
	struct sockaddr_in dirUDPSer;
	struct sockaddr_in dirTCPSer;
	int nPeticiones = argc-1;
	/*********************ABRE SOCKET UDP***********************************/	
	socketUDP = abrirSocketUDP(dirUDPSer);
	puertoUDP = abrirPuertoUDP(socketUDP,dirUDPSer);
	/*******************FIN ABRE SOCKET UDP ******************************/
	/*Escribimos el puerto de servicio*/
	escribir_puerto(puertoUDP); /*Numero de puerto asignado */
	/****************** ABRE SOCKET TCP ***********************************/
	socketTCP = abrirSocketTCP(dirTCPSer,nPeticiones);
	puertoTCP = abrirPuertoTCP(socketTCP,dirTCPSer);
	/****************** FIN ABRE SOCKET TCP ***********************************/

	/*************************** TRATAMIENTO MENSAJES CLIENTE *************************/
	while(1){
		struct sockaddr_in dirUDPCli;
		struct sockaddr_in dirTCPCli;
		size_t tam_dir;
		tam_dir = sizeof(struct sockaddr_in);
		tratarPeticiones(socketUDP,socketTCP,puertoTCP,mensaje,dirUDPCli,dirTCPCli,tam_dir);
	}		
	fprintf(stdout,"SERVIDOR: Finalizado\n");
	exit(0);
}


/************************FUNCIONES AUXILIARES ***********************/

/*Funcion que crea un socket UDP */

int abrirSocketUDP(struct sockaddr_in dirUDPSer){
	/*Creacion del socket UDP*/
	int socketUDP;
        fprintf(stdout,"SERVIDOR: Creacion del socket UDP: ");
        if((socketUDP= socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))<0){
                fprintf(stdout, "ERROR\n");
                exit(1);
        }
        fprintf(stdout, "OK\n");

        /* Asignacion de la direccion local del servidor */
        bzero((char*)&dirUDPSer,sizeof(struct sockaddr_in));/*Rellenamos a 0 la estructura */
        dirUDPSer.sin_family = AF_INET;
        dirUDPSer.sin_addr.s_addr = inet_addr(HOST_SERVIDOR);
        dirUDPSer.sin_port = htons(0);
        fprintf(stdout,"SERVIDOR: Asignacion del puerto servidor: ");
        if(bind(socketUDP,(struct sockaddr*)&dirUDPSer,sizeof(dirUDPSer))<0){
                close(socketUDP);
                fprintf(stdout,"ERROR\n");
                exit(1);
        }
        fprintf(stdout,"OK\n");
	return socketUDP;
}

/*Funcion que abre un puerto UDP */
int abrirPuertoUDP(int socketUDP, struct sockaddr_in dirUDPSer){
	int puertoUDP;
	socklen_t lenUDP = sizeof(dirUDPSer);
        getsockname(socketUDP,(struct sockaddr*)&dirUDPSer,&lenUDP);
        puertoUDP = htons(dirUDPSer.sin_port);
	return puertoUDP;
}

/* Funcion auxiliar que escribe un numero de puerto en el fichero */
void escribir_puerto (int puerto)
{
	int fd;
	if((fd=creat(FICHERO_PUERTO,0660))>=0)
	{
		write(fd,&puerto,sizeof(int));
		close(fd);
		fprintf(stdout,"SERVIDOR: Puerto guardado en fichero %s: OK\n",FICHERO_PUERTO);
	}
}

/*Funcion que abre un socket TCP */
int abrirSocketTCP(struct sockaddr_in dirTCPSer,int nPeticiones){
	 // Creacion del socket TCP de servicio
	int socketTCP;
        fprintf(stdout,"SERVIDOR: Creacion del socket TCP: ");
        socketTCP = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(socketTCP<0){
                fprintf(stdout,"ERROR\n");
                exit(1);
        }
        fprintf(stdout,"OK\n");
        /*Asignacion de la direccion local del servidor*/
        bzero((char*)&dirTCPSer,sizeof(dirTCPSer));
        dirTCPSer.sin_family = AF_INET;
        dirTCPSer.sin_addr.s_addr = inet_addr(HOST_SERVIDOR);
        dirTCPSer.sin_port = htons(0);
        fprintf(stdout,"SERVIDOR: Asignacion del puerto servidor: ");
        if(bind(socketTCP,(struct sockaddr*)&dirTCPSer,sizeof(dirTCPSer))<0){
                fprintf(stdout,"ERROR\n");
                exit(1);
        }
        fprintf(stdout,"OK\n");
        /*Aceptamos conexiones por el socket */
        fprintf(stdout,"SERVIDOR: Aceptacion de peticiones: ");
        if(listen(socketTCP,nPeticiones)<0){
                fprintf(stdout,"ERROR\n");
                exit(1);
        }
        fprintf(stdout,"OK\n");
	return socketTCP;
}

/*Funcion que abre un puerto TCP */
int  abrirPuertoTCP(int socketTCP, struct sockaddr_in dirTCPSer){
	/*Puerto TCP ya disponible */
	int puertoTCP;
        socklen_t lenTCP = sizeof(dirTCPSer);
        fprintf(stdout,"SERVIDOR: Puerto TCP reservado: ");
        if(getsockname(socketTCP,(struct sockaddr*)&dirTCPSer,&lenTCP)<0){
                fprintf(stdout,"ERROR\n");
                exit(1);
        }
        fprintf(stdout,"OK\n");
        puertoTCP = dirTCPSer.sin_port;
	return puertoTCP;
}

/*Tratamiento de las peticiones del cliente */

void tratarPeticiones(int socketUDP,int socketTCP,int puertoTCP,UDP_Msg mensaje,struct sockaddr_in dirUDPCli,struct sockaddr_in dirTCPCli,size_t tam_dir){
	socklen_t size;
	int cd,fd;
	bzero((char*)&dirUDPCli,sizeof(struct sockaddr_in));//Ponemos a 0 la estructura
	bzero((char*)&mensaje,sizeof(UDP_Msg));
	fprintf(stdout,"SERVIDOR: Esperando mensaje.\n");
	if(recvfrom(socketUDP,(char*)&mensaje,sizeof(UDP_Msg),0,(struct sockaddr*)&dirUDPCli,(socklen_t *)&tam_dir)<0){
		fprintf(stdout,"SERVIDOR: Mensaje del cliente: ERROR\n");
		exit(1);
	}
	fprintf(stdout,"SERVIDOR: Mensaje del cliente: OK\n");
	if(ntohl(mensaje.op)==QUIT){
		mensaje.op = htonl(OK);
		fprintf(stdout,"SERVIDOR: QUIT\n");
		if(sendto(socketUDP,(char*)&mensaje,sizeof(UDP_Msg),0,(struct sockaddr*)&dirUDPCli,tam_dir)<0){
			fprintf(stdout,"SERVIDOR: Enviando del resultado [OK]: ERROR");
			exit(1);
		}
		fprintf(stdout,"SERVIDOR: Enviando del resultado [OK]: OK\n");
		fprintf(stdout,"SERVIDOR: Finalizado\n");
		exit(0);
	}
	if(ntohl(mensaje.op)==REQUEST){
		fprintf(stdout,"SERVIDOR: REQUEST(%s,%s)\n",mensaje.local,mensaje.remoto);
		if((fd = open(mensaje.remoto,O_RDONLY))<0){
			mensaje.op = htonl(ERROR);
			fprintf(stdout,"SERVIDOR: Enviando del resultado [ERROR]: ");
			if(sendto(socketUDP,(char*)&mensaje,sizeof(UDP_Msg),0,(struct sockaddr*)&dirUDPCli,tam_dir)<0){
				fprintf(stdout,"ERROR\n");
				exit(1);
			}
			fprintf(stdout, "OK\n");
		}else if(fd >=0){
			// Servidor responde al cliente usando el puerto UDP con mensaje.op=OK
			mensaje.op = htonl(OK);
			mensaje.puerto = puertoTCP;
			fprintf(stdout,"SERVIDOR: Enviando del resultado [OK]: ");
			if(sendto(socketUDP,(char*)&mensaje,sizeof(UDP_Msg),0,(struct sockaddr*)&dirUDPCli,tam_dir)<0){
				fprintf(stdout,"ERROR\n");
				exit(1);
			}
			fprintf(stdout,"OK\n");
			//Esperamos la llegada de una conexion
			size = sizeof(dirTCPCli);
			fprintf(stdout,"SERVIDOR: Llegada de un mensaje: ");
			if((cd = accept(socketTCP,(struct sockaddr*)&dirTCPCli,&size))<0){
				fprintf(stdout,"ERROR\n");
				exit(1);
			}
			fprintf(stdout,"OK\n");
			enviarFichero(cd,fd);
		}
	}

}
/*Lee datos del fichero y los envia al socket */
void enviarFichero(int sock,int fich){

	char buff[512];
	size_t tam;
	while((tam=read(fich,(void*)buff,512))>0){
		write(sock,(void*)buff,tam);
	}
	close(fich);
	close(sock);
}
