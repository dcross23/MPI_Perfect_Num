#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>


#define ES_PROCESO_0(id) (id == 0)
#define ES_ULTIMO_PROCESO(id) (id == nProc-1)
#define ES_DIVISOR(d,n) (n % d == 0)

//Declaracion de tipos
typedef unsigned long Numero;


//Prototipos de funciones
Numero calculoDivisores(Numero num, Numero iInicio, Numero iFinal);

int main(int argc, char **argv){
	if(argc != 2) return 1;
	
	int id, nProc;
	Numero intervalo, num;
	
	
	
	
	
//Init
	MPI_Init(&argc, &argv);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &nProc);
	
	//Obtenemos el numero como argumento y calculamos los intervalos, 
	//  y enviamos ambos a todos los procesos
	if(ES_PROCESO_0(id)){
		num = strtoul(argv[1], NULL, 10);
		intervalo = num / (nProc - 1);
	}
	
	MPI_Bcast(&num, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
	MPI_Bcast(&intervalo, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
	
	
	//Cálculo
	Numero sumaIntervalo = 0;
	if(ES_PROCESO_0(id)){
		//Esperar por los mensajes
	
	}else{
		if(ES_ULTIMO_PROCESO(id)){
			sumaIntervalo = calculoDivisores(num, (id-1)*intervalo+1, (id-1)*intervalo + intervalo + num%(nProc-1));
		}else{
			sumaIntervalo = calculoDivisores(num, (id-1)*intervalo+1, (id-1)*intervalo + intervalo);
		}
	}
	
	Numero suma=0;
	MPI_Reduce(&sumaIntervalo, &suma, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
	
	if(ES_PROCESO_0(id)){
		printf("Suma total: %lu\n", suma);
		if(num == suma){
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

	Numero divisor, sumaIntervalo;
	
	for(divisor=iInicio; divisor<=iFinal; divisor++){
		if(divisor!=num && ES_DIVISOR(divisor, num)){
			//Enviar el divisor
			sumaIntervalo += divisor;
		}
	}
	
	
	//Enviar a proceso siguiente su resultado sumado al de los procesos anteriores. Si 
	// es el último proceso, le envía el resultado al 0
	/*int etiqueta = 5;
	if(ES_ULTIMO_PROCESO(id)){
		MPI_Send(&sumaIntervalo, 1, MPI_UNSIGNED_LONG, 0, etiqueta, MPI_COMM_WORLD);
	}else{	
		MPI_Send(&sumaIntervalo, 1, MPI_UNSIGNED_LONG, datos.pID+1, etiqueta, MPI_COMM_WORLD);
	}*/
	
	return sumaIntervalo;
}





