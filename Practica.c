#include <pthread.h>	//Hilos
#include <unistd.h>		//para sleep
#include <stdio.h>		//para exit y rand
#include <signal.h>		//para señales
#include <stdlib.h>		//para rand y srand
#include <time.h>		//para inicializar la semilla de srand

//Declaracion de los metodos
void nuevoPaciente(int edad);
void eliminar(int i);
int vacunar(int i);
int calculaAleatorio(int min, int max);
void end();
void ordenar();
void writeLogMessage(char *id, char *msg);

//Declaracion de las manejadoras
void manejadoraJunior(int sig);
void manejadoraNormal(int sig);
void manejadoraSenior(int sig);
void terminar(int sig);

//Declaracion de los metodos hilos
void * accionesPaciente();
void * accionesEnfermeros(void *enfermeros);
void * accionesMedico();
void * accionesEstadistico();

//Declaracion de los variables globales
FILE * logFile;
int fin;
int lista[4][15];//id,atendido,tipo,serologia
//Los valores del atendido:
//0 no esta atendido, 1 esta atendiendo, 2 ha vacunado pero no tiene gripe, 3 ha vacunado y tiene gripe
//4 comprueba que si hay reaccion en medico, 5 No hay reaccion y 6 hay reaccion. 
int contadorPacientes;

//Declaracion de los hilos
pthread_t pacientes[15];
pthread_t enfermero[3];
pthread_t medico;
pthread_t estadistico;

//Declaracion de los mutex
pthread_mutex_t semaforoP;
pthread_mutex_t semaforoL;
pthread_mutex_t mutexDeCondicion;
pthread_mutex_t mutexDeCondicion2;

//Declaracion de las condiciones
pthread_cond_t cond;
pthread_cond_t cond2;


int main() {

	int i, j;
	//Se cambia cuando pulsa ctrl+C.
	fin = 0;	
	//array donde almacenamos el tipo de enfermero.
	int tipoEnfermeros[3] = {1,2,3}; 
	
	//signal o sigaction SIGUSR1, paciente junior.
	signal(SIGUSR1,manejadoraJunior);

	//signal o sigaction SIGUSR2, paciente medio.
	signal(SIGUSR2,manejadoraNormal);

	//signal o sigaction SIGPIPE, paciente senior.
	signal(SIGPIPE,manejadoraSenior);

	//signal o sigaction SIGINT, terminar
	signal(SIGINT,terminar);

	//Inicializar recursos (¡Ojo!, Inicializar!=Declarar).
	//a. Semáforos.
	pthread_mutex_init(&semaforoP,NULL);
	pthread_mutex_init(&semaforoL,NULL);
	pthread_mutex_init(&mutexDeCondicion,NULL);
	pthread_mutex_init(&mutexDeCondicion2,NULL);
	//b. Contador de pacientes.
	contadorPacientes = 0;
	//c. Lista de pacientes id 0, atendido 0, tipo 0, serología 0.
	//inicializar las variables de la lista
	for(i=0; i<4; i++){
		for(j=0; j<15;j++){
			lista[i][j]=0;
		}
	}

	//f. Variables condición
	pthread_cond_init(&cond,NULL); 
	pthread_cond_init(&cond2,NULL); 
	//6. Crear 3 hilos enfermer@s.
	pthread_create(&enfermero[0], NULL, accionesEnfermeros,(void *)&tipoEnfermeros[0]);//enfermero 1
	pthread_create(&enfermero[1], NULL, accionesEnfermeros,(void *)&tipoEnfermeros[1]);//enfermero 2
	pthread_create(&enfermero[2], NULL, accionesEnfermeros,(void *)&tipoEnfermeros[2]);//enfermero 3
	//7. Crear el hilo médico.
	pthread_create(&medico, NULL, accionesMedico,NULL);

	//8. Crear el hilo estadístico.
	pthread_create(&estadistico, NULL, accionesEstadistico, NULL);

	//comprobar si ha introducido Ctrl+C.
 	while(fin!=2){
		end();
		//coordinar el tiempo de ejecucion del programa.
		sleep(0.75);	
	}
 	fin = 1;
 	//Espera a que el ultimo paciente se salga
 	pthread_join(pacientes[contadorPacientes%15],NULL);
 	
 	//termina hilo main.
	return 0;
}

	//Metodo que inicializa el hilo paciente
	void nuevoPaciente(int edad){
		//Comprobar si hay espacio en la lista de pacientes.
		printf("Crea paciente nuevo \n");
		pthread_mutex_lock(&semaforoP);
		//printf("Bloquea el mutex\n");
		int i=0;
		//Declaracion buffer de los logs.
		char buffer[20];
		while(lista[0][i]!=0 && i<15){
			i++;
		}
		//a. Si lo hay
		//Hay espacion libre para entrar un paciente nuevo
		if(lista[0][i] == 0){
			//i. Se añade el paciente.
			//	ii. Contador de pacientes se incrementa.
			contadorPacientes+=1;
			sprintf(buffer, "Paciente %d",contadorPacientes);
			//	iii. nuevaPaciente.id = ContadorPacientes.
			lista[0][i]=contadorPacientes;
			//	iv. nuevoPaciente.atendido=0
			lista[1][i]=0;

			//Escribir mensaje al log
			pthread_mutex_lock(&semaforoL);
			char msg[] = "Entra a la sala";
			writeLogMessage(buffer,msg);

			//	v. tipo=Depende de la señal recibida.
			if(edad==1){	//Es un niño

				lista[2][i]=1;
				char tipo[] = "El paciente es de tipo niño";
				writeLogMessage(buffer,tipo);

			}else if(edad==2){	//Es un adulto

				lista[2][i]=2;
				char tipo[]= "El paciente es de tipo adulto";
				writeLogMessage(buffer,tipo);

			}else{	//Es un senior

				lista[2][i]=3;
				char tipo[] = "El paciente es de tipo senior";
				writeLogMessage(buffer,tipo);

			}
			pthread_mutex_unlock(&semaforoL);

			//	vi. nuevoPaciente.Serología=0.
			lista[3][i]=0;

			//ordenar la lista de los pacientes.
			ordenar();
			//	vii. Creamos hilo para el paciente.
			pthread_create(&pacientes[contadorPacientes%15], NULL, accionesPaciente,(void *)&contadorPacientes);
		}
		pthread_mutex_unlock(&semaforoP);

		//	b. Si no hay espacio
		//	i. Se ignora la llamada.
		return;
	}

	//Manejadora que crea a los Juniors.
	void manejadoraJunior(int sig){
		if(fin == 0){
			printf("Se crea paciente junior y entra a la sala de espera.\n");
			int edad = 1;
			nuevoPaciente(edad);
		}
	}

	//Manejadora que crea a los Adultos.
	void manejadoraNormal(int sig){
		if(fin == 0){
			printf("Se crea paciente medio y entra a la sala de espera.\n");
			int edad = 2;
			nuevoPaciente(edad);
		}
	}
	//Manejadora que crea a los Seniors.
	void manejadoraSenior(int sig){
		if(fin == 0){
			printf("Se crea paciente senior y entra a la sala de espera.\n");
			int edad = 3;
			nuevoPaciente(edad);
		}
	}

	//Se ejecutan las acciones pacientes.
	void *accionesPaciente(void *id){
		//	1. Guardar en el log la hora de entrada.
		//	2. Guardar en el log el tipo de solicitud.
		//	3. Duerme 3 segundos.
		int ID = *(int *)id, i = 0;			//Asignar ID al paciente, empieza de 1.
		char buffer[20];					//Declaracion buffer de los logs.
		sprintf(buffer, "Paciente %d",ID);	
		sleep(3);
		//4. Comprueba si está siendo atendido.
		//	localizar la posicion.	i = Posicion del paciente.
		pthread_mutex_lock(&semaforoP);

		//localiza la posicion que esta el paciente en la lista.
		while(lista[0][i] != ID && i<15){		
			i++;
		}

		//Cuando el paciente esta esperando en la cola.
		while(lista[1][i] == 0){
			pthread_mutex_unlock(&semaforoP);

		//5. Si no lo está, calculamos el comportamiento del paciente (si se va por cansarse
		//de esperar, si se lo piensa mejor) o si se va al baño y pierde su turno.
			srand((int)pthread_self());					//inicializa la semilla.
			int aleatorio = calculaAleatorio(1,100);	//Calcula un valor aleatorio.
			if(aleatorio<=20){							//El paciente se cansa.
				printf("El paciente %d se cansa",ID);
				pthread_mutex_lock(&semaforoL);
				char cansar[] = "Se cansa";
				writeLogMessage(buffer,cansar);
				pthread_mutex_unlock(&semaforoL);
				eliminar(i);
				pthread_exit(NULL);		
			}else if(aleatorio >20 && aleatorio<=30){	//El paciente se lo piensa.
				printf("El paciente %d se lo piensa y se va",ID);
				pthread_mutex_lock(&semaforoL);
				char pensar[] = "Se lo piensa y se va";
				writeLogMessage(buffer,pensar);
				pthread_mutex_unlock(&semaforoL);
				eliminar(i);
				pthread_exit(NULL);		
			}else if(aleatorio>30 && aleatorio<=35){	//El paciente se va al baño.
				printf("El paciente %d se va al baño",ID);
				pthread_mutex_lock(&semaforoL);
				char irBanio[] = "Se va al baño";
				writeLogMessage(buffer,irBanio);
				pthread_mutex_unlock(&semaforoL);
				eliminar(i);
				pthread_exit(NULL);		
			}else{										//El paciente se mantiene en la cola.
				sleep(3);
			}
			
		}
		pthread_mutex_unlock(&semaforoP);

		//6. Si está siendo atendido por el enfermer@ debemos esperar a que termine.
		while(lista[1][i] == 1){
			sleep(1);
		}

		if(lista[1][i] == 3){				//El paciente tiene gripe.
			eliminar(i);
			printf("El paciente se va porque tiene gripe\n");
			pthread_exit(NULL);
		}else{								//Calcula si le da reaccion.
		//7. Si no se va por gripe o catarro calcula si le da reacción
		//en este caso lista[1][i] == 2, que no tiene gripe.
		pthread_mutex_lock(&semaforoP);
		lista[1][i] = 4;
		pthread_mutex_unlock(&semaforoP);

		}

		//Espera hasta que el medico pueda comprobar su reaccion.
		while(lista[1][i] == 4){
			sleep(1);
		}

		//El paciente no tiene reaccion.
		if(lista[1][i] == 5){
			printf("Paciente ID: %d no tiene reacción\n",lista[0][i]);

			int encuesta=calculaAleatorio(1,100);
			//	i. Si decide participar
			if(encuesta <= 25){
				
				//	ii. Cambia el valor de la variable serológica
				pthread_mutex_lock(&semaforoP);
				lista[3][i] = 1;

				pthread_mutex_lock(&semaforoL);
				char estudio[] = "Esta preparado para el estudio";
				writeLogMessage(buffer,estudio);
				pthread_mutex_unlock(&semaforoL);

				pthread_mutex_unlock(&semaforoP);

				pthread_cond_signal(&cond);						//Mandar la señal al estadistico.

				//4. Se queda esperando a que digan que pueden marchar
				pthread_cond_wait(&cond2, &mutexDeCondicion2);	//Espera que le responde el estadistico.
				pthread_mutex_lock(&semaforoL);

				char finEstudio[] = "Se va del estudio";
				writeLogMessage(buffer,finEstudio);

				pthread_mutex_unlock(&semaforoL);

			}else{	//Decide no ir a la encuesta
				pthread_mutex_lock(&semaforoL);

				char noEncuesta[] = "Decide no ir a la encuesta";
				writeLogMessage(buffer,noEncuesta);

				pthread_mutex_unlock(&semaforoL);
			}

			//El paciente tiene reaccion.
		}else if(lista[1][i] == 6){
			printf("Paciente ID: %d Si hay reacción\n",lista[0][i]);
		}
		//8. Libera su posición en cola de solicitudes y se va
		eliminar(i);
		pthread_exit(NULL);
	
	}
	
	void *accionesEnfermeros(void *tipoEnfermero){	//tipoEnfermero = {1,2,3}
		//	1. Buscar al primer paciente para atender de su tipo, esto es el que más tiempo
		//	lleve esperando.
		int i, j, numAtendidos = 0, tipo = *(int *)tipoEnfermero;	//Declarar los variables.
		char buffer[20];
		sprintf(buffer, "Enfermero %d",tipo);
		while(fin != 1){	//Se repite hasta que termine el programa.
			int atiende = 0;

			//1. Buscar al primer paciente para atender de su tipo, esto es el que más tiempo lleve esperando.
			while(atiende != 1){	
				pthread_mutex_lock(&semaforoP);
				for(i=0; i<15; i++){	//comprueba que si hay paciente para atender.
					if(lista[0][i]>0 && lista[1][i] == 0){
						atiende = 1;
					}
				}
				if(atiende != 1){//b. Si no hay pacientes para atender espero un segundo y vuelvo al paso 1.
					pthread_mutex_unlock(&semaforoP);
					sleep(1);
				}
			}

			//Los siguientes codigos se ejecuta cuando hay pacientes para atender.
			//Se localiza el paciente que se atiende.
			j=-1;
			for(i=0; i<15; i++){
				if(lista[0][i]>0 && lista[1][i] == 0 && lista[2][i] == tipo){	//cumple cuando encuentra 
					if(j == -1){	//un paciente que tiene ID, puede ser atendido, y de su tipo.
						j=i;	//j es la posicion del paciente que hay que coger
					}else if(lista[0][j] > lista[0][i]){	//coge siempre el que tenga ID menor.
						j=i;
					}
				}
			}
			if(j == -1){ //a. Si no hay de mi tipo busco uno del otro.
				for(i=0; i<15; i++){
					if(lista[0][i]>0 && lista[1][i] == 0){	//cumple cuando
						if(j == -1){	//Tiene ID, puede ser atendido.
							j=i;	//j es la posicion del paciente
						}else if(lista[0][j] > lista[0][i]){	//coge siempre el que tenga ID menor.
							j=i;
						}
					}
				}
			}
			//j es la posicion del paciente.

			//2. Cambiamos el flag de atendido.
			lista[1][j] = 1; 
			pthread_mutex_unlock(&semaforoP);

			//Enfermero atiende a tipo paciente.
			if(lista[2][j] ==1){
				printf("Enfermero ID: %d, atiendo a niño, ID: %d \n", tipo, lista[0][j]);
			}
			else if (lista[2][j] == 2){
				printf("Enfermero ID: %d, atiendo a normal, ID: %d \n", tipo, lista[0][j]);
			}
			else{
				printf("Enfermero ID: %d, atiendo a senior, ID: %d \n", tipo, lista[0][j]);
			}

			//3. Calculamos el tipo de atención y en función de esto el tiempo de atención (el
			//80%, 10%, 10%).
			//4. Guardamos en el log que comienza la atención
			pthread_mutex_lock(&semaforoL);
			char atender[] = "Comienza a atender";
			writeLogMessage(buffer,atender);
			pthread_mutex_unlock(&semaforoL);
			//5. Dormimos el tiempo de atención.

			//llama a metodo vacunar.
			int motivo = vacunar(j);

			//6. Guardamos en el log que finaliza la atención
			pthread_mutex_lock(&semaforoL);
			char finAtender[] = "Termina de atender";
			writeLogMessage(buffer,finAtender);
			pthread_mutex_unlock(&semaforoL);

			//7. Guardamos en el log el motivo del fin de la atención.
			if(motivo ==1){

				pthread_mutex_lock(&semaforoL);
				char malIdf[50];
				sprintf(malIdf, "El paciente %d esta mal identificado",lista[0][j]);
				writeLogMessage(buffer,malIdf);
				pthread_mutex_unlock(&semaforoL);

			}else if(motivo==2){

				pthread_mutex_lock(&semaforoL);
				char correcto[50];
				sprintf(correcto,"El paciente %d es correcto",lista[0][j]);
				writeLogMessage(buffer,correcto);
				pthread_mutex_unlock(&semaforoL);

			}else{

				pthread_mutex_lock(&semaforoL);
				char gripe[50];
				sprintf(gripe,"El paciente %d tiene gripe, debe irse",lista[0][j]);
				writeLogMessage(buffer,gripe);
				pthread_mutex_unlock(&semaforoL);

			}
			//8. Cambiamos el flag de atendido (ojo, si se trata de un paciente con gripe debe
			//irse del consultorio)
			
			//incrementar num atendido
			numAtendidos++;
			//	9. Mira si le toca tomar cafe, cuando atiende a 5 paciente
			if(numAtendidos%5 == 0){
				printf("El enfermero %d Se va a tomar un cafe.\n",tipo);
				sleep(5);
			}

		}	//termina el bucle while y vuelve al paso 1

		pthread_exit(NULL);


	}
	
	void *accionesMedico(){
		int i,j;

		while(fin != 1){

			pthread_mutex_lock(&semaforoP);
			j=-1;

			//Busca el paciente vvacunado.
			for(i=0; i<15; i++){
				if(lista[1][i] == 4){	
					if(j == -1){
						j=i;
					//comprobar si es el que lleve mas tiempo esperando
					}else if(lista[0][j] > lista[0][i]){	
						j=i;
					}
				}
			}
			pthread_mutex_unlock(&semaforoP);

			if(j == -1){	//Si no hay ninguno para comprobar que si tiene reaccion.

				for(i=0; i<15; i++){	//busca el paciente para vacunar.
					if(lista[0][i]>0 && lista[1][i] == 0){	//cumple cuando
						if(j == -1){	//Tiene ID, puede ser atendido.
							j=i;	//j es la posicion del paciente
						}else if(lista[0][j] > lista[0][i]){	//coge siempre el que tenga ID menor.
							j=i;
						}
					}
				}

				if(j != -1){	//Tiene pacientes para vacunar
					printf("Medico atiendo a Paciente, ID: %d \n", lista[0][j]);

					pthread_mutex_lock(&semaforoL);
	
					char atender[] = "Comienza a vacunar";
					writeLogMessage("medico",atender);

					pthread_mutex_unlock(&semaforoL);

					//vacuna al paciente.
					int motivo = vacunar(j);
					//rmotivo = resultado del vacunar.
					if(motivo ==1){

						pthread_mutex_lock(&semaforoL);
						char malIdf[50]; 
						sprintf(malIdf, "El paciente %d esta mal identificado",lista[0][j]);
						writeLogMessage("medico",malIdf);
						pthread_mutex_unlock(&semaforoL);

					}else if(motivo==2){

						pthread_mutex_lock(&semaforoL);
						char correcto[50];
						sprintf(correcto,"El paciente %d es correcto",lista[0][j]);
						writeLogMessage("medico",correcto);
						pthread_mutex_unlock(&semaforoL);

					}else{

						pthread_mutex_lock(&semaforoL);
						char gripe[50];
						sprintf(gripe,"El paciente %d tiene gripe, debe irse",lista[0][j]);
						writeLogMessage("medico",gripe);
						pthread_mutex_unlock(&semaforoL);

					}

				}else{	//si no hay pacientes para vacunar
					sleep(1);
				}

			}else{	//Comprueba el paciente vacunado si le da reaccion.

				pthread_mutex_lock(&semaforoL);
				char atenderReac[] = "Comprueba las reacciones";
				writeLogMessage("medico",atenderReac);
				pthread_mutex_unlock(&semaforoL);

				int reaccion=calculaAleatorio(1,100);	//calcula aleatorio.
				//8. Cambiamos el flag de atendido (si hay reacción debe irse del consultorio)

				if(reaccion <= 10){	//El pacinte tiene reaacion
					
					pthread_mutex_lock(&semaforoP);
					printf("Paciente ID: %d Hay reacción\n",lista[0][j]);

					pthread_mutex_lock(&semaforoL);
					char reacSi[50];
					sprintf(reacSi,"El paciente %d da reacción",lista[0][j]);
					writeLogMessage("medico",reacSi);
					pthread_mutex_unlock(&semaforoL);

					sleep(5);			//quedar 5 segundo con el paciente.
					lista[1][j] = 6;	//cambiar el flag de atendido

					pthread_mutex_unlock(&semaforoP);

				}else{	//El paciente no tiene reaccion.
					
					pthread_mutex_lock(&semaforoP);
	
					pthread_mutex_lock(&semaforoL);
					char reacNo[50];
					sprintf(reacNo,"El paciente %d no da reacción",lista[0][j]);
					writeLogMessage("medico",reacNo);
					pthread_mutex_unlock(&semaforoL);

					lista[1][j] = 5;	//cambiar el flag de atendido
					pthread_mutex_unlock(&semaforoP);

				}

				pthread_mutex_lock(&semaforoL);
				char acabar[] = "Acaba de comprobar las reacciones";
				writeLogMessage("medico",acabar);
				pthread_mutex_unlock(&semaforoL);

			}
		}//9. Volvemos al paso 1 

		pthread_exit(NULL);

	}

	void *accionesEstadistico(){
		while(fin != 1){ //Repite hasta que acaba el programa.

			//1. Espera que le avisen de que hay paciente en estudio
			pthread_cond_wait(&cond, &mutexDeCondicion);

			pthread_mutex_lock(&semaforoP);
			int i=0;
			//Busca en la lista el paciente que decide entrar a la encuesta.
			while(lista[3][i] != 1 && i<15){
				i++;
			}


			//2. Escribe en el log que comienza la actividad
			pthread_mutex_lock(&semaforoL);
			char empezar[50];
			sprintf(empezar, "Comienza la atención del paciente %d",lista[0][i]);
			writeLogMessage("estadístico",empezar);
			pthread_mutex_unlock(&semaforoL);

			pthread_mutex_unlock(&semaforoP);

			if (lista[3][i]==1){ //El estadistico estudia el paciente.
				//3. Calcula el tiempo de actividad (4 segundos)
				sleep(4);
				//5. Escribe en el log que finaliza la actividad
				pthread_mutex_lock(&semaforoL);
				char acabar[50];
				sprintf(acabar,"Acaba la atención del paciente %d",lista[0][i]);
				writeLogMessage("estadístico",acabar);
				pthread_mutex_unlock(&semaforoL);


				//4. Termina la actividad y avisa al paciente
				printf("El paciente %d ha sido estudiado\n",lista[0][i]);
				//Mandar señal al paciente, y se puede marchar el paciente.
				pthread_cond_signal(&cond2);
			}

		}	//6. Cambia paciente en estudio y vuelve a 1

		pthread_exit(NULL);

		
	}

	//El metodo que elimina los datos del paciente.
	void eliminar(int i){
		int j;
		pthread_mutex_lock(&semaforoP);
		printf("Paciente ID:%d Eliminado\n",lista[0][i]);
		for(j=0; j<4; j++){
			lista[j][i] = 0;
		}
		pthread_mutex_unlock(&semaforoP);

		return;
	}

	//El metodo que vacuna al paciente.
	int vacunar(int i){
		srand(time(NULL));
		int estadoPacientes = calculaAleatorio(1,100);

		if(estadoPacientes <= 10){		//El paciente esta mal identificado.
			int tiempo = calculaAleatorio(2,6);
			sleep(tiempo);
			printf("Paciente ID: %d Mal identificado \n",lista[0][i]);			
			pthread_mutex_lock(&semaforoP);
			lista[1][i] = 2;
			pthread_mutex_unlock(&semaforoP);
			return 1;
		}
		else if(estadoPacientes > 20){	//El paciente esta correcto.
			int tiempo = calculaAleatorio(1,4);
			sleep(tiempo);
			printf("Paciente ID: %d correcto \n",lista[0][i]);
			pthread_mutex_lock(&semaforoP);
			lista[1][i] = 2;
			pthread_mutex_unlock(&semaforoP);
			return 2;
		}else{							//El paciente tiene catarro o gripe.
			int tiempo = calculaAleatorio(6,10);	
			sleep(tiempo);
			printf("Paciente ID: %d Tiene catarro o gripe \n",lista[0][i]);
			pthread_mutex_lock(&semaforoP);			
			lista[1][i] = 3;
			pthread_mutex_unlock(&semaforoP);
			return 3;
		}

	}

	//El metodo que calcula un numero aleatorio entren el rango [min, max].
	int calculaAleatorio(int min, int max){
		int salida;
		srand(time(NULL));
		salida = rand() % (max-min+1) + min;

		return salida;
	}
	
	//El metodo que comprueba que si el programa se termina.
	void end(){
		int i;
		pthread_mutex_lock(&semaforoP);
		if(fin == -1){
			fin = 2;
			for (i=0;i<15;i++){
				if(lista[0][i]!=0){
					fin=-1;
				}
			}
		}
		pthread_mutex_unlock(&semaforoP);

	}
	
	//El metodo que ordena los pacientes de la cola.
	void ordenar(){
		int pos = 0, aux = 0, i;
		while(pos<15 && aux==0){
			if(lista[0][pos] != 0){
				aux=pos;
			}
			pos+=1;
		}

		while (pos<15){
			if(lista[0][pos] != 0){
				if(lista[0][aux]>lista[0][pos]){

					i=lista[0][aux];
					lista[0][aux]=lista[0][pos];
					lista[0][pos]=i;

					i=lista[1][aux];
					lista[1][aux]=lista[1][pos];
					lista[1][pos]=i;

					i=lista[2][aux];
					lista[2][aux]=lista[2][pos];
					lista[2][pos]=i;

					i=lista[3][aux];
					lista[3][aux]=lista[3][pos];
					lista[3][pos]=i;

					aux = pos;
				}
			}
		pos+=1;
		}
	}

	//Manejadora que recibe la señal SIGINT
	void terminar(int sig){
		if (fin==0){
			fin = -1;
		}
	}

	//Metodo que escribe los mensajes de logs.
	void writeLogMessage(char *id, char *msg) {
		// Calculamos la hora actual
		time_t now = time(0);
		struct tm *tlocal = localtime(&now);
		char stnow[25];
		strftime(stnow, 25, "%d/%m/%y %H:%M:%S", tlocal);
		// Escribimos en el log
		logFile = fopen("logFileName", "a");
		fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
		fclose(logFile);
	}
