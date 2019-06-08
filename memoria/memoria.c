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
		mensaje=NULL;
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
	int tamanioMax = 10; //TODO lo pasa LFS X HANDSHAKE
	int codValidacion;
	char** request = string_n_split(mensaje, 2, " ");
	char** requestSeparada= separarRequest(mensaje);
	codValidacion = validarMensaje(mensaje, MEMORIA, logger_MEMORIA);
	int palabraReservada = obtenerCodigoPalabraReservada(request[0],MEMORIA);
	switch(codValidacion){
		case EXIT_SUCCESS:
			if(palabraReservada == INSERT && validarValue(mensaje,requestSeparada[3],tamanioMax,logger_MEMORIA) == NUESTRO_ERROR){
				return NUESTRO_ERROR;
				break;
			}
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
	liberarArrayDeChar(request);
	request =NULL;
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
					free(request);
					request=NULL;

				}
			}//fin for

			if(FD_ISSET (descriptorServidor, &descriptoresDeInteres)) {
				int descriptorCliente = esperar_cliente(descriptorServidor); 					  // Se comprueba si algun cliente nuevo se quiere conectar
				enviarHandshakeMemoria("1, 2, 3", "4, 5, 6", descriptorCliente);
				numeroDeClientes = (int) list_add(descriptoresClientes, (int*) descriptorCliente); // Agrego el fd del cliente a la lista de fd's
				numeroDeClientes++;
			}
	//	}
	}
	eliminar_paquete(paqueteRecibido);
	paqueteRecibido=NULL;

}


/* interpretarRequest()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 * 	-> char* :: request
 * 	-> t_caller :: caller
 * 	-> int :: i (socket_kernel)
 * Descripcion: El coponente de memoria interpreta la reques que le llego desde cualquier caller (kernel o consola)
 * 				y ,segun la palabra reservada,delega al correspondiente procesar
 * Return:
 * 	-> :: void
 * VALGRIND:: SI */
void interpretarRequest(int palabraReservada,char* request,t_caller caller, int i) {

	consistencia consistenciaMemoria;
	if(caller== ANOTHER_COMPONENT){
		consistenciaMemoria = palabraReservada;
	}else{
		consistenciaMemoria = EC;
	}
	char** requestSeparada = string_n_split(request, 2, " ");
	int codRequest = obtenerCodigoPalabraReservada(requestSeparada[0],MEMORIA);

	log_info(logger_MEMORIA,"entre a interpretarr request");
	switch(codRequest) {

		case SELECT:
			log_info(logger_MEMORIA, "Me llego un SELECT");
			procesarSelect(codRequest, request,consistenciaMemoria, caller, i);
			break;
		case INSERT:
			log_info(logger_MEMORIA, "Me llego un INSERT");
			procesarInsert(codRequest, request,consistenciaMemoria, caller, i);
			break;
		case CREATE:
			log_info(logger_MEMORIA, "Me llego un CREATE");
			procesarCreate(codRequest, request,consistenciaMemoria, caller, i);
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
}

/*intercambiarConFileSystem()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 * 	-> char* ::  request
 * Descripcion: Permite enviar la request (recibida por parametro) a LFS y espera la respuesta de ella.
 * Return:
 * 	-> paqueteRecibido:: char*
 * VALGRIND:: NO */
t_paquete* intercambiarConFileSystem(cod_request palabraReservada, char* request){
	t_paquete* paqueteRecibido;

	enviar(palabraReservada, request, conexionLfs);
	//sem_post(&semLeerDeConsola);
	paqueteRecibido = recibir(conexionLfs);

	return paqueteRecibido;

}

/*procesarSelect()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 * 	-> char* ::  request
 * 	-> t_caller :: caller
 * 	-> int :: i (socket_kernel)
 * Descripcion: En caso de que exista, permite obtener el valor de la key consultada de una tabla.
 * 				SC || SHC : siempre consulta la existencia de la tabla y la key consultada a LFS. Y,en caso de que alli
 * 							no exista, se informa ello.
 * 							En caso de que exista, guardamos el resultado devuelto por LFS en cache (que nos sirve para
 * 							el insert).
 * 				EC : -Revisa si la tabla y value se encuentran en memoria.E n caso de encontrarlo en cache, devuelve el
 * 							value asosciado.
 * 							En caso de no existir la key o value, realiza procedimiento de SC||SHC
 * Return:
 * 	-> :: void
 * VALGRIND:: NO */
void procesarSelect(cod_request palabraReservada, char* request,consistencia consistenciaMemoria,t_caller caller, int i) {


//---------------CASOS DE PRUEBA------------------------------

	t_paquete* valorDeLFS=malloc(sizeof(t_paquete));
//valorDeLF->palabraReservada= 0;
//valorDeLF->tamanio=100;
//valorDeLF->request=strdup("tablaB 2 chau 454462636");
//-------------------------------------------------------------
	t_elemTablaDePaginas* elementoEncontrado = malloc(sizeof(t_elemTablaDePaginas));
	t_paquete* valorEncontrado=malloc(sizeof(t_paquete));


	int resultadoCache;

	if(consistenciaMemoria == EC || caller == CONSOLE){		// en caso de no existir el segmento o la tabla en MEMORIA, se lo solicta a LFS
		resultadoCache= estaEnMemoria(palabraReservada, request,&valorEncontrado,&elementoEncontrado);
		if(resultadoCache == EXIT_SUCCESS ) {
			log_info(logger_MEMORIA, "LO ENCONTRE EN CACHEE!");
			enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, valorEncontrado,caller, (int) list_get(descriptoresClientes,i));
			//free(elementoEncontrado);
			//free(parametros);
		} else {// en caso de no existir el segmento o la tabla en MEMORIA, se lo solicta a LFS
			log_info(logger_MEMORIA,"ME LO TIENE QUE DECIR LFS");
			valorDeLFS = intercambiarConFileSystem(palabraReservada,request);
			enviarAlDestinatarioCorrecto(palabraReservada, valorDeLFS->palabraReservada,request, valorDeLFS,caller, (int) list_get(descriptoresClientes,i));
			guardarRespuestaDeLFSaCACHE(valorDeLFS, resultadoCache);
		}
	}else if(consistenciaMemoria==SC || consistenciaMemoria == SHC){
		log_info(logger_MEMORIA,"ME LO TIENE QUE DECIR LFS");
		//TODO CUANDO LFS PUEDA HACER INSERT CORERCTAMENTE, HAY QUE DESCOMNTARLO
		valorDeLFS = intercambiarConFileSystem(palabraReservada,request);
		enviarAlDestinatarioCorrecto(palabraReservada, valorDeLFS->palabraReservada,request, valorDeLFS,caller, (int) list_get(descriptoresClientes,i));
		resultadoCache= estaEnMemoria(palabraReservada, request,&valorEncontrado,&elementoEncontrado);
		guardarRespuestaDeLFSaCACHE(valorDeLFS, resultadoCache);
	}else{
			log_info(logger_MEMORIA, "NO se le ha asignado un tipo de consistencia a la memoria, por lo que no puede responder la consulta: ", request);
	}

	eliminar_paquete(valorDeLFS);
	valorDeLFS=NULL;

}



/*estaEnMemoria()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 * 	-> char* :: request
 * 	-> t_paquete** :: valorEncontrado- se modifica por referencia
 * 	-> t_elemTablaDePaginas** :: elemento (que contiene el puntero a la pag econtrada)- se modifica por referecia.
 * Descripcion: Revisa si la tabla y key solicitada se encuentran que cache.
 * 				En caso de que sea cierto, guarda valorEncontrado y elementoEncontrado; devolviendo operacion exitosa.
 * 				En caso de que no exista la tabla||key devuelve el error correspondiente.
 * Return:
 * 	-> int :: resultado de la operacion
 * 	VALGRIND :NO */
int estaEnMemoria(cod_request palabraReservada, char* request,t_paquete** valorEncontrado,t_elemTablaDePaginas** elementoEncontrado){
	t_tablaDePaginas* tablaDeSegmentosEnCache = malloc(sizeof(t_tablaDePaginas));
	t_elemTablaDePaginas* elementoDePagEnCache = malloc(sizeof(t_elemTablaDePaginas));
	t_paquete* paqueteAuxiliar;

	char** parametros = separarRequest(request);
	char* segmentoABuscar=strdup(parametros[1]);
	uint16_t keyABuscar= convertirKey(parametros[2]);

	int encontrarTabla(t_tablaDePaginas* tablaDePaginas){
		return string_equals_ignore_case(tablaDePaginas->nombre, segmentoABuscar);
	}

	tablaDeSegmentosEnCache= list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);
	if(tablaDeSegmentosEnCache!= NULL){

		int encontrarElemDePag(t_elemTablaDePaginas* elemDePagina){
			return (elemDePagina->pagina->key == keyABuscar);
		}

		elementoDePagEnCache= list_find(tablaDeSegmentosEnCache->elementosDeTablaDePagina,(void*)encontrarElemDePag);
		if(elementoDePagEnCache !=NULL){ //registro = pagina
			paqueteAuxiliar=malloc(sizeof(t_paquete));
			paqueteAuxiliar->palabraReservada=SUCCESS;
			char* requestAEnviar= strdup("");
			string_append_with_format(&requestAEnviar,"%s%s%s%s%c%s%c",segmentoABuscar," ",parametros[2]," ",'"',elementoDePagEnCache->pagina->value,'"');
			paqueteAuxiliar->request = strdup(requestAEnviar);
			paqueteAuxiliar->tamanio=sizeof(elementoDePagEnCache->pagina->value);

			*elementoEncontrado=elementoDePagEnCache;
			*valorEncontrado = paqueteAuxiliar;
			return EXIT_SUCCESS;
		}else{
			return KEYINEXISTENTE;
		}
	}else{
		return SEGMENTOINEXISTENTE;
	}
	eliminar_paquete(paqueteAuxiliar);

}

/*encontrarSegmento()
 * Parametros:
 * 	-> char* ::segmentoABuscar
 * Descripcion: busca entre los segmentos existentes en cache y, en caso de existir el segmentoSolicitado,
 * 				devuelve un puntero al mismo.
 * Return:
 * 	-> t_tablaDePaginas* :: segmentoEncontrado
 * VALGRIND:: SI */
t_tablaDePaginas* encontrarSegmento(char* segmentoABuscar){
	int encontrarTabla(t_tablaDePaginas* tablaDePaginas){
		return string_equals_ignore_case(tablaDePaginas->nombre, segmentoABuscar);
	}

	return list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);
}

/* enviarAlDestinatarioCorrecto()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 * 	-> int :: codResultado(de la request solictada)
 * 	-> char* ::  request
 * 	-> t_paquete* :: valorAEnviar
 * 	-> t_caller :: caller
 * 	-> int :: i (socketKernel)
 * Descripcion: En caso de que la request haya llegado desde kernel, le envia la respuesta a el.
 * 				En caso de que haya llegado por consola, muestra el resultado de la request por la misma.
 * Return:
 * 	-> :: void
 * VALGRIND:: NO*/
 void enviarAlDestinatarioCorrecto(cod_request palabraReservada,int codResultado,char* request,t_paquete* valorAEnviar,t_caller caller,int socketKernel){
	 //TODO reservar y liberar
	 char *errorDefault= strdup("");
	 switch(caller){
	 	 case(ANOTHER_COMPONENT):
			enviar(codResultado, valorAEnviar->request, socketKernel);
	 	 	break;
	 	 case(CONSOLE):
	 		mostrarResultadoPorConsola(palabraReservada, codResultado,request, valorAEnviar);
	 	  	break;
	 	 default:
	 		string_append_with_format(&errorDefault, "%s%s","No se ha encontrado a quien devolver la reques realizada",request);
	 		 log_info(logger_MEMORIA,errorDefault);
	 		 break;

	}
 }

/* mostrarResultadoPorConsola()
  * Parametros:
  * 	-> cod_request :: palabraReservada
  * 	-> int :: codResultado(de la request solictada)
  * 	-> char* ::  request
  * 	-> t_paquete* :: valorAEnviar
  * Descripcion: Muestra por consola el mensaje a la request hecha, ya se un resultado exitoso o erroneo.
  * Return:
  * 	-> :: void
  * VALGRIND:: NO*/
 void mostrarResultadoPorConsola(cod_request palabraReservada, int codResultado,char* request,t_paquete* valorAEnviar){
	 char *respuesta= strdup("");
	 char* error=strdup("");
	 char** requestSeparada=separarRequest(valorAEnviar->request);
	 char* valorEncontrado = requestSeparada[2];
	 switch(palabraReservada){
	 	 case(SELECT):
	 		if(codResultado == SUCCESS){
				string_append_with_format(&respuesta, "%s%s%s%s","La respuesta a la request: ",request," es: ", valorEncontrado);
				log_info(logger_MEMORIA,respuesta);
	 		}else{
	 			switch(codResultado){
					case(KEY_NO_EXISTE):
						string_append_with_format(&error, "%s%s%s","La respuesta a: ",request," no es valida, KEY INEXISTENTE");
						log_info(logger_MEMORIA,error);
						break;
					case(TABLA_NO_EXISTE):
						string_append_with_format(&error, "%s%s%s","La respuesta a: ",request," no es valida, TABLA INEXISTENTE");
						log_info(logger_MEMORIA,error);
						break;
					default:
						log_info(logger_MEMORIA,"No se ha podido encontrar respuesta a la request",request);
						break;
	 			}
			}
	 	 break;
		case(INSERT):
			if(codResultado == SUCCESS){
				string_append_with_format(&respuesta, "%s%s%s","La request: ",request," se ha realizado con exito");
				log_info(logger_MEMORIA,respuesta);
			}else{//TODO CREO Q SOLO ME PUEDEN DECIR Q NO EXITE LA TABLA
				string_append_with_format(&error, "%s%s%s","La request: ",request," no a podido realizarse, TABLA INEXISTENTE");
				log_info(logger_MEMORIA,error);
			}
			break;
		case(CREATE):
			if(codResultado == SUCCESS || codResultado == TABLA_EXISTE){
				string_append_with_format(&respuesta, "%s%s%s","La request: ",request," se ha realizado con exito");
				log_info(logger_MEMORIA,respuesta);
			}else{
				string_append_with_format(&error, "%s%s%s","La request: ",request," no a podido realizarse");
				log_info(logger_MEMORIA,error);
			}
			break;
		default:
			log_info(logger_MEMORIA,"MEMORIA NO LO SABE RESOLVER AUN, PERO TE INVITO A QUE LO HAGAS VOS :)");
			break;

		}
}

 /* guardarRespuestaDeLFSaCACHE()
   * Parametros:
   * 	-> t_paquete* :: nuevoPaquete
   * 	-> t_erroresCache :: tipoError
   * Descripcion: Guarda el resultado devuelto por LFS a cache.
   * 			-En el caso de de que ya existia la key, se actualiza la misma
   * 			-En el caso de KEYIEXISTENTE= se debe solicitar PAGINA LIBRE:
   * 				*si la hay,se guarda una nueva pag con los valores obtenido en el
   * 					segmento(Que este si existia en cache).
   * 				*Si NO hay espacio libre => ALGORITMO DE REEMPLAZO (Y si MEMORIA == FULL) =>JOURNALING
   * 			-En el caso de SEGMENTOINEXISTENTE: se debe agregar un nuevo segmento y solictar PAGINA LIBRE
   * 				(siguiendo la logica de keyinexistente)
   * Return:
   * 	-> :: void
   * VALGRIND:: NO*/
void guardarRespuestaDeLFSaCACHE(t_paquete* nuevoPaquete,t_erroresCache tipoError){

	if(nuevoPaquete->palabraReservada == SUCCESS){
		char** requestSeparada= separarRequest(nuevoPaquete->request);
		char* nuevaTabla= strdup(requestSeparada[0]);
		uint16_t nuevaKey= convertirKey(requestSeparada[1]);
		char* nuevoValor= strdup(requestSeparada[2]);
		unsigned long long nuevoTimestamp;
		convertirTimestamp(requestSeparada[3],&nuevoTimestamp);//no checkeo, viene de LFS
		if(tipoError== KEYINEXISTENTE){
			t_tablaDePaginas* tablaBuscada= malloc(sizeof(t_tablaDePaginas));
			tablaBuscada= encontrarSegmento(nuevaTabla);
			list_add(tablaBuscada->elementosDeTablaDePagina,crearElementoEnTablaDePagina(nuevaKey, nuevoValor,nuevoTimestamp));
		}else if(tipoError==SEGMENTOINEXISTENTE){

			t_tablaDePaginas* nuevaTablaDePagina = crearTablaDePagina(nuevaTabla);
			list_add(nuevaTablaDePagina->elementosDeTablaDePagina,crearElementoEnTablaDePagina(nuevaKey,nuevoValor,nuevoTimestamp));
			list_add(tablaDeSegmentos->segmentos,nuevaTablaDePagina);

		}
	}
//		case(TABLA_NO_EXISTE):
//		case(KEY_NO_EXISTE):

}

/* procesarInsert()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 *	-> char* :: request
 *	-> t_caller :: caller
 *	-> int :: i (socket kernel)
 * Descripcion: EC:
 * 					1)se va a fijar si EXISTE SEGMENTO Y EXISTE KEY en memoria.
 	 	 	 	 	 si es correcto , se ACTUALIZA la pag (valor y timestamp)
 * 					2)si no encuentra NO EXISTE KEY Pero si EXISTE SEGMENTO.Solicitar PAGINA LIBRE:
 * 					*si la hay,se guarda una nueva pag con los valores obtenido en el
 * 						segmento existente.
 * 					*Si NO hay espacio libre => ALGORITMO DE REEMPLAZO (Y si MEMORIA == FULL) =>JOURNALING
 * 					3)Si NO EXISTE SEGMENTO,solicita un segmento y solictar PAGINA LIBRE(siguiendo la logica de keyinexistente)
 *
 *				SC || SHC:
 *					1)Le pregunta  LFS si existe la tabla en la que se desea insertar.
 *					2)Si no existe, se notifica error al destinatario correspondiente
 *					3)Si existe en lisandra=>se inserta siguiendo los pasos de EC
 * Return:
 * 	-> :: void
 * 	VALGRIND :: NO*/
void procesarInsert(cod_request palabraReservada, char* request,consistencia consistenciaMemoria, t_caller caller, int i) {
		t_elemTablaDePaginas* elementoEncontrado= malloc(sizeof(t_elemTablaDePaginas));
		t_paquete* valorEncontrado=malloc(sizeof(t_paquete));
		char* requestSeparada = separarRequest(request);
//---------------CASOS DE PRUEBA------------------------------
//en el .h para poder compartirlo con la funcion insertar
		t_paquete* valorDeLFS=malloc(sizeof(t_paquete));
//		valorDeLF->palabraReservada= 0;
//		valorDeLF->tamanio=100;
//		valorDeLF->request=strdup("tablaB  chau 123454657");

//-------------------------------------------------------------

		puts("ANTES DE IR A BUSCAR A CACHE");
		int resultadoCache= estaEnMemoria(palabraReservada, request,&valorEncontrado,&elementoEncontrado);
		if(consistenciaMemoria == EC || caller == CONSOLE){
				insertar(resultadoCache,palabraReservada,request,elementoEncontrado,caller,i);
		}else if(consistenciaMemoria == SC || consistenciaMemoria == SHC){
				char* consultaALFS=malloc(sizeof(char*));
				string_append_with_format(&consultaALFS,"%s%s%s%s%s","SELECT"," ",requestSeparada[1]," ",requestSeparada[2]);
				valorDeLFS = intercambiarConFileSystem(SELECT,consultaALFS);
				if((consistenciaMemoria== SC && validarInsertSC(valorDeLFS->palabraReservada)== EXIT_SUCCESS)){
					insertar(resultadoCache,palabraReservada,request,elementoEncontrado,caller,i);
				}else{
					enviarAlDestinatarioCorrecto(palabraReservada,valorDeLFS->palabraReservada,request,valorDeLFS,caller, (int) list_get(descriptoresClientes,i));
				}
				free(consultaALFS);
		}else{
			log_info(logger_MEMORIA, "NO se le ha asignado un tipo de consistencia a la memoria, por lo que no puede responder la consulta: ", request);

		}
}

/* insertar()
 * Parametros:
 * 	-> int :: resultadoCache
 * 	-> cod_request :: palabraReservada
 *	-> char* :: request
 *	-> t_elemTablaDePaginas* :: elementoEncontrado
 *	-> t_caller :: caller
 *	-> int :: i (socket kernel)
 * Descripcion: Dependiendo del resultado arrojado por estaEnCache, inserta en memoria.
 * 				Si EXISTE SEGMENTO Y EXISTE VALUR => ACTUALIZA
 * 				Si EXISTE SEGMENTO Y NO EXISTE KEY => solicita pagina libre (+algoritmo de reemplazo+jounaling en caso de ser necesario)
 * 				SI NO EXISTE LE SEGMENTO => se solicita agregar un nuevo segmento y se siguen los pasos de keyinexistente
 * Return:
 * 	-> :: void
 * 	VALGRIND :: NO*/
void insertar(int resultadoCache,cod_request palabraReservada,char* request,t_elemTablaDePaginas* elementoEncontrado,t_caller caller, int i){
	t_paquete* paqueteAEnviar= malloc(sizeof(t_paquete));

	char** requestSeparada = separarRequest(request);
	char* nuevaTabla = strdup(requestSeparada[1]);
	char* nuevaKeyChar = strdup(requestSeparada[2]);
	int nuevaKey = convertirKey(nuevaKeyChar);
	char* nuevoValor = strdup(requestSeparada[3]);
	unsigned long long nuevoTimestamp;

//	char *consultaALFS= strdup("");

	if(requestSeparada[4]!=NULL){
		convertirTimestamp(requestSeparada[4],&nuevoTimestamp);
	}else{
		nuevoTimestamp= obtenerHoraActual();
	}


	if(resultadoCache == EXIT_SUCCESS){ // es decir que EXISTE SEGMENTO y EXISTE KEY
		log_info(logger_MEMORIA, "LO ENCONTRE EN CACHEE!");
		actualizarElementoEnTablaDePagina(elementoEncontrado,nuevoValor);

		paqueteAEnviar= armarPaqueteDeRtaAEnviar(request);

		enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, paqueteAEnviar,caller, (int) list_get(descriptoresClientes,i));

	}else if(resultadoCache == KEYINEXISTENTE){//TODO:		KEY no encontrada -> nueva pagina solicitada
		int hayEspacio= EXIT_SUCCESS; // TODO ALGORTMO DE REEMPLAZO + JOURNALING

		if(hayEspacio ==EXIT_SUCCESS){
			t_tablaDePaginas* tablaDestino = (t_tablaDePaginas*)malloc(sizeof(t_tablaDePaginas));
			tablaDestino = encontrarSegmento(nuevaTabla);
			list_add(tablaDestino->elementosDeTablaDePagina,crearElementoEnTablaDePagina(nuevaKey,nuevoValor, nuevoTimestamp));

			//TODO unificar desp cuando lfs este, con valorDeLF
			paqueteAEnviar= armarPaqueteDeRtaAEnviar(request);
			enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, paqueteAEnviar,caller, (int) list_get(descriptoresClientes,i));
		}

	}else if(resultadoCache == SEGMENTOINEXISTENTE){ //	TODO hay q solicitar si hay o no espacio??

			t_tablaDePaginas* nuevaTablaDePagina = crearTablaDePagina(nuevaTabla);
			list_add(tablaDeSegmentos->segmentos,nuevaTablaDePagina);
			list_add(nuevaTablaDePagina->elementosDeTablaDePagina,crearElementoEnTablaDePagina(nuevaKey,nuevoValor,nuevoTimestamp));
			enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, paqueteAEnviar,caller, (int) list_get(descriptoresClientes,i));
	}

}

/* armarPaqueteDeRtaAEnviar()
 * Parametros:
 *	-> char* :: request
 * Descripcion: Cuando el resultado del insert fue EXITOSO,
 * 				arma un paquete para enviarse (a quien solicito un insert)
 * Return:
 * 	-> paqueteAEnviar:: t_paquete*
 * 	VALGRIND :: NO*/
t_paquete* armarPaqueteDeRtaAEnviar(char* request){
	t_paquete* paqueteAEnviar= malloc(sizeof(t_paquete));

	paqueteAEnviar->palabraReservada= SUCCESS;
	char** requestRespuesta= string_n_split(request,2," ");
	paqueteAEnviar->request=strdup(requestRespuesta[1]);
	paqueteAEnviar->tamanio=strlen(requestRespuesta[1]);

	return paqueteAEnviar;
}

/* validarInsertSC()
 * Parametros:
 *	-> errorNo :: codRespuestaDeLFS
 * Descripcion: En SC||SHC, para insertar debe existir la tabla en LFS.Esta funcion valida si el resultado
 * 				devuelto del select hecho a LFS es SUCCESS o NO
 * Return:
 * 	-> int :: resulltado de validacion
 * 	VALGRIND :: SI*/
int validarInsertSC(errorNo codRespuestaDeLFS){
	if (codRespuestaDeLFS == SUCCESS){
		return EXIT_SUCCESS;
	}else{
		return EXIT_FAILURE;
	}
}

t_pagina* crearPagina(uint16_t nuevaKey, char* nuevoValor, unsigned long long nuevoTimesTamp){
	t_pagina* nuevaPagina= (t_pagina*)malloc(sizeof(t_pagina));
	nuevaPagina->timestamp = nuevoTimesTamp;
	nuevaPagina->key = nuevaKey;
	nuevaPagina->value = nuevoValor;
	return nuevaPagina;
}

void actualizarPagina (t_pagina* pagina, char* newValue){
	unsigned long long newTimes = obtenerHoraActual();
	pagina->timestamp = newTimes;
	pagina->value =strdup(newValue);
}

t_elemTablaDePaginas* crearElementoEnTablaDePagina(uint16_t newKey, char* newValue, unsigned long long timesTamp){
	t_elemTablaDePaginas* newElementoDePagina= (t_elemTablaDePaginas*)malloc(sizeof(t_elemTablaDePaginas));
	newElementoDePagina->numeroDePag = rand();
	newElementoDePagina->pagina = crearPagina(newKey,newValue,timesTamp);
	newElementoDePagina->modificado = SINMODIFICAR;
	return newElementoDePagina;
}

void actualizarElementoEnTablaDePagina(t_elemTablaDePaginas* elemento, char* newValue){
	actualizarPagina(elemento->pagina,newValue);
	elemento->modificado = MODIFICADO;
}

t_tablaDePaginas* crearTablaDePagina(char* nuevaTabla){
	t_tablaDePaginas* newTablaDePagina = (t_tablaDePaginas*)malloc(sizeof(t_tablaDePaginas));
	newTablaDePagina->nombre=strdup(nuevaTabla);
	newTablaDePagina->elementosDeTablaDePagina=list_create();
	return newTablaDePagina;
}



void procesarCreate(cod_request codRequest, char* request ,consistencia consistencia, t_caller caller, int socketKernel){
	//TODO, si lfs dio ok, igual calcular en mem?
	t_paquete* valorDeLFS = intercambiarConFileSystem(codRequest,request);

	if(valorDeLFS->palabraReservada == SUCCESS || valorDeLFS->palabraReservada == TABLA_EXISTE){
		create(codRequest, request);
		enviarAlDestinatarioCorrecto(codRequest,SUCCESS,request, valorDeLFS, caller,socketKernel);
	}else{
		enviarAlDestinatarioCorrecto(codRequest,valorDeLFS->palabraReservada,request, valorDeLFS, caller,socketKernel);

	}

	eliminar_paquete(valorDeLFS);
	valorDeLFS= NULL;
}

void create(cod_request codRequest,char* request){
	errorNo rtaCache = existeSegentoEnMemoria(codRequest,request);

	if(rtaCache == SEGMENTOINEXISTENTE){
		char** requestSeparada = separarRequest(request);
		t_tablaDePaginas* nuevaTablaDePagina = crearTablaDePagina(requestSeparada[1]);
		list_add(tablaDeSegmentos->segmentos,nuevaTablaDePagina);
	}
}

errorNo existeSegentoEnMemoria(cod_request palabraReservada, char* request){
	t_tablaDePaginas* tablaDeSegmentosEnCache = malloc(sizeof(t_tablaDePaginas));
	char** parametros = separarRequest(request);
	char* segmentoABuscar=strdup(parametros[1]);
	int encontrarTabla(t_tablaDePaginas* tablaDePaginas){
		return string_equals_ignore_case(tablaDePaginas->nombre, segmentoABuscar);
	}

	tablaDeSegmentosEnCache= list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);
	if(tablaDeSegmentosEnCache!= NULL){
		return SEGMENTOEXISTENTE;
	}else{
		return SEGMENTOINEXISTENTE;
	}
	free(tablaDeSegmentosEnCache); //TODO hay q hace uno q libere bien
	free(segmentoABuscar);
	segmentoABuscar =NULL;
	liberarArrayDeChar(parametros);
	parametros=NULL;
}


// FUNCION QUE QUEREMOS UTILIZAR CUANDO FINALIZAN LOS DOS HILOS
void liberarMemoria(){
	void liberarElementoDePag(t_elemTablaDePaginas* self){
		 free(self->pagina->value);
		 free(self->pagina);
	 }
	list_clean_and_destroy_elements(tablaA->elementosDeTablaDePagina, (void*)liberarElementoDePag);
}


void eliminarElemTablaDePaginas(t_elemTablaDePaginas* elementoEncontrado){
	eliminarPagina(elementoEncontrado->pagina);
	free(elementoEncontrado);
	elementoEncontrado=NULL;
}
void eliminarPagina(t_pagina* pag){
	free(pag->value);
	free(pag);
	pag= NULL;
}
