#include "memoria.h"
#include <stdlib.h>

int main(void) {

	//--------------------------------INICIO DE MEMORIA ---------------------------------------------------------------
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/memoria/memoria.config");
	logger_MEMORIA = log_create("memoria.log", "Memoria", 1, LOG_LEVEL_DEBUG);

	//--------------------------------CONEXION CON LFS ---------------------------------------------------------------

	conectarAFileSystem();

	//--------------------------------RESERVAR MEMORIA ---------------------------------------------------------------
	inicializacionDeMemoria();
	log_info(logger_MEMORIA,"INICIO DE MEMORIA");

	//--------------------------------SEMAFOROS-HILOS ----------------------------------------------------------------
	//	SEMAFOROS
	//sem_init(&semLeerDeConsola, 0, 1);
	sem_init(&semEnviarMensajeAFileSystem, 0, 0);
	//pthread_mutex_init(&terminarHilo, NULL);

	// 	HILOS
	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);
	pthread_create(&hiloEscucharMultiplesClientes, NULL, (void*)escucharMultiplesClientes, NULL);

	pthread_join(hiloLeerDeConsola, NULL);
	log_info(logger_MEMORIA, "Hilo de consola finalizado");
	pthread_join(hiloEscucharMultiplesClientes, NULL);
	log_info(logger_MEMORIA, "Hilo recibir kernels finalizado");

	//-------------------------------- -PARTE FINAL DE MEMORIA---------------------------------------------------------
	liberarEstructurasMemoria();
	liberarMemoria();
	return EXIT_SUCCESS;
}


void inicializacionDeMemoria(){
	//-------------------------------Reserva de memoria-------------------------------------------------------

	memoria = malloc(config_get_int_value(config, "TAM_MEM"));
	marcosTotales = config_get_int_value(config, "TAM_MEM")/(27+maxValue);

	//-------------------------------Creacion de structs-------------------------------------------------------
	bitarrayString = string_repeat('0', marcosTotales);
	bitarray= bitarray_create_with_mode(bitarrayString,marcosTotales,LSB_FIRST);
	tablaDeSegmentos = (t_tablaDeSegmentos*)malloc(sizeof(t_tablaDeSegmentos));
	tablaDeSegmentos->segmentos =list_create();

	//-------------------------------AUXILIAR: creacion de tabla/pag/elemento ---------------------------------

//	t_marco* pagLibre = NULL;
//	int index= obtenerPaginaDisponible(&pagLibre);
//
//	t_segmento* nuevoSegmento = crearSegmento("tablaA");
//	t_elemTablaDePaginas* nuevoElemTablaDePagina = crearElementoEnTablaDePagina(index,pagLibre,1,"hola",12345678);
//	list_add(nuevoSegmento->tablaDePagina,nuevoElemTablaDePagina);
//	list_add(tablaDeSegmentos->segmentos,nuevoSegmento);

}


/* obtenerIndiceMarcoDisponible()
 * Parametros:
 * 	->  ::  void
 * Descripcion:Obtiene, en caso de que exista, un marco disponible.
 * 				En caso contrario, devuelve error.
 * Return:
 * 	-> error||indice :: int
 * VALGRIND:: SI */
int obtenerIndiceMarcoDisponible() {
	int index = 0;
	while(index < marcosTotales && bitarray_test_bit(bitarray, index)) index++;
	if(index >= marcosTotales) {
		index = NUESTRO_ERROR;
	}
	return index;
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
			pthread_cancel(hiloEscucharMultiplesClientes);
			free(mensaje);
			break;
		}
		validarRequest(mensaje);
//		free(mensaje);
//		mensaje=NULL;
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
void validarRequest(char* mensaje){
	char** request = string_n_split(mensaje, 2, " ");
	char** requestSeparada= separarRequest(mensaje);
	char* mensajeDeError;
	int mensajeValido = validarMensaje(mensaje, MEMORIA, &mensajeDeError);
	int palabraReservada = obtenerCodigoPalabraReservada(request[0],MEMORIA);
	if(mensajeValido == SUCCESS){
		if(!(palabraReservada == INSERT && validarValue(mensaje,requestSeparada[3],maxValue,logger_MEMORIA) == NUESTRO_ERROR)){
			interpretarRequest(palabraReservada, mensaje, CONSOLE,-1);
		}
	}else{
		log_error(logger_MEMORIA, mensajeDeError);
	}
	free(mensaje);
	mensaje=NULL;
	liberarArrayDeChar(request);
	request =NULL;
	liberarArrayDeChar(requestSeparada); //(1)
	requestSeparada=NULL;
}

/* conectarAFileSystem()
 * Parametros:
 * 	->  :: void
 * Descripcion: Se crea la conexion con lissandraFileSystem y se realiza el handshake con la misma
 * 				Se recibe de lfs el tam max del value
 * Return:
 * 	-> :: void
 * VALGRIND:: SI */
void conectarAFileSystem() {
	conexionLfs = crearConexion(
			config_get_string_value(config, "IP_FS"),
			config_get_string_value(config, "PUERTO_FS"));
	handshakeLFS = recibirHandshakeLFS(conexionLfs);
	maxValue= handshakeLFS->tamanioValue;
	log_info(logger_MEMORIA, "SE CONECTO CON LFS");
	log_info(logger_MEMORIA, "Recibi de LFS TAMAÑO_VALUE: %d", handshakeLFS->tamanioValue);
	free(handshakeLFS);
}


/* escucharMultiplesClientes()
 * VALGRIND:: SI */
void escucharMultiplesClientes() {
	int descriptorServidor = iniciar_servidor(config_get_string_value(config, "PUERTO"), config_get_string_value(config, "IP"));
	log_info(logger_MEMORIA, "Memoria lista para recibir al kernel");

	/* fd = file descriptor (id de Socket)
	 * fd_set es Set de fd's (una coleccion)*/
	clientesGossiping = list_create();
	clientesRequest = list_create();
	descriptoresClientes = list_create();	// Lista de descriptores de todos los clientes conectados (podemos conectar infinitos clientes)
	int numeroDeClientes = 0;				// Cantidad de clientes conectados
	int valorMaximo = 0;					// Descriptor cuyo valor es el mas grande (para pasarselo como parametro al select)
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

					int esElCliente(int clienteFd) {
						return clienteFd == (int) list_get(descriptoresClientes,i);
					}
					if (list_any_satisfy(clientesGossiping, (void*)esElCliente)) {
						// mandar lista de gossiping
					} else {
						// si no es gossiping, es de request
						paqueteRecibido = recibir((int) list_get(descriptoresClientes,i)); // Recibo de ese cliente en particular
						cod_request palabraReservada = paqueteRecibido->palabraReservada;
						char* request = paqueteRecibido->request;
						printf("Del fd %i \n", (int) list_get(descriptoresClientes,i)); // Muestro por pantalla el fd del cliente del que recibi el mensaje
						if(palabraReservada != COMPONENTE_CAIDO){
							printf("El codigo que recibi es: %i \n", palabraReservada);
							interpretarRequest(palabraReservada,request,ANOTHER_COMPONENT, i);
						}else{
							log_info(logger_MEMORIA, "Se desconecto el kernel %i", (int) list_get(descriptoresClientes,i));
							list_replace(descriptoresClientes, i, (void*)-1); // Si el cliente se desconecta le pongo un -1 en su fd}
							list_remove_by_condition(clientesGossiping, (void*)esElCliente);
							list_remove_by_condition(clientesRequest, (void*)esElCliente);
						}
						eliminar_paquete(paqueteRecibido);
						paqueteRecibido=NULL;
					}

				}
//				log_error(logger_MEMORIA, "el cliente se desconecto. Terminando servidor");
			}

			if(FD_ISSET (descriptorServidor, &descriptoresDeInteres)) {
				int descriptorCliente = esperar_cliente(descriptorServidor); 					  // Se comprueba si algun cliente nuevo se quiere conectar
				//enviarGossiping("8001", "127.0.0.1", "1", descriptorCliente);
				t_handshake_memoria* handshake = recibirHandshakeMemoria(descriptorCliente);
				if (handshake->tipoComponente == KERNEL || handshake->tipoComponente == MEMORIA) {
					numeroDeClientes = (int) list_add(descriptoresClientes, (int*) descriptorCliente); // Agrego el fd del cliente a la lista de fd's
					numeroDeClientes++;
					if (handshake->tipoRol == REQUEST) {
						list_add(clientesRequest, (void*)descriptorCliente);
					} else if (handshake->tipoRol == GOSSIPING) {
						list_add(clientesGossiping,(void*) descriptorCliente);
						if (handshake->tipoComponente == KERNEL) {
							// por ahora queda asi porque kernel no manda mensaje
							enviarGossiping("8001", "127.0.0.1", "1", descriptorCliente);
						}
					}
				} else {
					log_error(logger_MEMORIA, "Solo se puede conectar un kernel o una memoria, rechazando conexión...");
				}
				free(handshake);
			}

	}

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
 * VALGRIND:: EN PROCESO */
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
			log_debug(logger_MEMORIA, "Me llego un SELECT");
			procesarSelect(codRequest, request,consistenciaMemoria, caller, i);
			break;
		case INSERT:
			log_debug(logger_MEMORIA, "Me llego un INSERT");
			procesarInsert(codRequest, request,consistenciaMemoria, caller, i);
			break;
		case CREATE:
			log_debug(logger_MEMORIA, "Me llego un CREATE");
			procesarCreate(codRequest, request,consistenciaMemoria, caller, i);
			break;
		case DESCRIBE:
			log_debug(logger_MEMORIA, "Me llego un DESCRIBE");
			procesarDescribe(codRequest, request,caller,i);
			break;
		case DROP:
			log_debug(logger_MEMORIA, "Me llego un DROP");
			procesarDrop(codRequest, request ,consistenciaMemoria, caller, i);
			break;
		case JOURNAL:
			log_debug(logger_MEMORIA, "Me llego un JOURNAL");
			procesarJournal(codRequest, request, caller, i);
			break;
		case NUESTRO_ERROR:
			 if(caller == ANOTHER_COMPONENT){
				 log_error(logger_MEMORIA, "el cliente se desconecto. Terminando servidor");
				 int valorAnterior = (int) list_replace(descriptoresClientes, i, (int*) -1); // Si el cliente se desconecta le pongo un -1 en su fd}
				 // TODO: Chequear si el -1 se puede castear como int*
			  }
			 break;
		default:
			log_warning(logger_MEMORIA, "No has ingresado una request valida");
			break;
	}
	liberarArrayDeChar(requestSeparada);
	requestSeparada=NULL;
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
	usleep(config_get_int_value(config, "RETARDO_FS")*1000);
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

	t_paquete* valorDeLFS=NULL;
	t_elemTablaDePaginas* elementoEncontrado;



	int resultadoCache;

	if(consistenciaMemoria == EC || caller == CONSOLE){		// en caso de no existir el segmento o la tabla en MEMORIA, se lo solicta a LFS
		t_paquete* valorEncontrado;
		resultadoCache= estaEnMemoria(palabraReservada, request,&valorEncontrado,&elementoEncontrado);
		if(resultadoCache == EXIT_SUCCESS ) {
			log_info(logger_MEMORIA, "LO ENCONTRE EN CACHEE!");
			actualizarTimestamp(elementoEncontrado->marco);
			enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, valorEncontrado,caller, (int) list_get(descriptoresClientes,i));
			eliminar_paquete(valorEncontrado);
			valorEncontrado=NULL;

		} else {// en caso de no existir el segmento o la tabla en MEMORIA, se lo solicta a LFS
			log_info(logger_MEMORIA,"ME LO TIENE QUE DECIR LFS");
			valorDeLFS = intercambiarConFileSystem(palabraReservada,request);
			enviarAlDestinatarioCorrecto(palabraReservada, valorDeLFS->palabraReservada,request, valorDeLFS,caller, (int) list_get(descriptoresClientes,i));
			guardarRespuestaDeLFSaCACHE(valorDeLFS, resultadoCache);
			eliminar_paquete(valorDeLFS);
			valorDeLFS=NULL;
		}
	}else if(consistenciaMemoria==SC || consistenciaMemoria == SHC){
		log_info(logger_MEMORIA,"ME LO TIENE QUE DECIR LFS");
		valorDeLFS = intercambiarConFileSystem(palabraReservada,request);
		enviarAlDestinatarioCorrecto(palabraReservada, valorDeLFS->palabraReservada,request, valorDeLFS,caller, (int) list_get(descriptoresClientes,i));
		eliminar_paquete(valorDeLFS);
		valorDeLFS=NULL;
	}else{
		log_info(logger_MEMORIA, "NO se le ha asignado un tipo de consistencia a la memoria, por lo que no puede responder la consulta: ", request);

	}

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
	t_elemTablaDePaginas* elementoDePagEnCache;
	t_paquete* paqueteAuxiliar;

	char** parametros = separarRequest(request);
	char* segmentoABuscar=strdup(parametros[1]);
	uint16_t keyABuscar= convertirKey(parametros[2]);

	int encontrarTabla(t_segmento* segmento){
		return string_equals_ignore_case(segmento->path, segmentoABuscar);
	}

	t_segmento* segmentoEnCache = list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);
	if(segmentoEnCache!= NULL){

		int encontrarElemTablaDePag(t_elemTablaDePaginas* elemDePagina){
			return (elemDePagina->marco->key == keyABuscar);
		}

		elementoDePagEnCache= list_find(segmentoEnCache->tablaDePagina,(void*)encontrarElemTablaDePag);
		if(elementoDePagEnCache != NULL && palabraReservada == SELECT){
			paqueteAuxiliar=malloc(sizeof(t_paquete));
			paqueteAuxiliar->palabraReservada=SUCCESS;
			char* requestAEnviar= strdup("");
			string_append_with_format(&requestAEnviar,"%s%s%s%s%c%s%c",segmentoABuscar," ",parametros[2]," ",'"',elementoDePagEnCache->marco->value,'"');
			paqueteAuxiliar->request = strdup(requestAEnviar);
			paqueteAuxiliar->tamanio=sizeof(maxValue);

			free(requestAEnviar);
			requestAEnviar=NULL;
			*elementoEncontrado=elementoDePagEnCache;
			*valorEncontrado = paqueteAuxiliar;
			free(segmentoABuscar);
			segmentoABuscar=NULL;
			liberarArrayDeChar(parametros);
			parametros=NULL;
			return EXIT_SUCCESS;
		}else if (elementoDePagEnCache != NULL){
			*elementoEncontrado=elementoDePagEnCache;
			free(segmentoABuscar);
			segmentoABuscar=NULL;
			liberarArrayDeChar(parametros);
			parametros=NULL;
			return EXIT_SUCCESS;
		}else{
			free(segmentoABuscar);
			segmentoABuscar=NULL;
			liberarArrayDeChar(parametros);
			parametros=NULL;
			return KEYINEXISTENTE;
		}
	}else{
		free(segmentoABuscar);
		segmentoABuscar=NULL;
		liberarArrayDeChar(parametros);
		parametros=NULL;
		return SEGMENTOINEXISTENTE;
	}

}


/*encontrarSegmento()
 * Parametros:
 * 	-> char* ::segmentoABuscar
 * Descripcion: busca entre los segmentos existentes en cache y, en caso de existir el segmentoSolicitado,
 * 				devuelve un puntero al mismo.
 * Return:
 * 	-> t_tablaDePaginas* :: segmentoEncontrado
 * VALGRIND:: SI */
t_segmento* encontrarSegmento(char* segmentoABuscar){
	int encontrarTabla(t_segmento* segmento){
			return string_equals_ignore_case(segmento->path, segmentoABuscar);
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
	 char *errorDefault= strdup("");
	 switch(caller){
	 	 case(ANOTHER_COMPONENT):
	 		log_info(logger_MEMORIA, valorAEnviar->request);
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

	free(errorDefault);
	errorDefault=NULL;
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
  * VALGRIND:: SI */
 void mostrarResultadoPorConsola(cod_request palabraReservada, int codResultado,char* request,t_paquete* valorAEnviar){
	 char* respuesta= strdup("");
	 char* error=strdup("");
	 char** requestSeparada=separarRequest(request);
	 char* valorEncontrado;

	 switch(palabraReservada){
	 	 case(SELECT):
		{	char** valorAEnviarSeparado=separarRequest(valorAEnviar->request);
	 	 	 valorEncontrado = valorAEnviarSeparado[2];

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
		free(respuesta);
		respuesta = NULL;
		free(error);
		error = NULL;
		liberarArrayDeChar(valorAEnviarSeparado); //(3)
		valorAEnviarSeparado = NULL;
		liberarArrayDeChar(requestSeparada); //(4)
		requestSeparada = NULL;
	 	 break;
		}
		case(INSERT): {
			char** valorAEnviarSeparado = separarRequest(valorAEnviar->request);

			if(codResultado == SUCCESS){
				string_append_with_format(&respuesta, "%s%s%s","La request: ",request," se ha realizado con exito");
				log_info(logger_MEMORIA,respuesta);
	 			free(respuesta);
	 			respuesta=NULL;
	 			free(error);
	 			error=NULL;
	 			liberarArrayDeChar(valorAEnviarSeparado); //(3)
	 			valorAEnviarSeparado=NULL;
	 			liberarArrayDeChar(requestSeparada); //(4)
	 			requestSeparada=NULL;
			}else{
				string_append_with_format(&error, "%s%s%s","La request: ",request," no a podido realizarse, TABLA INEXISTENTE");
				log_info(logger_MEMORIA,error);
	 			free(respuesta);
	 			respuesta=NULL;
	 			free(error);
	 			error=NULL;
	 			liberarArrayDeChar(valorAEnviarSeparado); //(3)
	 			valorAEnviarSeparado=NULL;
	 			liberarArrayDeChar(requestSeparada); //(4)
	 			requestSeparada=NULL;
			}
			break;
			}
		case(CREATE):
			if(codResultado == SUCCESS || codResultado == TABLA_EXISTE){
				string_append_with_format(&respuesta, "%s%s%s","La request: ",request," se ha realizado con exito");
				log_info(logger_MEMORIA,respuesta);
	 			free(respuesta);
	 			respuesta=NULL;
	 			free(error);
	 			error=NULL;
	 			liberarArrayDeChar(requestSeparada);
	 			requestSeparada=NULL;
			}else{
				string_append_with_format(&error, "%s%s%s","La request: ",request," no a podido realizarse");
				log_info(logger_MEMORIA,error);
	 			free(respuesta);
	 			respuesta=NULL;
	 			free(error);
	 			error=NULL;
	 			liberarArrayDeChar(requestSeparada);
	 			requestSeparada=NULL;
			}
			break;
	 	 case(DESCRIBE):
	 		if(codResultado == SUCCESS){
				string_append_with_format(&error, "%s%s%s","La request: ",request," se ha realizado con exito");
				log_info(logger_MEMORIA,error);
	 		}else{
				string_append_with_format(&error, "%s%s%s","La request: ",request," no a podido realizarse");
				log_info(logger_MEMORIA,error);
	 		}
			free(respuesta);
			respuesta=NULL;
			free(error);
			error=NULL;
			liberarArrayDeChar(requestSeparada);
			requestSeparada=NULL;
			break;
	 	 case(DROP):
			if(codResultado == SUCCESS){
				string_append_with_format(&error, "%s%s%s","La request: ",request," se ha realizado con exito");
				log_info(logger_MEMORIA,error);
			}else{
				string_append_with_format(&error, "%s%s%s","La request: ",request," no a podido realizarse");
				log_info(logger_MEMORIA,error);
			}
			free(respuesta);
			respuesta=NULL;
			free(error);
			error=NULL;
			liberarArrayDeChar(requestSeparada);
			requestSeparada=NULL;
	 	 	break;
		default:
			log_info(logger_MEMORIA,"MEMORIA NO LO SABE RESOLVER AUN, PERO TE INVITO A QUE LO HAGAS VOS :)");
 			free(respuesta);
 			respuesta=NULL;
 			free(error);
 			error=NULL;
 			liberarArrayDeChar(requestSeparada);
 			requestSeparada=NULL;
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
 void guardarRespuestaDeLFSaCACHE(t_paquete* nuevoPaquete,t_erroresMemoria tipoError){

 	if(nuevoPaquete->palabraReservada == SUCCESS){
 		char** requestSeparada= separarRequest(nuevoPaquete->request);
 		char* nuevaTabla= strdup(requestSeparada[0]);
 		uint16_t nuevaKey= convertirKey(requestSeparada[1]);
 		char* nuevoValor= strdup(requestSeparada[2]);
 		unsigned long long nuevoTimestamp;
 		convertirTimestamp(requestSeparada[3],&nuevoTimestamp);//no chequeo, viene de LFS
 		t_marco* pagLibre = NULL;
 		int index= obtenerPaginaDisponible(&pagLibre);
 		if(index == MEMORIAFULL){
 			t_elemTablaDePaginas* elementoAInsertar= (t_elemTablaDePaginas*)malloc(sizeof(t_elemTablaDePaginas));
 			elementoAInsertar->marco=NULL;
 			int rtaLRU;
 			elementoAInsertar=correrAlgoritmoLRU(&rtaLRU);
 			if (rtaLRU == SUCCESS){
 				if (tipoError == KEYINEXISTENTE) {
 					t_segmento* tablaBuscada = malloc(sizeof(t_segmento));
 					tablaBuscada->tablaDePagina=NULL;
 					tablaBuscada = encontrarSegmento(nuevaTabla);
 					list_add(tablaBuscada->tablaDePagina,crearElementoEnTablaDePagina(elementoAInsertar->numeroDePag,elementoAInsertar->marco, nuevaKey,nuevoValor, nuevoTimestamp));
 					free(nuevaTabla);
 					nuevaTabla=NULL;
 					free(tablaBuscada);
 					tablaBuscada=NULL;
 				} else if (tipoError == SEGMENTOINEXISTENTE) {
 					t_segmento* nuevoSegmento = (t_segmento*)malloc(sizeof(t_segmento));
 					crearSegmento(nuevoSegmento, nuevaTabla);
 					list_add(nuevoSegmento->tablaDePagina,crearElementoEnTablaDePagina(elementoAInsertar->numeroDePag,elementoAInsertar->marco, nuevaKey,nuevoValor, nuevoTimestamp));
 					list_add(tablaDeSegmentos->segmentos, nuevoSegmento);

 					free(nuevaTabla);
 					nuevaTabla=NULL;
 				}

 				//free(elementoAInsertar);
 			} else {
 				log_info(logger_MEMORIA,
 						"NO hay paginas para reemplazar, hay q hacer journaling");
 			}
 		}else{
 			if(tipoError== KEYINEXISTENTE){
 				t_segmento* tablaBuscada= malloc(sizeof(t_segmento));
 				tablaBuscada= encontrarSegmento(nuevaTabla);
 				list_add(tablaBuscada->tablaDePagina,crearElementoEnTablaDePagina(index,pagLibre,nuevaKey, nuevoValor,nuevoTimestamp));

 				free(tablaBuscada);
 				tablaBuscada=NULL;
 			}else if(tipoError == SEGMENTOINEXISTENTE){
 				t_segmento* nuevoSegmento = (t_segmento*)malloc(sizeof(t_segmento));
 				crearSegmento(nuevoSegmento,nuevaTabla);
 				list_add(nuevoSegmento->tablaDePagina,crearElementoEnTablaDePagina(index,pagLibre,nuevaKey,nuevoValor,nuevoTimestamp));
 				list_add(tablaDeSegmentos->segmentos,nuevoSegmento);

 				free(nuevaTabla);
 				nuevaTabla=NULL;
 			}
 		}
 	liberarArrayDeChar(requestSeparada);
 	requestSeparada=NULL;
 	free(nuevaTabla);
 	nuevaTabla=NULL;
 	free(nuevoValor);
 	nuevoValor=NULL;
 	}
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
 *					3)Si existe en lisandra=>se inserta alli.
 * Return:
 * 	-> :: void
 * 	VALGRIND :: NO*/
void procesarInsert(cod_request palabraReservada, char* request,consistencia consistenciaMemoria, t_caller caller, int i) {
		t_elemTablaDePaginas* elementoEncontrado= NULL;
		char** requestSeparada = separarRequest(request);

		if(consistenciaMemoria == EC || caller == CONSOLE){
			int resultadoCache= estaEnMemoria(palabraReservada, request,NULL,&elementoEncontrado);
			insertar(resultadoCache,palabraReservada,request,elementoEncontrado,caller,i);
		}else if(consistenciaMemoria == SC || consistenciaMemoria == SHC){

			t_paquete* insertALFS =  intercambiarConFileSystem(palabraReservada,request);
			if(insertALFS->palabraReservada== EXIT_SUCCESS){
				enviarAlDestinatarioCorrecto(palabraReservada,SUCCESS,request,insertALFS,caller, (int) list_get(descriptoresClientes,i));
				eliminar_paquete(insertALFS);
				insertALFS=NULL;
			}else{
				enviarAlDestinatarioCorrecto(palabraReservada,insertALFS->palabraReservada,request,insertALFS,caller, (int) list_get(descriptoresClientes,i));
				eliminar_paquete(insertALFS);
				insertALFS=NULL;
			}
		}else{
			log_info(logger_MEMORIA, "NO se le ha asignado un tipo de consistencia a la memoria, por lo que no puede responder la consulta: ", request);
		}

		liberarArrayDeChar(requestSeparada);
		requestSeparada=NULL;
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
	t_paquete* paqueteAEnviar;

	char** requestSeparada = separarRequest(request);
	char* nuevaTabla = strdup(requestSeparada[1]);
	char* nuevaKeyChar = strdup(requestSeparada[2]);
	int nuevaKey = convertirKey(nuevaKeyChar);
	char* nuevoValor = strdup(requestSeparada[3]);
	unsigned long long nuevoTimestamp;

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
		eliminar_paquete(paqueteAEnviar);

	}else{
		t_marco* pagLibre =NULL;
		int index =obtenerPaginaDisponible(&pagLibre);
		if(index == MEMORIAFULL){
			log_info(logger_MEMORIA,"la memoria se encuentra full, debe ejecutars eel algoritmo de reemplazo");
			t_elemTablaDePaginas* elementoAInsertar;
			int rtaLRU;
			elementoAInsertar=correrAlgoritmoLRU(&rtaLRU);
			if (rtaLRU == SUCCESS){
				if (resultadoCache == KEYINEXISTENTE) {
					t_segmento* tablaBuscada;
					tablaBuscada = encontrarSegmento(nuevaTabla);
					list_add(tablaBuscada->tablaDePagina,crearElementoEnTablaDePagina(elementoAInsertar->numeroDePag,elementoAInsertar->marco, nuevaKey,nuevoValor, nuevoTimestamp));
					free(nuevaTabla);
					nuevaTabla=NULL;
				} else if (resultadoCache == SEGMENTOINEXISTENTE) {
					t_segmento* nuevoSegmento = (t_segmento*)malloc(sizeof(t_segmento));
					crearSegmento(nuevoSegmento, nuevaTabla);
					list_add(nuevoSegmento->tablaDePagina,crearElementoEnTablaDePagina(elementoAInsertar->numeroDePag,elementoAInsertar->marco, nuevaKey,nuevoValor, nuevoTimestamp));
					list_add(tablaDeSegmentos->segmentos, nuevoSegmento);

					free(nuevaTabla);
					nuevaTabla=NULL;
				}
			} else {
				log_info(logger_MEMORIA,
						"NO hay paginas para reemplazar, hay q hacer journaling");
			}
		}else{
			if(resultadoCache == KEYINEXISTENTE){
				t_segmento* tablaDestino = encontrarSegmento(nuevaTabla);
				list_add(tablaDestino->tablaDePagina,crearElementoEnTablaDePagina(index,pagLibre,nuevaKey,nuevoValor, nuevoTimestamp));
				paqueteAEnviar= armarPaqueteDeRtaAEnviar(request);
				enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, paqueteAEnviar,caller, (int) list_get(descriptoresClientes,i));
				eliminar_paquete(paqueteAEnviar);

			}else if(resultadoCache == SEGMENTOINEXISTENTE){

				t_segmento* nuevoSegmento = (t_segmento*)malloc(sizeof(t_segmento));
				crearSegmento(nuevoSegmento,nuevaTabla);
				t_elemTablaDePaginas* nuevaElemTablaDePagina = crearElementoEnTablaDePagina(index,pagLibre,nuevaKey,nuevoValor,nuevoTimestamp);
				list_add(tablaDeSegmentos->segmentos,nuevoSegmento);
				list_add(nuevoSegmento->tablaDePagina,nuevaElemTablaDePagina);
				paqueteAEnviar= armarPaqueteDeRtaAEnviar(request);
				enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, paqueteAEnviar,caller, (int) list_get(descriptoresClientes,i));
				eliminar_paquete(paqueteAEnviar);

			}
		}
	}
	free(nuevaKeyChar);
	nuevaKeyChar=NULL;
	free(nuevaTabla);
	nuevaTabla=NULL;
	free(nuevoValor);
	nuevoValor=NULL;
	liberarArrayDeChar(requestSeparada);
	requestSeparada=NULL;
}

/* armarPaqueteDeRtaAEnviar()
 * Parametros:
 *	-> char* :: request
 * Descripcion: Cuando el resultado del insert fue EXITOSO,
 * 				arma un paquete para enviarse (a quien solicito un insert)
 * Return:
 * 	-> paqueteAEnviar:: t_paquete*
 * 	VALGRIND :: EN PROCESO */
t_paquete* armarPaqueteDeRtaAEnviar(char* request){
	t_paquete* paqueteAEnviar= malloc(sizeof(t_paquete));

	paqueteAEnviar->palabraReservada= SUCCESS;
	char** requestRespuesta= string_n_split(request,2," ");
	paqueteAEnviar->request=strdup(requestRespuesta[1]);
	paqueteAEnviar->tamanio=strlen(requestRespuesta[1]);

	liberarArrayDeChar(requestRespuesta);
	requestRespuesta=NULL;
	return paqueteAEnviar;
}

/* actualizarTimestamp()
 * Parametros:
 *	-> t_marco* :: marco
 * Descripcion: actualiza el timestamp correspondiente al marco dado
 * Return:
 * 	-> void ::
 * 	VALGRIND :: SI*/
void actualizarTimestamp(t_marco* marco){
	marco->timestamp= obtenerHoraActual();
}


/* actualizarPagina()
 * Parametros:
 *	-> t_marco* :: pagina
 *	-> char* :: nuevoValor
 * Descripcion: actualiza la pagina, insertando la hora actual, y actualiza el puntero
 * 				al marco, insertando el nuevo valor.
 * Return:
 * 	-> void ::
 * 	VALGRIND :: SI*/
void actualizarPagina (t_marco* marco, char* nuevoValue){
	marco->timestamp =  obtenerHoraActual();
	//strcpy(marco->value,'/0');
	strcpy(marco->value,nuevoValue);
}

/* actualizarElementoEnTablaDePagina()
 * Parametros:
 *	-> t_elemTablaDePaginas* :: elemento
 *	-> char* :: nuevoValor
 * Descripcion: actualiza un elemento de la tabla de paginas(de un segmento), indicando
 * 				que este ha sido actualizado(falg activado).
 * Return:
 * 	-> void ::
 * 	VALGRIND :: SI*/
void actualizarElementoEnTablaDePagina(t_elemTablaDePaginas* elemento, char* nuevoValor){
	actualizarPagina(elemento->marco,nuevoValor);
	elemento->modificado = MODIFICADO;
}

/* crearPagina()
 * Parametros:
 *	-> t_marco* :: pagina
 *	-> uint16_t :: nuevaKey
 *	-> char* :: nuevoValor
 *	-> unsigned long long :: timesTamp
 * Descripcion: crea una nueva pagina, utilizando como marco, un puntero que fue previamente
 * 				hallado.
 * Return:
 * 	-> t_marco* :: pagina
 * 	VALGRIND :: SI*/
t_marco* crearMarcoDePagina(t_marco* pagina,uint16_t nuevaKey, char* nuevoValue, unsigned long long timesTamp){
	pagina->key= nuevaKey;
	strcpy(pagina->value,nuevoValue);
	pagina->timestamp= timesTamp;
	return pagina;
}

/* crearElementoEnTablaDePagina()
 * Parametros:
 * 	-> int :: id
 *	-> t_marco* :: pagina
 *	-> uint16_t :: nuevaKey
 *	-> char* :: nuevoValor
 *	-> unsigned long long :: timesTamp
 * Descripcion: crea una nuevo elemento (de la tabla de paginas).Le asigna al mismo
 * 				un nuevo id(por lo cual, primero incrementa la variable global), le setea el
 * 				flag modificado en 0 y, crea una pagina.
 * Return:
 * 	-> t_elemTablaDePaginas* :: elementoDeTablaDePagina
 * 	VALGRIND :: SI*/
t_elemTablaDePaginas* crearElementoEnTablaDePagina(int id,t_marco* pagLibre, uint16_t nuevaKey, char* nuevoValue, unsigned long long timesTamp){
	t_elemTablaDePaginas* nuevoElemento= (t_elemTablaDePaginas*)malloc(sizeof(t_elemTablaDePaginas));

	bitarray_set_bit(bitarray, id);
	nuevoElemento->numeroDePag = id;
	nuevoElemento->marco = crearMarcoDePagina(pagLibre,nuevaKey,nuevoValue,timesTamp);
	nuevoElemento->modificado = SINMODIFICAR;

	return nuevoElemento;
}

/* crearSegmento()
 * Parametros:
 *	-> char* :: pathNuevoSegmento
 * Descripcion: crea un nuevo segmento con el path que fue solicitado.
 * Return:
 * 	-> t_segmento* :: nuevoSegmento
 * 	VALGRIND :: SI*/
void crearSegmento(t_segmento* nuevoSegmento,char* pathNuevoSegmento){
	nuevoSegmento->path=strdup(pathNuevoSegmento);
	nuevoSegmento->tablaDePagina=list_create();
}

/* procesarCreate()
 * Parametros:
 *	-> codRequest :: codRequest
 *	-> char* :: request
 *	-> consistencia :: consistencia
 *	-> t_caller :: caller
 *	-> int :: socketKernel
 * Descripcion: SC || SHC -> envia el create  LFS.
 * 				EC -> crea el segmento en la memoria, sin consultarle a LFS.
 * Return:
 * 	-> void ::
 * 	VALGRIND :: NO*/
void procesarCreate(cod_request codRequest, char* request ,consistencia consistencia, t_caller caller, int socketKernel){
	t_paquete* valorDeLFS=intercambiarConFileSystem(codRequest,request);
	if(consistencia == EC || caller == CONSOLE){
		create(codRequest, request);
	}
	enviarAlDestinatarioCorrecto(codRequest,SUCCESS,request, valorDeLFS, caller,(int) list_get(descriptoresClientes,socketKernel));
	eliminar_paquete(valorDeLFS);
	valorDeLFS=NULL;
}

/* create()
 * Parametros:
 *	-> codRequest :: codRequest
 *	-> char* :: request
 * Descripcion: Verifica que el segmento si existe en la memoria,y , en caso
 * 				de que no sea asi, se crea un nuevo segmento.
 * 				En caso contrario, no lo crea.
 * Return:
 * 	-> void ::
 * 	VALGRIND :: EN PROCESO */
void create(cod_request codRequest,char* request){
	t_erroresMemoria rtaCache = existeSegmentoEnMemoria(codRequest,request);

	if(rtaCache == SEGMENTOINEXISTENTE){
		char** requestSeparada = separarRequest(request);
		t_segmento* nuevoSegmento = (t_segmento*)malloc(sizeof(t_segmento));
		crearSegmento(nuevoSegmento,requestSeparada[1]);
		list_add(tablaDeSegmentos->segmentos,nuevoSegmento);
		liberarArrayDeChar(requestSeparada);
		requestSeparada=NULL;
	}
}

/* existeSegmentoEnMemoria()
 * Parametros:
 *	-> codRequest :: codRequest
 *	-> char* :: request
 * Descripcion: Solamente verifica si existe o no un segmento en memoria.
 * Return:
 * 	-> void ::
 * 	VALGRIND :: EN PROCESO */
t_erroresMemoria existeSegmentoEnMemoria(cod_request palabraReservada, char* request){
	char** parametros = separarRequest(request);
	char* segmentoABuscar=strdup(parametros[1]);
	int encontrarTabla(t_segmento* segmento){
		return string_equals_ignore_case(segmento->path, segmentoABuscar);
	}

	t_segmento* segmentosEnCache = list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);
	if(segmentosEnCache!= NULL){
		liberarArrayDeChar(parametros);
		parametros=NULL;
		free(segmentoABuscar);
		segmentoABuscar =NULL;
		log_info(logger_MEMORIA,"YA EXISTE EL SEGMENTO");
		return SEGMENTOEXISTENTE;
	}else{
		liberarArrayDeChar(parametros);
		parametros=NULL;
		free(segmentoABuscar);
		segmentoABuscar =NULL;
		log_info(logger_MEMORIA,"NO EXISTE EL SEGMENTO");
		return SEGMENTOINEXISTENTE;
	}
}


/* obtenerPaginaDisponible()
 * Parametros:
 *	-> t_marco** :: pagLibre
 * Descripcion: Obtiene un puntero a una pagina libre, ocurriendo esto cuando existe un marco libre
 *				donde pueda guardarse la misma.
 * Return:
 * 	-> int :: resultado de la obtencion de una paina libre
 * 	VALGRIND :: SI*/
int obtenerPaginaDisponible(t_marco** pagLibre){
	int index= obtenerIndiceMarcoDisponible();
	if(index == NUESTRO_ERROR){
		return MEMORIAFULL;
	}else{
		*pagLibre = memoria + (sizeof(unsigned long long)+sizeof(uint16_t)+maxValue)*index;
		return index;
	}

}


/* liberarTabla()
 * Parametros:
 *	-> t_segmento* :: segmento
 * Descripcion: Libera a el marco de la pagina, libera cada pagina de una tabla
 * Return:
 * 	-> :: void
 * 	VALGRIND :: NO */
void liberarTabla(t_segmento* segmento){
	int listaIgual(t_segmento* segmentoComparar){
		if(strcmp(segmentoComparar->path, segmento->path)){
			return TRUE;
		}else{
			return FALSE;
		}
	}
	list_remove_and_destroy_by_condition(tablaDeSegmentos->segmentos,(void*) listaIgual, (void*) eliminarElemTablaPagina);
}

/* liberarEstructurasMemoria()
 * Parametros:
 *	-> t_tablaDeSEgmentos :: tablaDeSegmento
 * Descripcion: Libera el marco de cada pagina, libera cada pagina de una tablaDePagina, libera la tablaDePagina
 * 				de un segmento, libera lista de segmentos de una tablaDeSegmento, libera lista de tablaDeSegmentos
 * Return:
 * 	-> :: void
 * 	VALGRIND :: NO */
void liberarEstructurasMemoria(){
	list_destroy_and_destroy_elements(tablaDeSegmentos->segmentos, (void*) eliminarElemTablaSegmentos);
	free(tablaDeSegmentos);
}
void eliminarElemTablaSegmentos(t_segmento* segmento){
	free(segmento->path);
	segmento->path=NULL;
	list_destroy_and_destroy_elements(segmento->tablaDePagina, (void*) eliminarElemTablaPagina);
	free(segmento);
}
void eliminarElemTablaPagina(t_elemTablaDePaginas* pagina){
	eliminarMarco(pagina,pagina->marco);
	free(pagina);
	pagina=NULL;
}
/* liberarMemoria()
 * Parametros:
 *	-> :: void
 * Descripcion: Libera los punteros reservados en inicializarMemoria()
 * Return:
 * 	-> :: void
 * 	VALGRIND :: NO */
void liberarMemoria(){
	log_info(logger_MEMORIA, "Finaliza MEMORIA");
	bitarray_destroy(bitarray);
	free(bitarrayString);
	bitarrayString=NULL;
	free(memoria);
//	memoria=NULL;
	liberar_conexion(conexionLfs);
	FD_ZERO(&descriptoresDeInteres);
	log_destroy(logger_MEMORIA);
	config_destroy(config);
	list_destroy(clientesGossiping);
	list_destroy(descriptoresClientes);
	list_destroy(clientesRequest);
}


/* eliminarMarco()
 * Parametros:
 *	-> t_marco** :: pagLibre
 * Descripcion: Obtiene un puntero a una pagina libre, ocurriendo esto cuando existe un marco libre
 *				donde pueda guardarse la misma.
 * Return:
 * 	-> void ::
 * 	VALGRIND :: SI*/
void eliminarMarco(t_elemTablaDePaginas* elem,t_marco* marcoAEliminar){
	bitarray_clean_bit(bitarray, elem->numeroDePag);
	memset(marcoAEliminar->value, 0, maxValue);
//	free(marcoAEliminar->value);
//	free(marcoAEliminar);
}

/* procesarDescribe()
 * Parametros:
 *	-> cod_request :: codRequest
 *	-> char* :: request
 *	-> t_caller :: caller
 *	-> int :: i//socket kernel
 * Descripcion: Actua como un pasamanos para pasarle a info del describe, mandada por lfs
 * 				a kernel.
 * Return:
 * 	-> void ::
 * 	VALGRIND :: NO*/
void procesarDescribe(cod_request codRequest, char* request,t_caller caller,int i){
	t_paquete* describeLFS=intercambiarConFileSystem(codRequest,request);
	enviarAlDestinatarioCorrecto(codRequest,describeLFS->palabraReservada,request,describeLFS,caller,(int) list_get(descriptoresClientes,i));
	eliminar_paquete(describeLFS);
	describeLFS=NULL;
}


/* procesarDrop()
 * Parametros:
 *	-> cod_request :: codRequest
 *	-> char* :: request
 *	-> consistencia :: consistencias
 *	-> t_caller :: caller
 *	-> int :: i (socket kernel)
 * Descripcion: Busca la tabla en memoria principal, si la encuentra libera la memoria. Siempre le avisa a LFS
 * Return:
 * 	-> void ::
 * 	VALGRIND :: NO*/
void procesarDrop(cod_request codRequest, char* request ,consistencia consistencia, t_caller caller, int i) {
	t_paquete* valorDeLFS = malloc(sizeof(t_paquete));
	valorDeLFS->palabraReservada=SUCCESS;
	valorDeLFS->request=strdup("");
	valorDeLFS->tamanio=sizeof(valorDeLFS->request);
	char** requestSeparada = separarRequest(request);
	char* segmentoABuscar=strdup(requestSeparada[1]);
	valorDeLFS = intercambiarConFileSystem(codRequest,request);
	if(consistencia == EC || caller == CONSOLE){
		int encontrarTabla(t_segmento* segmento){
			return string_equals_ignore_case(segmento->path, segmentoABuscar);
		}
		t_segmento* segmentosEnCache= list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);

		if(segmentosEnCache!= NULL){
			log_info(logger_MEMORIA,"La %s fue eliminada de MEMORIA",segmentosEnCache->path);
			liberarTabla(segmentosEnCache);
		}else{
			log_info(logger_MEMORIA,"La %s no existe en MEMORIA",segmentoABuscar);
		}
	}
	enviarAlDestinatarioCorrecto(codRequest,valorDeLFS->palabraReservada,request, valorDeLFS, caller,(int) list_get(descriptoresClientes,i));
	eliminar_paquete(valorDeLFS);
	valorDeLFS= NULL;
	liberarArrayDeChar(requestSeparada);
	requestSeparada=NULL;
	free(segmentoABuscar);
	segmentoABuscar=NULL;
}

/* desvincularVictimaDeSuSegmento()
 *	-> t_elemTablaDePaginas* :: elemVictima
 * Descripcion: Le quita al segmento la pagina indicada
 * Return:
 * 	-> void ::
 * 	VALGRIND :: NO*/
int desvincularVictimaDeSuSegmento(t_elemTablaDePaginas* elemVictima){
	int i =0;
	int retorno=NUESTRO_ERROR;
	t_segmento* segmento;
	segmento=NULL;
		int contieneElElementoVictima(t_elemTablaDePaginas* elem){
			if(elem->numeroDePag == elemVictima->numeroDePag){
				retorno = SUCCESS;
				puts(elem->marco->value);
				return SUCCESS;
			}else{
				return NUESTRO_ERROR;
			}
		}

	while(list_get(tablaDeSegmentos->segmentos,i)!=NULL){
		segmento = list_get(tablaDeSegmentos->segmentos,i);
		list_remove_and_destroy_by_condition(segmento->tablaDePagina,(void*)contieneElElementoVictima,(void*)eliminarElemTablaPagina);
		i++;
	}

	return retorno;
}

/* menorTimestamp()
 * desvincularVictimaDeSuSegmento:
 *	-> t_elemTablaDePaginas* :: primerElem
 *	-> t_elemTablaDePaginas* :: segundoElem
 * Descripcion: Indica si es cierto que el primer elemento contiene un timestamp menor que el segundo.
 * Return:
 * 	-> int :: bool
 * 	VALGRIND :: SI*/
 int menorTimestamp(t_elemTablaDePaginas* primerElem,t_elemTablaDePaginas* segundoElem) {
	return primerElem->marco->timestamp <= segundoElem->marco->timestamp;
}

/* correrAlgoritmoLRU()
 * desvincularVictimaDeSuSegmento:
 *	-> t_elemTablaDePaginas** :: primerElem
 * Descripcion: Revisa si existe un pag sin modificar y , de hacerlo, la elige como victima para darsela
 * 				a la nueva request ingresada.
 * Return:
 * 	-> int :: bool-rta del algoritmo LRU
 * 	VALGRIND :: NO*/
 t_elemTablaDePaginas* correrAlgoritmoLRU(int* rta) {
	log_info(logger_MEMORIA,"la memoria se encuentra full, debe ejecutarse el algoritmo de reemplazo");
	int i=0, j= 0;

	t_list* elemSinModificar=list_create();

	while(list_get(tablaDeSegmentos->segmentos,j)!=NULL){
		t_segmento* segmento=list_get(tablaDeSegmentos->segmentos,j);

		while (list_get(segmento->tablaDePagina, i) != NULL) {
			t_elemTablaDePaginas* elemenetoTablaDePag =list_get(segmento->tablaDePagina, i);
			if (elemenetoTablaDePag->modificado == SINMODIFICAR) {
				list_add(elemSinModificar, elemenetoTablaDePag);
			}
			i++;
		}
		j++;
		i=0;
	}

	//t_list* listaVictima;
	t_elemTablaDePaginas* elementoVictima;
	elementoVictima =NULL;

	if (!list_is_empty(elemSinModificar)) {
		list_sort(elemSinModificar, (void*) menorTimestamp);
		//listaVictima = list_take_and_remove(elemSinModificar, 1);
		elementoVictima= list_get(elemSinModificar,0);
//		list_destroy_and_destroy_elements(elemSinModificar,(void*)eliminarElemTablaPagina);
		list_destroy(elemSinModificar);
		int desvinculacion=desvincularVictimaDeSuSegmento(elementoVictima);
		if(desvinculacion == SUCCESS){
			*rta=SUCCESS;
		}else{
			*rta=NUESTRO_ERROR;
		}

	} else {
		*rta=JOURNALTIME;
	}
//	list_clean_and_destroy_elements(elemSinModificar,(void*)liberarElemTablaPagina);
	return elementoVictima;
}



void procesarJournal(cod_request codRequest, char* request, t_caller caller, int i) {
	/** Todas aquellas páginas con el flag activado son las que contienen las Key que deben ser actualizadas en el FS.
	 *  Las páginas cuyo flag esté desactivado implican que el dato en memoria es consistente (o eventualmente consistente)
	 *  con el que está en el FS.
	 **/
	t_list* elemModificados = list_create();

	void encontrarElemModificado(t_segmento* segmento){
		void encontrarPagModificada(t_elemTablaDePaginas* elemPagina){
			if(elemPagina->modificado == MODIFICADO){
				list_add(elemModificados, elemPagina);
				puts(elemPagina->marco->value);
			}
		}
		list_iterate(segmento->tablaDePagina, (void*) encontrarPagModificada);
	}
	list_iterate(tablaDeSegmentos->segmentos,(void*) encontrarElemModificado);
}




//void procesarJournal(cod_request codRequest, char* request, t_caller caller, int i) {
//	/** Todas aquellas páginas con el flag activado son las que contienen las Key que deben ser actualizadas en el FS.
//	 *  Las páginas cuyo flag esté desactivado implican que el dato en memoria es consistente (o eventualmente consistente)
//	 *  con el que está en el FS.
//	 **/
//	t_list* paginasModificadas = list_map(tablaDeSegmentos->segmentos,(void*) obtenerTablasModificadas);
//}
//
//t_list* obtenerTablasModificadas(t_segmento* segmento){
//	t_list* tablasModificadas = list_filter(segmento->tablaDePagina,(void*) tablaDePaginaModificada);
//	return tablasModificadas;
//}
//
//int tablaDePaginaModificada(t_elemTablaDePaginas* elementoTP){
//	if(elementoTP->modificado==MODIFICADO){
//		printf("El codigo que recibi es: %i \n", elementoTP->marco->value );
//		return TRUE;
//	}else{
//		return FALSE;
//	}
//}
