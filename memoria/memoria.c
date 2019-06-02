#include "memoria.h"
#include <stdlib.h>

int main(void) {

//--------------------------------RESERVA DE MEMORIA------------------------------------------------------------
	//TODO solo se debe reservar el tam max y DELEGARSE A UNA FUNCION
	pag =(t_pagina*) malloc(sizeof(t_pagina));
	elementoA1 =malloc(sizeof(t_elemTablaDePaginas));
	tablaA = malloc(sizeof(t_tablaDePaginas));
	tablaDeSegmentos = malloc(sizeof(t_segmentos));

//--------------------------------AUXILIAR: creacion de tabla/pag/elemento --------------------------------------

	tablaDeSegmentos->segmentos =list_create();
	tablaA->elementosDeTablaDePagina = list_create();	//list_create() HACE UN MALLOC
	tablaA->nombre= strdup("TablaA");
	pag->timestamp = 123456789;
	pag->key=1;
	pag->value=strdup("hola");
	elementoA1->numeroDePag=1;
	elementoA1->pagina= pag;
	elementoA1->modificado = SINMODIFICAR;

	list_add(tablaA->elementosDeTablaDePagina, elementoA1);
	list_add(tablaDeSegmentos->segmentos,tablaA);

//--------------------------------INICIO DE MEMORIA ---------------------------------------------------------------
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/memoria/memoria.config");
	logger_MEMORIA = log_create("memoria.log", "Memoria", 1, LOG_LEVEL_DEBUG);

//--------------------------------CONEXION CON LFS ---------------------------------------------------------------
	conectarAFileSystem();

//--------------------------------SEMAFOROS-HILOS ----------------------------------------------------------------
	//	SEMAFOROS
	//sem_init(&semLeerDeConsola, 0, 1);
	sem_init(&semEnviarMensajeAFileSystem, 0, 0);
	//pthread_mutex_init(&terminarHilo, NULL);

	// 	HILOS
	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);
	pthread_create(&hiloEscucharMultiplesClientes, NULL, (void*)escucharMultiplesClientes, NULL);

	pthread_detach(hiloLeerDeConsola);
	pthread_join(hiloEscucharMultiplesClientes, NULL);

//-------------------------------- -PARTE FINAL DE MEMORIA---------------------------------------------------------
	liberar_conexion(conexionLfs);
	//liberarMemoria();
	log_destroy(logger_MEMORIA);
 	config_destroy(config);
 	FD_ZERO(&descriptoresDeInteres);// TODO momentaneamente, asi cerramos todas las conexiones

	return 0;
}

/* leerDeConsola()
 * Parametros:
 * 	->  ::  void
 * Descripcion: Lee la request de consola.
 * 				Valida el caso de que s ehaya ingresado SALIDA (por lo que setea el flagTerminaHiloMultiplesClientes en 1
 * Return:
 * 	-> codPalabraReservada :: void
 * VALGRIND:: SI */
void leerDeConsola(void){
	char* mensaje;
	log_info(logger_MEMORIA, "Vamos a leer de consola");
	while (1) {
		//sem_wait(&semLeerDeConsola);
		mensaje = readline(">");
		if (!strcmp(mensaje, "\0")) {
			break;
		}
//		if(validarRequest(mensaje)== SALIDA){
//			pthread_mutex_lock(&terminarHilo);
//			flagTerminarHiloMultiplesClientes =1;
//			pthread_mutex_unlock(&terminarHilo);
//			break;
//		}
		validarRequest(mensaje);
		free(mensaje);
	}
}

/* validarRequest()
 * Parametros:
 * 	->  mensaje :: char*
 * Descripcion: Cachea si el codigo de validacion encontrado.
 * 				En caso de ser un codigo valido, se proede a interpretar la request y se devuelve EXIT_SUCCESS.
 * 				En caso de error se deuvuelve NUESTRO_ERROR y se logea el mismo.
 * 				En caso de de ser salida, se devuelve SALIDA.
 *
 * Return:
 * 	-> resultadoVlidacion :: int
 * VALGRIND:: NO */
int validarRequest(char* mensaje){
	int codValidacion;
	char** request = string_n_split(mensaje, 2, " ");
	codValidacion = validarMensaje(mensaje, MEMORIA, logger_MEMORIA);
	cod_request palabraReservada = obtenerCodigoPalabraReservada(request[0],MEMORIA);
	switch(codValidacion){
		case EXIT_SUCCESS:
			interpretarRequest(palabraReservada, mensaje, CONSOLE,-1);
			return EXIT_SUCCESS;
			break;
		case NUESTRO_ERROR:
			//TODO es la q hay q hacerla generica
			log_error(logger_MEMORIA, "La request no es valida");
			return NUESTRO_ERROR;
			break;
		case SALIDA:
			return SALIDA;
			break;
		default:
			return NUESTRO_ERROR;
			break;
	}
}

/* conectarAFileSystem()
 * Parametros:
 * 	->  :: void
 * Descripcion: Se crea la conexion con lissandraFileSystem
 *
 * Return:
 * 	-> :: void
 * VALGRIND:: SI */
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
	int numeroDeClientes = 0;				// Cantidad de clientes conectados
	int valorMaximo = 0;					// Descriptor cuyo valor es el mas grande (para pasarselo como parametro al select)
	t_paquete* paqueteRecibido;

	while(1) {
		//Varibale global, el hilo de leerConsola, cuando reccibe "SALIDA" lo setea en 1
//		pthread_mutex_lock(&terminarHilo);
//		if(flagTerminarHiloMultiplesClientes ==1){
//			pthread_mutex_unlock(&terminarHilo);
//			log_info(logger_MEMORIA,"ENTRE AL WHILE 1");
//			break;
//		}else{
//			pthread_mutex_unlock(&terminarHilo);

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
			}//fin for

			if(FD_ISSET (descriptorServidor, &descriptoresDeInteres)) {
				int descriptorCliente = esperar_cliente(descriptorServidor); 					  // Se comprueba si algun cliente nuevo se quiere conectar
				numeroDeClientes = (int) list_add(descriptoresClientes, (int*) descriptorCliente); // Agrego el fd del cliente a la lista de fd's
				numeroDeClientes++;
			}
	//	}
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
		case SALIDA:
			log_info(logger_MEMORIA,"HaS finalizado el componente MEMORIA");
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

/*intercambiarConFileSystem()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 * 	-> char* ::  request
 * Descripcion: Permite enviar la request (recibida por parametro) a LFS y espera la respuesta de ella.
 * Return:
 * 	-> paqueteRecibido:: char* */
char* intercambiarConFileSystem(cod_request palabraReservada, char* request){
	t_paquete* paqueteRecibido;

	enviar(palabraReservada, request, conexionLfs);
	//sem_post(&semLeerDeConsola);
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
	int resultadoCache;
	switch(consistenciaMemoria){
		case SC:
		case SHC:
			log_info(logger_MEMORIA,"ME LO TIENE QUE DECIR LFS");
			//TODO CUANDO LFS PUEDA HACER INSERT CORERCTAMENTE, HAY QUE DESCOMNTARLO
			// en caso de no existir el segmento o la tabla en MEMORIA, se lo solicta a LFS
			//valorDeLFS = intercambiarConFileSystem(palabraReservada,request);
			//enviarAlDestinatarioCorrecto(palabraReservada,request, valorDeLFS,caller, (int) list_get(descriptoresClientes,i));
			//TODO GUARDAR EN CACHE RTA DE LFS
			break;
		case EC:
			resultadoCache= estaEnMemoria(palabraReservada, parametros,&valorEncontrado,&elementoEncontrado);
			if(resultadoCache == EXIT_SUCCESS ) {
				log_info(logger_MEMORIA, "LO ENCONTRE EN CACHEE!");
				enviarAlDestinatarioCorrecto(palabraReservada,request, valorEncontrado,caller, (int) list_get(descriptoresClientes,i));
				free(elementoEncontrado);
				free(parametros);
			} else {
				log_info(logger_MEMORIA,"ME LO TIENE QUE DECIR LFS");
				//TODO CUANDO LFS PUEDA HACER INSERT CORERCTAMENTE, HAY QUE DESCOMNTARLO
				// en caso de no existir el segmento o la tabla en MEMORIA, se lo solicta a LFS
				//valorDeLFS = intercambiarConFileSystem(palabraReservada,request);
				//enviarAlDestinatarioCorrecto(palabraReservada,request, valorDeLFS,caller, (int) list_get(descriptoresClientes,i));
				//TODO GUARDAR EN CACHE RTA DE LFS
				valorDeLFS="SELECT tablaA 2 chau 1234567898";
				char** resultadoSeparadoLFS= separarRequest(valorDeLFS);
				switch(resultadoCache){
					case(KEYINEXISTENTE):
							break;
					case(SEGMENTOINEXISTENTE):
							break;
				}
			}
			break;
		default:
			log_info(logger_MEMORIA, "NO se le ha asignado un tipo de consistencia a la memoria, por lo que no puede responder la consulta: ", request);
			break;
	}


}

/*estaEnMemoria()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 * 	-> char** ::  parametros de la request
 * 	-> char** :: valorEncontrado- se modifica por referencia
 * 	-> t_elemTablaDePaginas** :: elemento (que contiene el puntero a la pag econtrada)- se modifica por referecia.
 * Descripcion: Revisa si la tabla y key solicitada se encuentran que cache.
 * 				En caso de que sea cierto, modifica valorEncontrado y elementoEncontrado; devuelce operacion exitosa.
 * 				En ccaso de que no exista la tabla||key devuelve el error correspondiente.
 * Return:
 * 	-> resultado de la operacion:: int */
int estaEnMemoria(cod_request palabraReservada, char** parametros,char** valorEncontrado,t_elemTablaDePaginas** elementoEncontrado){
	t_tablaDePaginas* tablaDeSegmentosEnCache = malloc(sizeof(t_tablaDePaginas));
	t_elemTablaDePaginas* elementoDePagEnCache = malloc(sizeof(t_elemTablaDePaginas));
	char* segmentoABuscar=strdup(parametros[0]);
	uint16_t keyABuscar= convertirKey(parametros[1]);

	int encontrarTabla(t_tablaDePaginas* tablaDePaginas){
		return string_equals_ignore_case(tablaDePaginas->nombre, segmentoABuscar);
	}

	tablaDeSegmentosEnCache= list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);
	if(tablaDeSegmentosEnCache!= NULL){

		int encontrarElemDePag(t_elemTablaDePaginas* elemDePagina)
					{
						return (elemDePagina->pagina->key == keyABuscar);
					}

		elementoDePagEnCache= list_find(tablaDeSegmentosEnCache->elementosDeTablaDePagina,(void*)encontrarElemDePag);
		if(elementoDePagEnCache !=NULL){ //registro = pagina
			*elementoEncontrado=elementoDePagEnCache;
			*valorEncontrado = strdup(elementoDePagEnCache->pagina->value);
			return EXIT_SUCCESS;
		}else{
			return KEYINEXISTENTE;
		}
	}else{
		return SEGMENTOINEXISTENTE;
	}
}

t_tablaDePaginas* encontrarSegmento(char* segmentoABuscar){
	int encontrarTabla(t_tablaDePaginas* tablaDePaginas){
		return string_equals_ignore_case(tablaDePaginas->nombre, segmentoABuscar);
	}

	return list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);
}
/* enviarAlDestinatarioCorrecto()
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
 void enviarAlDestinatarioCorrecto(cod_request palabraReservada,char* request,char* valorAEnviar,t_caller caller,int socketKernel){
	if(caller == ANOTHER_COMPONENT) {
		enviar(palabraReservada, valorAEnviar, socketKernel);
	} else if(caller == CONSOLE) {
		printf("La respuesta del request %s es %s \n", request, valorAEnviar);
	}
 }


///* procesarInsert()
// * Parametros:
// * 	-> cod_request :: palabraReservada
// *	-> char* :: request
// *	->
// * Descripcion: se va a fijar si existe el segmento de la tabla ,que se quiere hacer insert,
// * 				en la memoria principal.
// * 				Si Existe dicho segmento, busca la key (y de encontrarla,
// * 				actualiza su valor insertando el timestap actual).Y si no encuentra, solicita pag
// * 				y la crea. Pero de no haber pag suficientes, se hace journaling.
// * 				Si no se encuentra el segmento,solicita un segment para crearlo y lo hace.Y, en
// * Return:
// * 	-> :: void */
void procesarInsert(cod_request palabraReservada, char* request, t_caller caller) {
		t_elemTablaDePaginas* elementoEncontrado;
		char* valorEncontrado;
		char** parametros = obtenerParametros(request);
		char* newTabla = strdup(parametros[0]);
		char* newKeyChar = strdup(parametros[1]);
		int newKey = convertirKey(newKeyChar);
		char* newValue = strdup(parametros[2]);

		puts("ANTES DE IR A BUSCAR A CACHE");

		if(estaEnMemoria(palabraReservada, parametros,&valorEncontrado,&elementoEncontrado)== EXIT_SUCCESS) {
//			KEY encontrada	-> modifico timestamp
//							-> modifico valor
//							-> modifico flagTabla
			actualizarElementoEnTablaDePagina(elementoEncontrado,newValue);
			log_info(logger_MEMORIA, "KEY encontrada: pagina modificada");
		}else if(estaEnMemoria(palabraReservada, parametros,&valorEncontrado,&elementoEncontrado)== KEYINEXISTENTE){
//			KEY no encontrada -> nueva pagina solicitada
//TODO:							si faltaEspacio JOURNAL
			list_add(tablaA->elementosDeTablaDePagina,crearElementoEnTablaDePagina(newKey,newValue));
			log_info(logger_MEMORIA, "KEY no encontrada: nueva pagina creada");
		}else if(estaEnMemoria(palabraReservada, parametros,&valorEncontrado,&elementoEncontrado)== TABLAINEXISTENTE){
//TODO:		TABLA no encontrada -> nuevo segmento
			t_tablaDePaginas* nuevaTablaDePagina = crearTablaDePagina(newTabla,newKey,newValue);
			list_add(tablaDeSegmentos->segmentos,nuevaTablaDePagina);
			list_add(nuevaTablaDePagina->elementosDeTablaDePagina,crearElementoEnTablaDePagina(newKey,newValue));
			log_info(logger_MEMORIA, "TABLA no encontrada: nuevo segmento creado");
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

t_elemTablaDePaginas* crearElementoEnTablaDePagina(uint16_t newKey, char* newValue){
	t_elemTablaDePaginas* newElementoDePagina= (t_elemTablaDePaginas*)malloc(sizeof(t_elemTablaDePaginas));
	newElementoDePagina->numeroDePag = rand();
	newElementoDePagina->pagina = crearPagina(newKey,newValue);
	newElementoDePagina->modificado = SINMODIFICAR;
	return newElementoDePagina;
}

void actualizarElementoEnTablaDePagina(t_elemTablaDePaginas* elemento, char* newValue){
	actualizarPagina(elemento->pagina,newValue);
	elemento->modificado = MODIFICADO;
}

t_tablaDePaginas* crearTablaDePagina(char* nuevaTabla, uint16_t newKey, char* newValue){
	t_tablaDePaginas* newTablaDePagina = (t_tablaDePaginas*)malloc(sizeof(t_tablaDePaginas));
	newTablaDePagina->nombre=strdup(nuevaTabla);
	newTablaDePagina->elementosDeTablaDePagina=list_create();
	return newTablaDePagina;
}


// FUNCION QUE QUEREMOS UTILIZAR CUANDO FINALIZAN LOS DOS HILOS
void liberarMemoria(){
	void liberarElementoDePag(t_elemTablaDePaginas* self){
		 free(self->pagina->value);
		 free(self->pagina);
	 }
	list_clean_and_destroy_elements(tablaA->elementosDeTablaDePagina, (void*)liberarElementoDePag);
}
