#include "memoria.h"
#include <stdbool.h>


int main(void) {
	//tablaA= (t_tablaDePaginas*)malloc(sizeof(t_tablaDePaginas));
	//pag= (t_pagina*)malloc(sizeof(t_pagina));
	tablaA= malloc(sizeof(t_tablaDePaginas));

/*	paginas y tabla son dos listas, debemos hacer malloc? 	*/

	pag->timestamp = 12345;
	pag->key=1;
	pag->value=(char*)"hola";

	tablaA->numeroDePag=1;
	tablaA->pagina= list_create();
	tablaA->modificado = SINMODIFICAR;
	list_add(tablaA->pagina,pag);

	//printf("%llu \n", obtenerHoraActual());

	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/memoria/memoria.config");
	logger_MEMORIA = log_create("memoria.log", "Memoria", 1, LOG_LEVEL_DEBUG);
	//datoEstaEnCache = TRUE;
	//conectar con file system
	conectarAFileSystem();

	//	SEMAFOROS
	sem_init(&semLeerDeConsola, 0, 1);
	sem_init(&semEnviarMensajeAFileSystem, 0, 0);

	// 	HILOS
	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);
	pthread_create(&hiloEscucharMultiplesClientes, NULL, (void*)escucharMultiplesClientes, NULL);
	//pthread_create(&hiloEnviarMensajeAFileSystem, NULL, (void*)enviarMensajeAFileSystem, NULL);


	pthread_join(hiloLeerDeConsola, NULL);
	pthread_join(hiloEscucharMultiplesClientes, NULL);
	//pthread_join(hiloEnviarMensajeAFileSystem, NULL);

	liberar_conexion(conexionLfs);
	log_destroy(logger_MEMORIA);
	config_destroy(config);

	//momentaneamente, asi cerramos todas las conexiones
	FD_ZERO(&descriptoresDeInteres);
	return 0;
}

void leerDeConsola(void){
	char* mensaje;
	log_info(logger_MEMORIA, "Vamos a leer de consola");
	while (1) {
		sem_wait(&semLeerDeConsola);
		mensaje = readline(">");
		if (!strcmp(mensaje, "\0")) {
			break;
		}
		validarRequest(mensaje);
		free(mensaje);
	}
}


/*obtenerHoraActual()
 * Parametros:
 * Descripcion: Hora actual en minutos y microsegundos
 * Return:
 * 	-> :: unsigned long long */
unsigned long long obtenerHoraActual(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long long millisegundosDesdeEpoch = ((unsigned long long)tv.tv_sec) * 1000 + ((unsigned long long)tv.tv_usec) / 1000;
	return millisegundosDesdeEpoch;
}

void validarRequest(char* mensaje){
	int codValidacion;
	codValidacion = validarMensaje(mensaje, MEMORIA, logger_MEMORIA);
	switch(codValidacion){
		case EXIT_SUCCESS:
			interpretarRequest(codValidacion, mensaje,CONSOLE,-1);
			break;
		case NUESTRO_ERROR:
			//es la q hay q hacerla generica
			log_error(logger_MEMORIA, "No es valida la request");
			break;
		default:
			break;
	}
}


void conectarAFileSystem() {
	conexionLfs = crearConexion(
			config_get_string_value(config, "IP_LFS"),
			config_get_string_value(config, "PUERTO_LFS"));
	log_info(logger_MEMORIA, "SE CONECTO CN LFS");
}

void escucharMultiplesClientes() {
	int descriptorServidor = iniciar_servidor(config_get_string_value(config, "PUERTO"), config_get_string_value(config, "IP"));
	log_info(logger_MEMORIA, "Memoria lista para recibir al kernel");

	/* fd = file descriptor (id de Socket)
	 * fd_set es Set de fd's (una coleccion)*/

	descriptoresClientes = list_create();	// Lista de descriptores de todos los clientes conectados (podemos conectar infinitos clientes)
	int numeroDeClientes = 0;						// Cantidad de clientes conectados
	int valorMaximo = 0;							// Descriptor cuyo valor es el mas grande (para pasarselo como parametro al select)
	t_paquete* paqueteRecibido;

	while(1) {

		eliminarClientesCerrados(descriptoresClientes, &numeroDeClientes);	// Se eliminan los clientes que tengan un -1 en su fd
		FD_ZERO(&descriptoresDeInteres); 									// Inicializamos descriptoresDeInteres
		FD_SET(descriptorServidor, &descriptoresDeInteres);					// Agregamos el descriptorServidor a la lista de interes

		for (int i=0; i< numeroDeClientes; i++) {
			FD_SET((int) list_get(descriptoresClientes,i), &descriptoresDeInteres); // Agregamos a la lista de interes, los descriptores de los clientes
		}

		valorMaximo = maximo(descriptoresClientes, descriptorServidor, numeroDeClientes); // Se el valor del descriptor mas grande. Si no hay ningun cliente, devuelve el fd del servidor
		select(valorMaximo + 1, &descriptoresDeInteres, NULL, NULL, NULL); 				  // Espera hasta que algún cliente tenga algo que decir

		for(int i=0; i<numeroDeClientes; i++) {
			if (FD_ISSET((int) list_get(descriptoresClientes,i), &descriptoresDeInteres)) {   // Se comprueba si algún cliente ya conectado mando algo
				paqueteRecibido = recibir((int) list_get(descriptoresClientes,i)); // Recibo de ese cliente en particular
				cod_request palabraReservada = paqueteRecibido->palabraReservada;
				char* request = paqueteRecibido->request;
				printf("El codigo que recibi es: %s \n", request);
				printf("Del fd %i \n", (int) list_get(descriptoresClientes,i)); // Muestro por pantalla el fd del cliente del que recibi el mensaje
				interpretarRequest(palabraReservada,request,HIMSELF, i);

			}
		}

		if(FD_ISSET (descriptorServidor, &descriptoresDeInteres)) {
			int descriptorCliente = esperar_cliente(descriptorServidor); 					  // Se comprueba si algun cliente nuevo se quiere conectar
			numeroDeClientes = (int) list_add(descriptoresClientes, (int*) descriptorCliente); // Agrego el fd del cliente a la lista de fd's
			numeroDeClientes++;
		}
	}
}

void interpretarRequest(cod_request palabraReservada,char* request,t_caller caller, int i) {


	switch(palabraReservada) {

		case SELECT:
			log_info(logger_MEMORIA, "Me llego un SELECT");
			procesarSelect(palabraReservada, request, caller, i);
			break;
		case INSERT:
			log_info(logger_MEMORIA, "Me llego un INSERT");
			//procesarInsert(palabraReservada, request);
			break;
		case CREATE:
			log_info(logger_MEMORIA, "Me llego un CREATE");
			break;
		case DESCRIBE:
			log_info(logger_MEMORIA, "Me llego un DESCRIBE");
			break;
		case DROP:
			log_info(logger_MEMORIA, "Me llego un DROP");
			break;
		case JOURNAL:
			log_info(logger_MEMORIA, "Me llego un JOURNAL");
			break;
		case NUESTRO_ERROR:
			if(caller == HIMSELF){
				log_error(logger_MEMORIA, "el cliente se desconecto. Terminando servidor");
				int valorAnterior = (int) list_replace(descriptoresClientes, i, (int*) -1); // Si el cliente se desconecta le pongo un -1 en su fd}
				// TODO: Chequear si el -1 se puede castear como int*
				break;
			}
			else{
				break;
			}
		default:
			log_warning(logger_MEMORIA, "Operacion desconocida. No quieras meter la pata");
			break;
	}
	log_info(logger_MEMORIA, "(MENSAJE DE KERNEL)");
}


char* intercambiarConFileSystem(cod_request palabraReservada, char* request){
	t_paquete* paqueteRecibido;

	enviar(palabraReservada, request, conexionLfs);

	sem_post(&semLeerDeConsola);
	paqueteRecibido = recibir(conexionLfs);

	return paqueteRecibido->request;

}

/*procesarSelect()
 * Parametros:
 * 	-> char* ::  request
 * 	-> cod_request :: palabraReservada
 * Descripcion: Permite obtener el valor de la key consultada de una tabla.
 * Return:
 * 	-> :: void */
void procesarSelect(cod_request palabraReservada, char* request, t_caller caller, int i) {
	t_pagina* pagEncontrada;
	char* valorEncontrado;
	char* valorDeLFS;
	char** parametros = obtenerParametros(request);
	puts("ANTES DE IR A BUSCAR A CACHE");
	if(estaEnMemoria(palabraReservada, parametros,&valorEncontrado,&pagEncontrada)!= FALSE) {
		log_info(logger_MEMORIA, "LO ENCONTRE EN CACHEE!");
		enviarAlDestinatarioCorrecto(palabraReservada,request, valorEncontrado,caller, (int) list_get(descriptoresClientes,i));


	} else {

		// en caso de no existir el segmento o la tabla en MEMORIA, se lo solicta a LFS
		valorDeLFS = intercambiarConFileSystem(palabraReservada,request);
		enviarAlDestinatarioCorrecto(palabraReservada,request, valorDeLFS,caller, (int) list_get(descriptoresClientes,i));
	}

}

int estaEnMemoria(cod_request palabraReservada, char** parametros,char** valorEncontrado, t_pagina** pagEncontrada){
	log_info(logger_MEMORIA,"ENTRE A ESTAE EN MEMORIA");
	char* tablaABuscar= parametros[0];
	 char *ptrResto;
	int keyABuscar = (int)strtol((char*)parametros[1],&ptrResto,16);

	if(strcmp(tablaABuscar,"tablaA")==0)
	{
		if(keyABuscar==tablaA->pagina->key){
			*pagEncontrada=tablaA->pagina;
			*valorEncontrado=(char*)tablaA->pagina->value;
			printf("LA RTA ES %s \n",*valorEncontrado);

		return TRUE;
		}
	}else{
		return FALSE;
	}

}

 void enviarAlDestinatarioCorrecto(cod_request palabraReservada,char* request,char* valorAEnviar,t_caller caller,int i){
		if(caller == HIMSELF) {
				enviar(palabraReservada, valorAEnviar, (int) list_get(descriptoresClientes,i));
			} else if(caller == CONSOLE) {
				log_info(logger_MEMORIA, "La respuesta del ", request, " es ", valorAEnviar);
			} else {
	//			log_error();
			}
 }

/*procesarInsert()
 * Parametros:
 * 	-> char* ::  request
 * 	-> cod_request :: palabraReservada
 * Descripcion: se va a fijar si existe el segmento de la tabla ,que se quiere hacer insert,
 * 				en la memoria principal.
 * 				Si Existe dicho segmento, busca la key (y de encontrarla,
 * 				actualiza su valor insertando el timestap actual).Y si no encuentra, solicita pag
 * 				y la crea. Pero de no haber pag suficientes, se hace journaling.
 * 				Si no se encuentra el segmento,solicita un segment para crearlo y lo hace.Y, en
 * Return:
 * 	-> :: void */
void procesarInsert(cod_request palabraReservada, char* request, t_caller caller) {
		t_pagina* pagEncontrada;
		char* valorEncontrado;
		char* valorDeLFS;
		char** parametros = obtenerParametros(request);
		char* pagina; //t_pagina

		puts("ANTES DE IR A BUSCAR A CACHE");
		if(estaEnMemoria(palabraReservada, parametros,&valorEncontrado,&pagEncontrada)!= FALSE) {
//		KEY encontrada	-> modifico timestamp
//						-> modifico valor
//						-> modifico flagTabla

			log_info(logger_MEMORIA, "LO ENCONTRE EN CACHEE!");
			printf("LA RTA ES %s \n",pagina);
//			timestampAhora(pagina);
		}else if((pagina= estaEnCache(palabraReservada, parametros))== FALSE){
//		KEY no encontrada -> solicito nueva pagina
			//list

		}
}

t_pagina* timestampAhora (t_pagina* pagina){
	unsigned long long nuevoTimes = obtenerHoraActual();
	pagina->timestamp = nuevoTimes;
	return pagina;
}

t_pagina* crearPagina(int clave, char* valor){ //VER TIPO DE KEY
	t_pagina* nuevaPagina;
	nuevaPagina->timestamp = obtenerHoraActual();
	nuevaPagina->key = clave;
	nuevaPagina->value = valor;
	return nuevaPagina;
}

t_tablaDePaginas* crearTablaDePagina(int numeroDePag){
	t_tablaDePaginas* nuevaTablaDePaginas;
	nuevaTablaDePaginas->numeroDePag = numeroDePag;
	nuevaTablaDePaginas->pagina = list_create();
	nuevaTablaDePaginas->modificado = SINMODIFICAR;
	return nuevaTablaDePaginas;
}





