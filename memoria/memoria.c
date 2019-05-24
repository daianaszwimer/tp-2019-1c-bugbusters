#include "memoria.h"
#include <stdlib.h>

int main(void) {

	pag = malloc(sizeof(t_pagina));
	elementoA1 =malloc(sizeof(t_elemTablaDePaginas));
	tablaA = malloc(sizeof(t_tablaDePaginas));


	tablaA->elementosDeTablaDePagina = list_create();	//list_create() HACE UN MALLOC

	pag->timestamp = 123456789;
	pag->key=1;
	pag->value=strdup("hola");

	elementoA1->numeroDePag=1;
	elementoA1->pagina= pag;
	elementoA1->modificado = SINMODIFICAR;


	//printf("%llu \n", obtenerHoraActual());

	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/memoria/memoria.config");
	logger_MEMORIA = log_create("memoria.log", "Memoria", 1, LOG_LEVEL_DEBUG);
	//datoEstaEnMemoria = TRUE;
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

void validarRequest(char* mensaje){
	int codValidacion;
	char** request = string_n_split(mensaje, 2, " ");
	codValidacion = validarMensaje(mensaje, MEMORIA, logger_MEMORIA);
	cod_request palabraReservada = obtenerCodigoPalabraReservada(request[0],MEMORIA);
	switch(codValidacion){
		case EXIT_SUCCESS:
			interpretarRequest(palabraReservada, mensaje, CONSOLE,-1);
			break;
		case NUESTRO_ERROR:
			//es la q hay q hacerla generica
			log_error(logger_MEMORIA, "La request no es valida");
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
				interpretarRequest(palabraReservada,request,ANOTHER_COMPONENT, i);

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

log_info(logger_MEMORIA,"entre a interpretarr request");
	switch(palabraReservada) {

		case SELECT:
			log_info(logger_MEMORIA, "Me llego un SELECT");
			procesarSelect(palabraReservada, request, caller, i);
			break;
		case INSERT:
			log_info(logger_MEMORIA, "Me llego un INSERT");
			procesarInsert(palabraReservada, request, caller);
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
			if(caller == ANOTHER_COMPONENT){
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
	t_elemTablaDePaginas* elementoEncontrado;
	char* valorEncontrado;
	char* valorDeLFS;
	char** parametros = obtenerParametros(request);
	puts("ANTES DE IR A BUSCAR A CACHE");
	if(estaEnMemoria(palabraReservada, parametros,&valorEncontrado,&elementoEncontrado)!= FALSE) {
		log_info(logger_MEMORIA, "LO ENCONTRE EN CACHEE!");
		enviarAlDestinatarioCorrecto(palabraReservada,request, valorEncontrado,caller, (int) list_get(descriptoresClientes,i));
		//hay q liberar

	} else {

		// en caso de no existir el segmento o la tabla en MEMORIA, se lo solicta a LFS
		valorDeLFS = intercambiarConFileSystem(palabraReservada,request);
		enviarAlDestinatarioCorrecto(palabraReservada,request, valorDeLFS,caller, (int) list_get(descriptoresClientes,i));
	}

}

int estaEnMemoria(cod_request palabraReservada, char** parametros,char** valorEncontrado,t_elemTablaDePaginas** elementoEncontrado){
	char* segmentoABuscar= parametros[0];
	int keyABuscar= convertirKey(parametros[1]);

	if(strcmp(segmentoABuscar,"tablaA")==0) //tabla = segmento (esto hay que cambiarlo)
	{
		if(keyABuscar==(int)elementoA1->pagina->key){ //registro = pagina
			*elementoEncontrado=elementoA1;
			*valorEncontrado = strdup(elementoA1->pagina->value);
			//printf("LA RTA ES %s \n",*valorEncontrado);
			return TRUE;
		}else{
			return FALSE;
		}
	}else{
		return NUESTRO_ERROR;
	}
}

 void enviarAlDestinatarioCorrecto(cod_request palabraReservada,char* request,char* valorAEnviar,t_caller caller,int socketKernel){
	if(caller == ANOTHER_COMPONENT) {
		enviar(palabraReservada, valorAEnviar, socketKernel);
	} else if(caller == CONSOLE) {
		printf("La respuesta del request %s es %s \n", request, valorAEnviar);
	}
 }

/* procesarInsert()
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
		t_elemTablaDePaginas* elementoEncontrado;
		char* valorEncontrado;
		char** parametros = obtenerParametros(request);
		char* newKeyChar = parametros[1];
		int newKey = convertirKey(newKeyChar);
		char* newValue = parametros[2];

		puts("ANTES DE IR A BUSCAR A CACHE");

		if(estaEnMemoria(palabraReservada, parametros,&valorEncontrado,&elementoEncontrado)!= FALSE) {
//			KEY encontrada	-> modifico timestamp
//							-> modifico valor
//							-> modifico flagTabla
			actualizarElementoEnTablaDePagina(elementoEncontrado,newValue);
			log_info(logger_MEMORIA, "KEY encontrada: pagina modificada");
		}else if(estaEnMemoria(palabraReservada, parametros,&valorEncontrado,&elementoEncontrado)== FALSE){
//			KEY no encontrada -> nueva pagina solicitada
//TODO:							si faltaEspacio JOURNAL
			crearElementoEnTablaDePagina(tablaA,newKey,newValue);
			log_info(logger_MEMORIA, "KEY no encontrada: nueva pagina creada");
		}else{
//TODO:		TABLA no encontrada -> nuevo segmento

		}
}

t_pagina* crearPagina(uint16_t newKey, char* newValue){
	t_pagina* nuevaPagina= (t_pagina*)malloc(sizeof(t_pagina));
	nuevaPagina->timestamp = obtenerHoraActual();
	nuevaPagina->key = newKey;
	nuevaPagina->value = newValue;
	return nuevaPagina;
}

void actualizarPagina (t_pagina* pagina, char* newValue){
	unsigned long long newTimes = obtenerHoraActual();
	pagina->timestamp = newTimes;
	pagina->value = newValue;
}

void crearElementoEnTablaDePagina(t_tablaDePaginas* tablaDestino, uint16_t newKey, char* newValue){
	t_elemTablaDePaginas* newElementoDePagina= (t_elemTablaDePaginas*)malloc(sizeof(t_elemTablaDePaginas));
	newElementoDePagina->numeroDePag = rand();
	newElementoDePagina->pagina = crearPagina(newKey,newValue);
	newElementoDePagina->modificado = SINMODIFICAR;
	list_add(tablaDestino->elementosDeTablaDePagina,newElementoDePagina);
}

void actualizarElementoEnTablaDePagina(t_elemTablaDePaginas* elemento, char* newValue){
	actualizarPagina(elemento->pagina,newValue);
	elemento->modificado = MODIFICADO;
}




