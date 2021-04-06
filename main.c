#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>


#define ES_PROCESO_0(id) (id == 0)
#define ES_ULTIMO_PROCESO(id) (id == nProc-1)
#define ES_DIVISOR(d,n) (n % d == 0)

#define MPI_INFO_PROCESO(info) (crearInfoProceso(info))

#define E_SIG_PROC 1
#define E_NUEVO_DIV 2
#define E_PROC_FIN 3


//Declaracion de tipos
typedef unsigned long Numero;

typedef struct infoProceso{
	double tCalculo;
	Numero nDivisores;
	Numero sumaIntervalo;
} InfoProceso;



//Prototipos de funciones
void calculoDivisores(Numero num, Numero iInicio, Numero iFinal);
MPI_Datatype crearInfoProceso(InfoProceso info);
void imprimirDatosFinales(InfoProceso *infoProcesos, int nProc, Numero sumaRecibida);



/* MAIN */
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
		printf("NUMERO A COMPROBAR: %lu EN %d PROCESOS\n\n", num, nProc);
		intervalo = num / (nProc - 1);
	}
	
	MPI_Bcast(&num, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
	MPI_Bcast(&intervalo, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
	
	
	//Cálculo
	Numero sumaRecibida = 0;
	Numero sumaCalculada, divRec;
	MPI_Status status;
	
	InfoProceso infoProcesos[nProc - 1];
	int i;
	
	if(ES_PROCESO_0(id)){
		
		//Debe esperar por todos los div de un proceso
		//Luego ya imprime que ha acabado
		
		int procPorAcabar = nProc - 1;
		int flag;
		sumaCalculada = 0;
		
		int divAcu[nProc - 1];
		for(i=0; i<nProc-1; i++) 
			divAcu[i] = 0;
		
		while(procPorAcabar > 0){
		
			//MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			
			MPI_Iprobe(MPI_ANY_SOURCE, E_NUEVO_DIV, MPI_COMM_WORLD, &flag, &status);
			if(flag != 0){
				MPI_Recv(&divRec, 1, MPI_UNSIGNED_LONG, MPI_ANY_SOURCE, E_NUEVO_DIV, MPI_COMM_WORLD, &status);	
				fprintf(stdout, "DIV: %3d,  DIV RECV: %10lu,  DIV ACU: %5d\n", status.MPI_SOURCE, divRec, divAcu[status.MPI_SOURCE - 1]);
				sumaCalculada += divRec; 
				divAcu[status.MPI_SOURCE - 1]++;
			
			}else{
			
				MPI_Iprobe(MPI_ANY_SOURCE, E_PROC_FIN, MPI_COMM_WORLD, &flag, &status);
				if(flag != 0){
					InfoProceso info;
					MPI_Recv(&info, 1, MPI_INFO_PROCESO(info), MPI_ANY_SOURCE, E_PROC_FIN, MPI_COMM_WORLD, &status);
					
					infoProcesos[status.MPI_SOURCE - 1] = info;
					
					while(infoProcesos[status.MPI_SOURCE - 1].nDivisores != divAcu[status.MPI_SOURCE - 1]){
						MPI_Recv(&divRec, 1, MPI_UNSIGNED_LONG, status.MPI_SOURCE, E_NUEVO_DIV, MPI_COMM_WORLD, &status);
						fprintf(stdout, "DIV: %3d,  DIV RECV: %10lu,  DIV ACU: %5d\n", status.MPI_SOURCE, divRec, divAcu[status.MPI_SOURCE - 1]);
						sumaCalculada += divRec; 
						divAcu[status.MPI_SOURCE - 1]++;
					}
					
						
					fprintf(stdout, "FIN: %3d -> tiempo:%f\n", status.MPI_SOURCE, infoProcesos[status.MPI_SOURCE - 1].tCalculo);
					procPorAcabar--;
				}	
			}		
		}
		
		//Recibe la suma del último proceso
		MPI_Recv(&sumaRecibida, 1, MPI_UNSIGNED_LONG, nProc-1, E_SIG_PROC, MPI_COMM_WORLD, &status);
	
	}else{
		if(ES_ULTIMO_PROCESO(id)){
			calculoDivisores(num, (id-1)*intervalo+1, (id-1)*intervalo + intervalo + num%(nProc-1));
		}else{
			calculoDivisores(num, (id-1)*intervalo+1, (id-1)*intervalo + intervalo);
		}
	}
	
	
	
	if(ES_PROCESO_0(id)){
		if(sumaCalculada == sumaRecibida)
			fprintf(stdout, "\nSUMA TOTAL OK: calculada %lu  recibida %lu\n", sumaCalculada, sumaRecibida);
		else
			fprintf(stdout, "\nSUMA TOTAL ERROR: calculada %lu  recibida %lu\n", sumaCalculada, sumaRecibida);
		
		if(num == sumaRecibida)
			printf("\nEl numero %lu ES PERFECTO\n", num);
		else
			printf("\nEl numero %lu NO ES PERFECTO\n", num);
		
		fprintf(stdout, "\n\n");
		
		imprimirDatosFinales(infoProcesos, nProc, sumaRecibida);		
		
	}
	
	
//Finalize
	MPI_Finalize();
	return 0;
}




void calculoDivisores(Numero num, Numero iInicio, Numero iFinal){

	int nProc, id;
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &nProc);
	
	Numero divisor, sumaAcum = 0;
	double tInicio, tFin;
	InfoProceso info;
	info.nDivisores=0;
	info.sumaIntervalo=0;
		

	tInicio = MPI_Wtime();		
	for(divisor=iInicio; divisor<=iFinal; divisor++){
		if(divisor!=num && ES_DIVISOR(divisor, num)){
			MPI_Send(&divisor, 1, MPI_UNSIGNED_LONG, 0, E_NUEVO_DIV, MPI_COMM_WORLD);
			(info.sumaIntervalo) += divisor;
			info.nDivisores++;
		}
	}
	tFin = MPI_Wtime();	
	info.tCalculo = tFin - tInicio;

	
	//El proceso ha acabado 
	MPI_Send(&info, 1, MPI_INFO_PROCESO(info), 0, E_PROC_FIN, MPI_COMM_WORLD);
	//MPI_Send(&info.nDivisores, 1, MPI_UNSIGNED_LONG, 0, E_PROC_FIN, MPI_COMM_WORLD);
	
	
	//Espero por la suma acumulada de los anteriores (el primer proceso no espera porque el anterior
	// es el 0, y por tanto, no va a calcular nada)
	if(id > 1){
		MPI_Recv(&sumaAcum, 1, MPI_UNSIGNED_LONG, id-1, E_SIG_PROC, MPI_COMM_WORLD, NULL);
	}
	
	sumaAcum += info.sumaIntervalo;
	
	//Enviar a proceso siguiente su resultado sumado al de los procesos anteriores. Si 
	// es el último proceso, le envía el resultado al 0
	if(ES_ULTIMO_PROCESO(id))
		MPI_Send(&sumaAcum, 1, MPI_UNSIGNED_LONG, 0, E_SIG_PROC, MPI_COMM_WORLD);
	else
		MPI_Send(&sumaAcum, 1, MPI_UNSIGNED_LONG, id+1, E_SIG_PROC, MPI_COMM_WORLD);
	
	
}


MPI_Datatype crearInfoProceso(InfoProceso info){
	MPI_Datatype nuevoTipo;
	MPI_Datatype tipos[3];
	int longitudes[3];
	MPI_Aint desp[4];
	MPI_Aint dir[5];

	//Tipos
	tipos[0] = MPI_DOUBLE; 
	tipos[1] = MPI_UNSIGNED_LONG; 
	tipos[2] = MPI_UNSIGNED_LONG;

	//Longitudes
	longitudes[0] = 1;
	longitudes[1] = 1;
	longitudes[2] = 1;
	
	//Direcciones	
	MPI_Get_address(&info, &dir[0]);
	MPI_Get_address(&(info.tCalculo), &dir[1]);
	MPI_Get_address(&(info.nDivisores), &dir[2]);
	MPI_Get_address(&(info.sumaIntervalo), &dir[3]);
	
	//Desplazamientos
	desp[0] = dir[1] - dir[0];
	desp[1] = dir[2] - dir[0];
	desp[2] = dir[3] - dir[0];

	MPI_Type_create_struct(3, longitudes, desp, tipos, &nuevoTipo);
	MPI_Type_commit(&nuevoTipo);
	return nuevoTipo;
}


void imprimirDatosFinales(InfoProceso *infoProcesos, int nProc, Numero sumaRecibida){
	int i;
	fprintf(stdout, "%-9s | %-15s  | %-20s | %-10s\n","Proceso","Nº Divisores","Suma","Tiempo calculo");
	for(i=0; i<70; i++)
		fprintf(stdout,"-");
	fprintf(stdout, "\n");
	
	int totalDiv = 0;
	double tTotal = 0;
	for(i=0; i<nProc-1; i++){
		fprintf(stdout, "%9d | %15lu | %20lu | %10f\n",i+1,infoProcesos[i].nDivisores,infoProcesos[i].sumaIntervalo,infoProcesos[i].tCalculo);
		totalDiv += (int)infoProcesos[i].nDivisores;
		tTotal += infoProcesos[i].tCalculo;
	}
	for(i=0; i<70; i++)
		fprintf(stdout,"-");
	fprintf(stdout, "\n");
	
	fprintf(stdout, "%9s | %15d | %20lu | %10f\n","TOTAL",totalDiv, sumaRecibida, tTotal);
}






