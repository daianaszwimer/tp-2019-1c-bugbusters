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
	liberarEstructurasMemoria(tablaDeSegmentos);
	liberarMemoria();
	return EXIT_SUCCESS;
}


void inicializacionDeMemoria(){
	//-------------------------------Reserva de memoria-------------------------------------------------------
	//maxValue = handshakeConLFS(); TODO handshake posta

	memoria = malloc(config_get_int_value(config, "TAM_MEM"));
	marcosTotales = config_get_int_value(config, "TAM_MEM")/sizeof(t_marco);

	//-------------------------------Creacion de structs-------------------------------------------------------
	bitarrayString= strdup("");
	bitarray_create_with_mode(bitarrayString,marcosTotales,LSB_FIRST);
	tablaDeSegmentos = (t_tablaDeSegmentos*)malloc(sizeof(t_tablaDeSegmentos));
	tablaDeSegmentos->segmentos =list_create();

	//-------------------------------AUXILIAR: creacion de tabla/pag/elemento ---------------------------------

	t_marco* pagLibre = NULL;
	int index= obtenerPaginaDisponible(&pagLibre);

	t_segmento* nuevoSegmento = crearSegmento("tablaA");
	t_elemTablaDePaginas* nuevoElemTablaDePagina = crearElementoEnTablaDePagina(index,pagLibre,1,"hola",12345678);
	list_add(nuevoSegmento->tablaDePagina,nuevoElemTablaDePagina);
	list_add(tablaDeSegmentos->segmentos,nuevoSegmento);

}

//int calcularTamMarco(){
//	t_marco* marco=malloc(sizeof(t_marco));
//	int tamanio= sizeof(marco->timestamp)+ sizeof(marco->key)+sizeof(marco->value);
//	return tamanio;
//}

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
	int tamanioMax = handshake->tamanioValue; //TODO lo pasa LFS X HANDSHAKE
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
	handshake = recibirHandshakeLFS(conexionLfs);
	log_info(logger_MEMORIA, "SE CONECTO CON LFS");
	log_info(logger_MEMORIA, "Recibi de LFS TAMAÑO_VALUE: %d", handshake->tamanioValue);
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
					printf("El codigo que recibi es: %i \n", palabraReservada);
					printf("Del fd %i \n", (int) list_get(descriptoresClientes,i)); // Muestro por pantalla el fd del cliente del que recibi el mensaje
					interpretarRequest(palabraReservada,request,ANOTHER_COMPONENT, i);
					free(request);
					request=NULL;


				}
//				log_error(logger_MEMORIA, "el cliente se desconecto. Terminando servidor");
//				int valorAnterior = (int) list_replace(descriptoresClientes, i, (int*) -1); // Si el cliente se desconecta le pongo un -1 en su fd}
//				// TODO: Chequear si el -1 se puede castear como int*
			}//fin for

			if(FD_ISSET (descriptorServidor, &descriptoresDeInteres)) {
				int descriptorCliente = esperar_cliente(descriptorServidor); 					  // Se comprueba si algun cliente nuevo se quiere conectar
				// todo: por ahora que no tenemos gossiping vamos a levantar siempre 3 memorias con estos 3 datos hardcodeados
				enviarHandshakeMemoria("8001", "127.0.0.1", "1", descriptorCliente);
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
			procesarDescribe(codRequest, request,caller,i);
			break;
		case DROP:
			log_info(logger_MEMORIA, "Me llego un DROP");
			procesarDrop(codRequest, request ,consistenciaMemoria, caller, i);
			break;
		case JOURNAL:
			log_info(logger_MEMORIA, "Me llego un JOURNAL");
			break;
		case SALIDA:
			log_info(logger_MEMORIA,"Has finalizado el componente MEMORIA");
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

//-------------------------------------------------------------

	t_paquete* valorDeLFS=malloc(sizeof(t_paquete));
	t_elemTablaDePaginas* elementoEncontrado = malloc(sizeof(t_elemTablaDePaginas));
	t_paquete* valorEncontrado=malloc(sizeof(t_paquete));


	int resultadoCache;

	if(consistenciaMemoria == EC || caller == CONSOLE){		// en caso de no existir el segmento o la tabla en MEMORIA, se lo solicta a LFS
		resultadoCache= estaEnMemoria(palabraReservada, request,&valorEncontrado,&elementoEncontrado);
		if(resultadoCache == EXIT_SUCCESS ) {
			log_info(logger_MEMORIA, "LO ENCONTRE EN CACHEE!");
			actualizarTimestamp(valorEncontrado);
			enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, valorEncontrado,caller, (int) list_get(descriptoresClientes,i));

		} else {// en caso de no existir el segmento o la tabla en MEMORIA, se lo solicta a LFS
			log_info(logger_MEMORIA,"ME LO TIENE QUE DECIR LFS");
			valorDeLFS = intercambiarConFileSystem(palabraReservada,request);
			enviarAlDestinatarioCorrecto(palabraReservada, valorDeLFS->palabraReservada,request, valorDeLFS,caller, (int) list_get(descriptoresClientes,i));
			guardarRespuestaDeLFSaCACHE(valorDeLFS, resultadoCache);
		}
	}else if(consistenciaMemoria==SC || consistenciaMemoria == SHC){
		log_info(logger_MEMORIA,"ME LO TIENE QUE DECIR LFS");
		valorDeLFS = intercambiarConFileSystem(palabraReservada,request);
		enviarAlDestinatarioCorrecto(palabraReservada, valorDeLFS->palabraReservada,request, valorDeLFS,caller, (int) list_get(descriptoresClientes,i));
	}else{
		log_info(logger_MEMORIA, "NO se le ha asignado un tipo de consistencia a la memoria, por lo que no puede responder la consulta: ", request);

	}

	//eliminar_paquete(valorDeLFS); TODO revisar esto si se deberia borrar
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
	t_segmento* segmentoEnCache = malloc(sizeof(t_segmento));
	t_elemTablaDePaginas* elementoDePagEnCache = malloc(sizeof(t_elemTablaDePaginas));
	t_paquete* paqueteAuxiliar;

	char** parametros = separarRequest(request);
	char* segmentoABuscar=strdup(parametros[1]);
	uint16_t keyABuscar= convertirKey(parametros[2]);

	int encontrarTabla(t_segmento* segmento){
		return string_equals_ignore_case(segmento->path, segmentoABuscar);
	}

	segmentoEnCache = list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);
	if(segmentoEnCache!= NULL){

		int encontrarElemTablaDePag(t_elemTablaDePaginas* elemDePagina){
			return (elemDePagina->marco->key == keyABuscar);
		}

		elementoDePagEnCache= list_find(segmentoEnCache->tablaDePagina,(void*)encontrarElemTablaDePag);
		if(elementoDePagEnCache !=NULL){
			paqueteAuxiliar=malloc(sizeof(t_paquete));
			paqueteAuxiliar->palabraReservada=SUCCESS;
			char* requestAEnviar= strdup("");
			string_append_with_format(&requestAEnviar,"%s%s%s%s%c%s%c",segmentoABuscar," ",parametros[2]," ",'"',elementoDePagEnCache->marco->value,'"');
			paqueteAuxiliar->request = strdup(requestAEnviar);
			paqueteAuxiliar->tamanio=sizeof(elementoDePagEnCache->marco->value);

			*elementoEncontrado=elementoDePagEnCache;
			*valorEncontrado = paqueteAuxiliar;

			liberarArrayDeChar(parametros);
			parametros=NULL;
			return EXIT_SUCCESS;
		}else{
			liberarArrayDeChar(parametros);
			parametros=NULL;
			return KEYINEXISTENTE;
		}
	}else{
		liberarArrayDeChar(parametros);
		parametros=NULL;
		return SEGMENTOINEXISTENTE;
	}
	liberarArrayDeChar(parametros);
	parametros=NULL;
	free(paqueteAuxiliar); //(4)
	paqueteAuxiliar=NULL;
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
	 //TODO reservar y liberar
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
  * VALGRIND:: NO*/
 void mostrarResultadoPorConsola(cod_request palabraReservada, int codResultado,char* request,t_paquete* valorAEnviar){
	 char* respuesta= strdup("");
	 char* error=strdup("");
	 char** requestSeparada=separarRequest(request);
	 char** valorAEnviarSeparado=strdup("");
	 char* valorEncontrado=strdup("");
	 switch(palabraReservada){
	 	 case(SELECT):
			 valorAEnviarSeparado=separarRequest(valorAEnviar->request);
	 	 	 valorEncontrado = valorAEnviarSeparado[2];

	 		if(codResultado == SUCCESS){
				string_append_with_format(&respuesta, "%s%s%s%s","La respuesta a la request: ",request," es: ", valorEncontrado);
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
		case(INSERT):
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
			}else{//TODO CREO Q SOLO ME PUEDEN DECIR Q NO EXITE LA TABLA
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
		case(CREATE):
			if(codResultado == SUCCESS || codResultado == TABLA_EXISTE){
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
				string_append_with_format(&error, "%s%s%s","La request: ",request," no a podido realizarse");
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
	 	 case(DESCRIBE):
	 		if(codResultado == SUCCESS){
				string_append_with_format(&error, "%s%s%s","La request: ",request," se ha realizado con exito");
				log_info(logger_MEMORIA,error);
	 		}else{
				string_append_with_format(&error, "%s%s%s","La request: ",request," no a podido realizarse");
				log_info(logger_MEMORIA,error);
	 		}
				break;
	 	 case(DROP):
			if(codResultado == SUCCESS){
				string_append_with_format(&error, "%s%s%s","La request: ",request," se ha realizado con exito");
				log_info(logger_MEMORIA,error);
			}else{
				string_append_with_format(&error, "%s%s%s","La request: ",request," no a podido realizarse");
				log_info(logger_MEMORIA,error);
			}
	 	 break;
		default:
			log_info(logger_MEMORIA,"MEMORIA NO LO SABE RESOLVER AUN, PERO TE INVITO A QUE LO HAGAS VOS :)");
 			free(respuesta);
 			respuesta=NULL;
 			free(error);
 			error=NULL;
 			liberarArrayDeChar(valorAEnviarSeparado); //(3)
 			valorAEnviarSeparado=NULL;
 			liberarArrayDeChar(requestSeparada); //(4)
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
			log_info(logger_MEMORIA,"la memoria se encuentra full, debe ejecutars eel algoritmo de reemplazo");
			//TODO
		}else{
			if(tipoError== KEYINEXISTENTE){
				t_segmento* tablaBuscada= malloc(sizeof(t_segmento));
				tablaBuscada= encontrarSegmento(nuevaTabla);
				list_add(tablaBuscada->tablaDePagina,crearElementoEnTablaDePagina(index,pagLibre,nuevaKey, nuevoValor,nuevoTimestamp));

			}else if(tipoError == SEGMENTOINEXISTENTE){
				t_segmento* nuevaSegmento = crearSegmento(nuevaTabla);
				list_add(nuevaSegmento->tablaDePagina,crearElementoEnTablaDePagina(index,pagLibre,nuevaKey,nuevoValor,nuevoTimestamp));
				list_add(tablaDeSegmentos->segmentos,nuevaSegmento);

			}
		}
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
		t_elemTablaDePaginas* elementoEncontrado= malloc(sizeof(t_elemTablaDePaginas));
		t_paquete* valorEncontrado=malloc(sizeof(t_paquete));
		char** requestSeparada = separarRequest(request);
		t_paquete* valorDeLFS=malloc(sizeof(t_paquete));

		if(consistenciaMemoria == EC || caller == CONSOLE){
			int resultadoCache= estaEnMemoria(palabraReservada, request,&valorEncontrado,&elementoEncontrado);
			insertar(resultadoCache,palabraReservada,request,elementoEncontrado,caller,i);
		}else if(consistenciaMemoria == SC || consistenciaMemoria == SHC){
			char* consultaALFS=malloc(sizeof(char*));
			*consultaALFS = '\0';
			string_append_with_format(&consultaALFS,"%s%s%s%s%s","SELECT"," ",requestSeparada[1]," ",requestSeparada[2]);

			t_paquete* insertALFS = malloc(sizeof(t_paquete));
			insertALFS = intercambiarConFileSystem(palabraReservada,request);
			if(insertALFS->palabraReservada== EXIT_SUCCESS){
				enviarAlDestinatarioCorrecto(palabraReservada,SUCCESS,request,insertALFS,caller, (int) list_get(descriptoresClientes,i));
			}else{
				enviarAlDestinatarioCorrecto(palabraReservada,valorDeLFS->palabraReservada,request,valorDeLFS,caller, (int) list_get(descriptoresClientes,i));
			}
			free(consultaALFS);
		}else{
			log_info(logger_MEMORIA, "NO se le ha asignado un tipo de consistencia a la memoria, por lo que no puede responder la consulta: ", request);
			free(elementoEncontrado);
			elementoEncontrado=NULL;
			free(valorEncontrado);
			valorEncontrado=NULL;
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
		free(nuevaTabla);
		nuevaTabla=NULL;
		free(nuevoValor);
		nuevoValor=NULL;
	}else{
		t_marco* pagLibre = NULL;
		int index =obtenerPaginaDisponible(&pagLibre);
		if(index == MEMORIAFULL){
		log_info(logger_MEMORIA,"la memoria se encuentra full, debe ejecutars eel algoritmo de reemplazo");
		//TODO
		}else{
			if(resultadoCache == KEYINEXISTENTE){
				t_segmento* tablaDestino = (t_segmento*)malloc(sizeof(t_segmento));
				tablaDestino = encontrarSegmento(nuevaTabla);
				list_add(tablaDestino->tablaDePagina,crearElementoEnTablaDePagina(index,pagLibre,nuevaKey,nuevoValor, nuevoTimestamp));
				paqueteAEnviar= armarPaqueteDeRtaAEnviar(request);
				enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, paqueteAEnviar,caller, (int) list_get(descriptoresClientes,i));

				eliminar_paquete(paqueteAEnviar);
				free(nuevaTabla);
				nuevaTabla=NULL;

			}else if(resultadoCache == SEGMENTOINEXISTENTE){

				t_segmento* nuevoSegmento = crearSegmento(nuevaTabla);
				t_elemTablaDePaginas* nuevaElemTablaDePagina = crearElementoEnTablaDePagina(index,pagLibre,nuevaKey,nuevoValor,nuevoTimestamp);
				list_add(tablaDeSegmentos->segmentos,nuevoSegmento);
				list_add(nuevoSegmento->tablaDePagina,nuevaElemTablaDePagina);
				enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, paqueteAEnviar,caller, (int) list_get(descriptoresClientes,i));

			}
		}//TODO Liberar memoria
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

/* actualizarTimestamp()
 * Parametros:
 *	-> t_elemTablaDePaginas* :: elemTablaDePag
 * Descripcion: actualiza el timestamp correspondiente al marco dado
 * Return:
 * 	-> void ::
 * 	VALGRIND :: SI*/
void actualizarTimestamp(t_elemTablaDePaginas* elemTablaDePag){
	elemTablaDePag->marco->timestamp= obtenerHoraActual();
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
void actualizarPagina (t_marco* pagina, char* nuevoValue){
	pagina->timestamp =  obtenerHoraActual();
	strcpy(pagina->value,nuevoValue);
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
t_marco* crearPagina(t_marco* pagina,uint16_t nuevaKey, char* nuevoValue, unsigned long long timesTamp){
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
	nuevoElemento->marco = crearPagina(pagLibre,nuevaKey,nuevoValue,timesTamp);
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
t_segmento* crearSegmento(char* pathNuevoSegmento){
	t_segmento* nuevoSegmento = (t_segmento*)malloc(sizeof(t_segmento));
	nuevoSegmento->path=strdup(pathNuevoSegmento);
	nuevoSegmento->tablaDePagina=list_create();
	return nuevoSegmento;
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
	//TODO, si lfs dio ok, igual calcular en mem?
	t_paquete* valorDeLFS = (t_paquete*)malloc(sizeof(t_paquete));
	valorDeLFS=intercambiarConFileSystem(codRequest,request);
	if(consistencia == EC || caller == CONSOLE){
		create(codRequest, request);
	}else if(consistencia == SC ||consistencia == SHC){
		enviarAlDestinatarioCorrecto(codRequest,SUCCESS,request, valorDeLFS, caller,(int) list_get(descriptoresClientes,socketKernel));
	}
	eliminar_paquete(valorDeLFS);
	valorDeLFS= NULL;
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
 * 	VALGRIND :: NO*/
void create(cod_request codRequest,char* request){
	t_erroresMemoria rtaCache = existeSegmentoEnMemoria(codRequest,request);

	if(rtaCache == SEGMENTOINEXISTENTE){
		char** requestSeparada = separarRequest(request);
		t_segmento* nuevaTablaDePagina = crearSegmento(requestSeparada[1]);
		list_add(tablaDeSegmentos->segmentos,nuevaTablaDePagina);
	}
}

/* existeSegmentoEnMemoria()
 * Parametros:
 *	-> codRequest :: codRequest
 *	-> char* :: request
 * Descripcion: Solamente verifica si existe o no un segmento en memoria.
 * Return:
 * 	-> void ::
 * 	VALGRIND :: NO*/
t_erroresMemoria existeSegmentoEnMemoria(cod_request palabraReservada, char* request){
	t_segmento* tablaDeSegmentosEnCache = malloc(sizeof(t_segmento));
	char** parametros = separarRequest(request);
	char* segmentoABuscar=strdup(parametros[1]);
	int encontrarTabla(t_segmento* segmento){
		return string_equals_ignore_case(segmento->path, segmentoABuscar);
	}

	tablaDeSegmentosEnCache= list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);
	if(tablaDeSegmentosEnCache!= NULL){
		log_info(logger_MEMORIA,"YA EXISTE EL SEGMENTO");
		return SEGMENTOEXISTENTE;
	}else{
		log_info(logger_MEMORIA,"NO EXISTE EL SEGMENTO");
		return SEGMENTOINEXISTENTE;
	}
	liberarTabla(tablaDeSegmentosEnCache); //(6)
	free(segmentoABuscar);
	segmentoABuscar =NULL;
	liberarArrayDeChar(parametros);
	parametros=NULL;
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
		*pagLibre = memoria + sizeof(t_marco)*index;
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
	void eliminarElemTablaPagina(t_elemTablaDePaginas* pagina){
		eliminarMarco(pagina,pagina->marco);
		free(pagina);
		pagina=NULL;
	}
	free(segmento->path);
	segmento->path=NULL;
	list_clean_and_destroy_elements(segmento->tablaDePagina, (void*) eliminarElemTablaPagina);
}


/* liberarEstructurasMemoria()
 * Parametros:
 *	-> t_tablaDeSEgmentos :: tablaDeSegmento
 * Descripcion: Libera el marco de cada pagina, libera cada pagina de una tablaDePagina, libera la tablaDePagina
 * 				de un segmento, libera lista de segmentos de una tablaDeSegmento, libera lista de tablaDeSegmentos
 * Return:
 * 	-> :: void
 * 	VALGRIND :: NO */
void liberarEstructurasMemoria(t_tablaDeSegmentos* tablaDeSegmentos){
	void eliminarElemTablaSegmentos(t_segmento* segmento){
		void eliminarElemTablaPagina(t_elemTablaDePaginas* pagina){
			eliminarMarco(pagina,pagina->marco);
			free(pagina);
			pagina=NULL;
		}
		free(segmento->path);
		segmento->path=NULL;
		list_clean_and_destroy_elements(segmento->tablaDePagina, (void*) eliminarElemTablaPagina);
	}
	list_clean_and_destroy_elements(tablaDeSegmentos->segmentos, (void*) eliminarElemTablaSegmentos);
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
	free(bitarray);
	bitarray=NULL; //(5)
	free(bitarrayString);
	bitarrayString=NULL;
	free(memoria);
	memoria=NULL;
	liberar_conexion(conexionLfs);
	FD_ZERO(&descriptoresDeInteres);
	log_destroy(logger_MEMORIA);
	config_destroy(config);
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
	memset(marcoAEliminar->value, 0, sizeof(marcoAEliminar->value));
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
	t_paquete* describeLFS = (t_paquete*) malloc(sizeof(t_paquete));
	describeLFS=intercambiarConFileSystem(codRequest,request);
	enviarAlDestinatarioCorrecto(codRequest,describeLFS->palabraReservada,request,describeLFS,caller,(int) list_get(descriptoresClientes,i));
}


/* procesarDrop()
 * Parametros:
 *	-> cod_request :: codRequest
 *	-> char* :: request
 *	-> consistencia :: consistencia
 *	-> t_caller :: caller
 *	-> int :: i (socket kernel)
 * Descripcion: Busca la tabla en memoria principal, si la encuentra libera la memoria. Siempre le avisa a LFS
 * Return:
 * 	-> void ::
 * 	VALGRIND :: NO*/
void procesarDrop(cod_request codRequest, char* request ,consistencia consistencia, t_caller caller, int i) {
	t_segmento* tablaDeSegmentosEnCache = malloc(sizeof(t_segmento));
	t_paquete* valorDeLFS = malloc(sizeof(t_paquete));
	valorDeLFS->palabraReservada=SUCCESS;
	valorDeLFS->request=strdup("");
	valorDeLFS->tamanio=sizeof(valorDeLFS->request);
	char** requestSeparada = separarRequest(request);
	char* segmentoABuscar=strdup(requestSeparada[1]);
	//valorDeLFS = intercambiarConFileSystem(codRequest,request);
	if(consistencia == EC || caller == CONSOLE){
		int encontrarTabla(t_segmento* segmento){
			return string_equals_ignore_case(segmento->path, segmentoABuscar);
		}
		tablaDeSegmentosEnCache= list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);

		if(tablaDeSegmentosEnCache!= NULL){
			log_info(logger_MEMORIA,"La %s fue eliminada de MEMORIA",tablaDeSegmentosEnCache->path);
			liberarTabla(tablaDeSegmentosEnCache);
		}else{
			log_info(logger_MEMORIA,"La %s no existe en MEMORIA",segmentoABuscar);
		}
	}
	enviarAlDestinatarioCorrecto(codRequest,valorDeLFS->palabraReservada,request, valorDeLFS, caller,(int) list_get(descriptoresClientes,i));
	eliminar_paquete(valorDeLFS);
	valorDeLFS= NULL;
	free(tablaDeSegmentosEnCache);
	tablaDeSegmentosEnCache=NULL;
	liberarArrayDeChar(requestSeparada);
	requestSeparada=NULL;
	free(segmentoABuscar);
	segmentoABuscar=NULL;
}
