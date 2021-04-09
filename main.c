#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define EXPORTAR_TIEMPOS 

#define ES_PROCESO_0(id) (id == 0)
#define ES_ULTIMO_PROCESO(id) (id == nProc-1)
#define ES_DIVISOR(d,n) (n % d == 0)

#define MPI_INFO_PROCESO(info) (crearInfoProceso(info))
#define INICIAR_ARRAY(valor, array, tam) for (i=0; i<tam; i++) \
                      				array[i] = valor;
                      				
#define ES_PERFECTO(num, suma) do { \
		if(num == suma) fprintf(stdout, "\nEl numero %llu ES PERFECTO\n\n", num); \
		else if(num < suma) fprintf(stdout, "\nEl numero %llu NO ES PERFECTO, es EXCESIVO (%llu)\n\n", num, suma); \
		else fprintf(stdout, "\nEl numero %llu NO ES PERFECTO, es DEFECTIVO (%llu)\n\n", num, suma); \
	}while(0);

//Etiquetas
#define E_SIG_PROC 1
#define E_NUEVO_DIV 2
#define E_PROC_FIN 3


//Declaracion de tipos
typedef unsigned long long Numero;

typedef struct infoProceso{
	double tCalculo;
	Numero nDivisores;
	Numero sumaIntervalo;
} InfoProceso;



//Prototipos de funciones
InfoProceso calculoSecuencial(Numero num);
void calculoDivisores(Numero num, Numero iInicio, Numero iFinal);
MPI_Datatype crearInfoProceso(InfoProceso info);
void imprimirDatosFinales(InfoProceso *infoProcesos, int nProc, double tTotal, Numero sumaRecibida);
void exportarTiempos(double tTotal,int nProc, char *archivo);



/* MAIN */
int main(int argc, char **argv){
	if(argc != 2){
		fprintf(stdout, "Error: Numero invalido de argumentos\n");
		fprintf(stdout, "Uso: mpirun -np X [--oversubscribe] %s numero\n", argv[0]);
		return 1;
	}
	
	int i;
	int id, nProc;
	Numero intervalo, num;
	InfoProceso infoTotal;
	
	double tiempoTotal;
	char *archivo = "tiempos.txt";
	
	
//Init
	MPI_Init(&argc, &argv);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &nProc);
	
	//Obtenemos el numero como argumento y calculamos los intervalos, 
	//  y enviamos ambos a todos los procesos
	if(ES_PROCESO_0(id)){
		num = strtoul(argv[1], NULL, 10);
		fprintf(stdout, "NUMERO A COMPROBAR: %llu EN %d PROCESOS\n\n", num, nProc);
	
		//En caso de que solo haya un proceso, lo hace todo de forma secuencial el proceso 0
		if(nProc == 1){
			InfoProceso infoSecuencial;
			
			tiempoTotal = MPI_Wtime();
			infoSecuencial = calculoSecuencial(num);
			tiempoTotal = MPI_Wtime() - tiempoTotal;
			
			//Imprime si es perfecto o no
			ES_PERFECTO(num,infoSecuencial.sumaIntervalo)
				
			imprimirDatosFinales(&infoSecuencial, nProc, tiempoTotal ,infoSecuencial.sumaIntervalo);
			
			#ifdef EXPORTAR_TIEMPOS
				exportarTiempos(tiempoTotal, nProc, archivo);
			#endif
			
//Finalize
			MPI_Finalize();
			return 0;
			
		}else{
		//En caso de que haya más procesos, calcula los intervalos 
			intervalo = num / (nProc - 1);
		}
	}
	
	
	//Envia los intervalos
	MPI_Bcast(&num, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
	MPI_Bcast(&intervalo, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
	
	
	//Cálculo
	Numero sumaRecibida = 0;
	Numero sumaCalculada, divRec;
	MPI_Status status;
	
	InfoProceso infoProcesos[nProc - 1];
	
	tiempoTotal = MPI_Wtime();
	if(ES_PROCESO_0(id)){	
		InfoProceso info;
		int procPorAcabar = nProc - 1;
		
		int divAcu[nProc - 1];   INICIAR_ARRAY(0, divAcu, nProc - 1)
		Numero sumaAcum[nProc - 1];   INICIAR_ARRAY(0, sumaAcum, nProc - 1)
		
		sumaCalculada = 0;		
		while(procPorAcabar > 0){
			MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			
			switch(status.MPI_TAG){
				case E_NUEVO_DIV:
					MPI_Recv(&divRec, 1, MPI_UNSIGNED_LONG, MPI_ANY_SOURCE, E_NUEVO_DIV, MPI_COMM_WORLD, &status);	
					sumaCalculada += divRec; 
					divAcu[status.MPI_SOURCE - 1]++;
					sumaAcum[status.MPI_SOURCE - 1] += divRec;
					fprintf(stdout, "DIV: %3d,  DIV RECV: %10llu,  DIV ACU: %5d,  SUMA ACU: %10llu \n", status.MPI_SOURCE, divRec, divAcu[status.MPI_SOURCE - 1], sumaAcum[status.MPI_SOURCE - 1]);
					break;
				
				case E_PROC_FIN:
					MPI_Recv(&info, 1, MPI_INFO_PROCESO(info), MPI_ANY_SOURCE, E_PROC_FIN, MPI_COMM_WORLD, &status);
					infoProcesos[status.MPI_SOURCE - 1] = info;
					
					//Si llega el mensaje de fín pero todavía no han llegado todos los divisores, espera por ellos
					while(infoProcesos[status.MPI_SOURCE - 1].nDivisores != divAcu[status.MPI_SOURCE - 1]){
						MPI_Recv(&divRec, 1, MPI_UNSIGNED_LONG, status.MPI_SOURCE, E_NUEVO_DIV, MPI_COMM_WORLD, &status);
						sumaCalculada += divRec; 
						divAcu[status.MPI_SOURCE - 1]++;
						sumaAcum[status.MPI_SOURCE - 1] += divRec;
						fprintf(stdout, "DIV: %3d,  DIV RECV: %10llu,  DIV ACU: %5d,  SUMA ACU: %10llu \n", status.MPI_SOURCE, divRec, divAcu[status.MPI_SOURCE - 1], sumaAcum[status.MPI_SOURCE - 1]);
					}
					
						
					fprintf(stdout, "FIN: %3d,  SUMA RECV: %10llu,  SUMA ACU: %10llu", status.MPI_SOURCE, infoProcesos[status.MPI_SOURCE - 1].sumaIntervalo, sumaAcum[status.MPI_SOURCE - 1]);
					if(infoProcesos[status.MPI_SOURCE - 1].sumaIntervalo == sumaAcum[status.MPI_SOURCE - 1])
						fprintf(stdout,",  SUMA ACU OK\n");
					else
						fprintf(stdout,",  SUMA ACU ERROR\n");
					
					procPorAcabar--;
					break;
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
	tiempoTotal = MPI_Wtime() - tiempoTotal;
	
	if(ES_PROCESO_0(id)){
		if(sumaCalculada == sumaRecibida)
			fprintf(stdout, "\nSUMA TOTAL OK: calculada %llu  recibida %llu\n", sumaCalculada, sumaRecibida);
		else
			fprintf(stdout, "\nSUMA TOTAL ERROR: calculada %llu  recibida %llu\n", sumaCalculada, sumaRecibida);
		
		ES_PERFECTO(num, sumaRecibida)
		imprimirDatosFinales(infoProcesos, nProc, tiempoTotal, sumaRecibida);		
		#ifdef EXPORTAR_TIEMPOS
			exportarTiempos(tiempoTotal, nProc, archivo);
		#endif
	}
	
	
//Finalize
	MPI_Finalize();
	return 0;
}



//Funcion para el cálculo secuencial cuando solo hay un proceso
InfoProceso calculoSecuencial(Numero num){
	double tInicio, tFin;
	Numero divisor;
	
	InfoProceso info;
	info.nDivisores=0;
	info.sumaIntervalo=0;
	
	tInicio = MPI_Wtime();
	for(divisor=1; divisor<=num; divisor++){
		if(divisor != num && ES_DIVISOR(divisor, num)){
			info.sumaIntervalo += divisor;
			info.nDivisores++;
			fprintf(stdout, "DIV: %3d,  DIV RECV: %10llu,  DIV ACU: %5llu,  SUMA ACU: %10llu \n", 0, divisor, info.nDivisores, info.sumaIntervalo);
		}
	}
	tFin = MPI_Wtime();
	fprintf(stdout, "FIN: %3d,  SUMA RECV: %10llu,  SUMA ACU: %10llu, SUMA ACU OK", 0, info.sumaIntervalo, info.sumaIntervalo);	
	info.tCalculo = tFin - tInicio;
	
	return info;
}




//Función para el cálculo con varios procesos
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



//Función para imprimir los datos en forma de tabla al final
void imprimirDatosFinales(InfoProceso *infoProcesos, int nProc, double tTotal, Numero sumaRecibida){
	int i;
	Numero nDivisores = 0;
		
	fprintf(stdout, "%-9s | %-15s  | %-20s | %-10s\n","Proceso","Nº Divisores","Suma","Tiempo calculo");
	for(i=0; i<70; i++)
		fprintf(stdout,"-");
	fprintf(stdout, "\n");
	
	if(nProc == 1){
		fprintf(stdout, "%9d | %15llu | %20llu | %10f\n", 0, infoProcesos->nDivisores, infoProcesos->sumaIntervalo,infoProcesos->tCalculo);
		nDivisores = infoProcesos->nDivisores;
	
	}else{
		for(i=0; i<nProc-1; i++){
			fprintf(stdout, "%9d | %15llu | %20llu | %10f\n",i+1,infoProcesos[i].nDivisores,infoProcesos[i].sumaIntervalo,infoProcesos[i].tCalculo);
			nDivisores += (int)infoProcesos[i].nDivisores;
		}
	}
	
	for(i=0; i<70; i++)
		fprintf(stdout,"-");
	fprintf(stdout, "\n");
	
	fprintf(stdout, "%9s | %15llu | %20llu | %10f\n","TOTAL", nDivisores, sumaRecibida, tTotal);
}



//Función para exportar tiempos y crear la gráfica
void exportarTiempos(double tiempoTotal, int nProc, char *archivo){
	FILE *f = fopen(archivo, "a+");
	if(f != NULL){
		fseek(f, 0, SEEK_END);
		fprintf(f, "%d/%f\n", nProc, tiempoTotal);
		fclose(f);
	}
}



//Función para crear el tipo de dato para el struct
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


