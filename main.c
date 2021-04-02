#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>


#define ES_PROCESO_0(id) (id == 0)
#define ES_ULTIMO_PROCESO(id) (id == nProc-1)
#define ES_DIVISOR(d,n) (n % d == 0)

//Declaracion de tipos
typedef unsigned long Numero;

typedef struct infoProceso{
	int id;
	Numero tCalculo;
	int nDivisores;
	Numero sumaIntervalo;
} InfoProceso;



//Prototipos de funciones
Numero calculoDivisores(Numero num, Numero iInicio, Numero iFinal);



/* MAIN */
int main(int argc, char **argv){
	if(argc != 2) return 1;
	
	int id, nProc;
	Numero intervalo, num;
	
	MPI_Status status;
	
	
	
//Init
	MPI_Init(&argc, &argv);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &nProc);
	
	//Obtenemos el numero como argumento y calculamos los intervalos, 
	//  y enviamos ambos a todos los procesos
	if(ES_PROCESO_0(id)){
		num = strtoul(argv[1], NULL, 10);
		printf("NUMERO A COMPROBAR: %lu EN %d PROCESOS\n\n", num, nProc);
		intervalo = num / (nProc - 1);
	}
	
	MPI_Bcast(&num, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
	MPI_Bcast(&intervalo, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
	
	
	//Cálculo
	Numero sumaIntervalo = 0;
	Numero sumaRecibida = 0;
	
	if(ES_PROCESO_0(id)){
		//Esperar por los mensajes
		MPI_Recv(&sumaRecibida, 1, MPI_UNSIGNED_LONG, nProc-1, 5, MPI_COMM_WORLD, &status);
		printf("%d -> recibido %lu\n", id, sumaRecibida);
	
	}else{
		if(ES_ULTIMO_PROCESO(id)){
			sumaIntervalo = calculoDivisores(num, (id-1)*intervalo+1, (id-1)*intervalo + intervalo + num%(nProc-1));
		}else{
			sumaIntervalo = calculoDivisores(num, (id-1)*intervalo+1, (id-1)*intervalo + intervalo);
		}
	}
	
	//Numero suma=0;
	//MPI_Reduce(&sumaIntervalo, &suma, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
	
	if(ES_PROCESO_0(id)){
		printf("Suma total: %lu\n", sumaRecibida);
		if(num == sumaRecibida){
			printf("\nEl numero %lu es perfecto\n", num);
		}else{
			printf("\nEl numero %lu no es perfecto\n", num);
		}
	}
	
	
//Finalize
	MPI_Finalize();
	return 0;
}




Numero calculoDivisores(Numero num, Numero iInicio, Numero iFinal){

	int nProc, id;
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &nProc);
	
	Numero divisor, sumaIntervalo, sumaAcum;
	
	sumaIntervalo = 0;
	sumaAcum = 0;
	
//Coger tInicio
	for(divisor=iInicio; divisor<=iFinal; divisor++){
		if(divisor!=num && ES_DIVISOR(divisor, num)){
			//Enviar el divisor
			sumaIntervalo += divisor;
		}
	}
//Coger tFin
	
	
	//Espero por la suma acumulada de los anteriores (el primer proceso no espera porque el anterior
	// es el 0, y por tanto, no va a calcular nada)
	if(id > 1){
		MPI_Recv(&sumaAcum, 1, MPI_UNSIGNED_LONG, id-1, 5, MPI_COMM_WORLD, NULL);
	}
	
	sumaAcum += sumaIntervalo;
	
	//Enviar a proceso siguiente su resultado sumado al de los procesos anteriores. Si 
	// es el último proceso, le envía el resultado al 0
	int etiqueta = 5;
	if(ES_ULTIMO_PROCESO(id)){
		MPI_Send(&sumaAcum, 1, MPI_UNSIGNED_LONG, 0, etiqueta, MPI_COMM_WORLD);
	}else{	
		MPI_Send(&sumaAcum, 1, MPI_UNSIGNED_LONG, id+1, etiqueta, MPI_COMM_WORLD);
	}
	
	return sumaIntervalo;
}





