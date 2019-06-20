#include "kernel.h"

int main(void) {
	inicializarVariables();

	log_info(logger_KERNEL, "----------------INICIO DE KERNEL--------------");

	//Hilos
	pthread_create(&hiloConectarAMemoria, NULL, (void*)conectarAMemoria, NULL);
	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);
	pthread_create(&hiloPlanificarNew, NULL, (void*)planificarNewAReady, NULL);
	pthread_create(&hiloPlanificarExec, NULL, (void*)planificarReadyAExec, NULL);
	pthread_create(&hiloMetricas, NULL, (void*)loguearMetricas, NULL);
	pthread_create(&hiloDescribe, NULL, (void*)hacerDescribe, NULL);

	pthread_join(hiloConectarAMemoria, NULL);
	pthread_join(hiloLeerDeConsola, NULL);
	pthread_join(hiloPlanificarNew, NULL);
	pthread_join(hiloPlanificarExec, NULL);
	pthread_join(hiloMetricas, NULL);
	pthread_join(hiloDescribe, NULL);

	liberarMemoria();

	return EXIT_SUCCESS;
}

/* inicializarVariables()
 * Parametros:
 * 	-> void
 * Descripcion: inicializa todas las variables que se van a usar.
 * Return:
 * 	-> :: void  */
void inicializarVariables() {
	logger_KERNEL = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	logger_METRICAS_KERNEL = log_create("metricas.log", "MetricasKernel", 0, LOG_LEVEL_DEBUG);
	// Configs
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config");
	int multiprocesamiento = config_get_int_value(config, "MULTIPROCESAMIENTO");
	quantum = config_get_int_value(config, "QUANTUM");
	// Semáforos
	sem_init(&semRequestNew, 0, 0);
	sem_init(&semRequestReady, 0, 0);
	sem_init(&semMultiprocesamiento, 0, multiprocesamiento);
	pthread_mutex_init(&semMColaNew, NULL);
	pthread_mutex_init(&semMColaReady, NULL);
	pthread_mutex_init(&semMMetricas, NULL);
	// Colas de planificacion
	new = queue_create();
	ready = queue_create();
	// Memorias
	memoriasEc = list_create();
	memoriasShc = list_create();
	memorias = list_create();
	memoriaSc = (config_memoria*) malloc(sizeof(config_memoria));
	memoriaSc = NULL;
	// Tablas
	tablasSC = list_create();
	tablasSHC = list_create();
	tablasEC = list_create();
	// Metricas
	cargaMemoriaSC = list_create();
	cargaMemoriaSHC = list_create();
	cargaMemoriaEC = list_create();
}

/* liberarMemoria()
 * Parametros:
 * 	-> void
 * Descripcion: libera los recursos.
 * Return:
 * 	-> void  */
void liberarMemoria(void) {
	log_info(logger_KERNEL, "Estoy liberando toda la memoria, chau");
	liberar_conexion(conexionMemoria);
	config_destroy(config);
	log_destroy(logger_KERNEL);
	log_destroy(logger_METRICAS_KERNEL);
	pthread_mutex_destroy(&semMColaNew);
	pthread_mutex_destroy(&semMColaReady);
	pthread_mutex_destroy(&semMMetricas);
	sem_destroy(&semRequestNew);
	sem_destroy(&semRequestReady);
	sem_destroy(&semMultiprocesamiento);
	queue_destroy(new);
	queue_destroy(ready);
	//list_clean_and_destroy_elements(memorias, (void*)liberarConfigMemoria);
	list_destroy(memorias);
	liberarConfigMemoria(memoriaSc);
	list_destroy(memoriasShc);
	list_destroy(memoriasEc);
	list_clean_and_destroy_elements(tablasSC, (void*)liberarTabla);
	list_clean_and_destroy_elements(tablasSHC, (void*)liberarTabla);
	list_clean_and_destroy_elements(tablasEC, (void*)liberarTabla);
	list_clean_and_destroy_elements(cargaMemoriaSC, (void*)liberarEstadisticaMemoria);
	list_clean_and_destroy_elements(cargaMemoriaSHC, (void*)liberarEstadisticaMemoria);
	list_clean_and_destroy_elements(cargaMemoriaEC, (void*)liberarEstadisticaMemoria);
}

/* conectarAMemoria()
 * Parametros:
 * 	-> void
 * Descripcion: conecta kernel con memoria.
 * Return:
 * 	-> :: void  */
void conectarAMemoria(void) {
	t_handshake_memoria* handshake;
	conexionMemoria = crearConexion(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"));
	handshake = recibirHandshakeMemoria(conexionMemoria);
	procesarHandshake(handshake);
	liberarHandshakeMemoria(handshake);
}

/* procesarHandshake()
 * Parametros:
 * 	-> t_handshake_memoria* :: handshakeRecibido
 * Descripcion: guarda las memorias levantadas en una lista.
 * Return:
 * 	-> :: void  */
void procesarHandshake(t_handshake_memoria* handshakeRecibido) {
	size_t i = 0;
	char** ips = string_split(handshakeRecibido->ips, ",");
	char** puertos = string_split(handshakeRecibido->puertos, ",");
	char** numeros = string_split(handshakeRecibido->numeros, ",");
	for( i = 0; ips[i] != NULL; i++)
	{
		config_memoria* memoriaNueva = (config_memoria*) malloc(sizeof(config_memoria));
		memoriaNueva->ip = strdup(ips[i]);// config_get_string_value(config, "IP_MEMORIA");
		memoriaNueva->puerto = strdup(puertos[i]);// config_get_string_value(config, "PUERTO_MEMORIA");
		memoriaNueva->numero = strdup(numeros[i]);
		log_info(logger_KERNEL, "estoy agregando el ip: %s puerto: %s numero: %s", memoriaNueva->ip, memoriaNueva->puerto, memoriaNueva->numero);
		list_add(memorias, memoriaNueva);
		memoriaNueva = NULL;
	}
	liberarArrayDeChar(ips);
	liberarArrayDeChar(puertos);
	liberarArrayDeChar(numeros);
}

/* leerDeConsola()
 * Parametros:
 * 	-> void
 * Descripcion: es un hilo que lee input de consola y agrega los requests a cola de new.
 * Return:
 * 	-> :: void  */
void leerDeConsola(void){
	char* mensaje;
	while (1) {
		mensaje = readline(">");
		if (!strcmp(mensaje, "\0")) {
			pthread_cancel(hiloConectarAMemoria);
			pthread_cancel(hiloPlanificarNew);
			pthread_cancel(hiloPlanificarExec);
			pthread_cancel(hiloMetricas);
			pthread_cancel(hiloDescribe);
			free(mensaje);
			break;
		}
		pthread_mutex_lock(&semMColaNew);
		//agregar request a la cola de new
		queue_push(new, mensaje);
		pthread_mutex_unlock(&semMColaNew);
		sem_post(&semRequestNew);
	}
}

/* hacerDescribe()
 * Parametros:
 * 	-> void
 * Descripcion: cada x segundos hace describe global.
 * Return:
 * 	-> :: void  */
void hacerDescribe(void) {
	while(1) {
		break;
		int segundos = config_get_int_value(config, "METADATA_REFRESH");
		// agregar a new? o ejecutarlo directamente?
		usleep(segundos * 1000);
	}
}

/* loguearMetricas()
 * Parametros:
 * 	-> void
 * Descripcion: cada 30 segundos loguea data.
 * Return:
 * 	-> :: void  */
void loguearMetricas(void) {
	while(1) {
		log_info(logger_KERNEL, "pasaron 30s");
		informarMetricas(FALSE);
		// limpio variables para empezar a contar de nuevo
		pthread_mutex_lock(&semMMetricas);
		tiempoSelectSC = 0;
		tiempoInsertSC = 0;
		cantidadSelectSC = 0;
		cantidadInsertSC = 0;
		if (list_size(cargaMemoriaSC) > 0) {
			list_clean_and_destroy_elements(cargaMemoriaSC, (void*)liberarEstadisticaMemoria);
		}
		tiempoSelectSHC = 0;
		tiempoInsertSHC = 0;
		cantidadSelectSHC = 0;
		cantidadInsertSHC = 0;
		if (list_size(cargaMemoriaSHC) > 0) {
			list_clean_and_destroy_elements(cargaMemoriaSHC, (void*)liberarEstadisticaMemoria);
		}
		tiempoSelectEC = 0;
		tiempoInsertEC = 0;
		cantidadSelectEC = 0;
		cantidadInsertEC = 0;
		if (list_size(cargaMemoriaEC) > 0) {
			list_clean_and_destroy_elements(cargaMemoriaEC, (void*)liberarEstadisticaMemoria);
		}
		pthread_mutex_unlock(&semMMetricas);
		sleep(30);
	}
}

/* informarMetricas()
 * Parametros:
 * 	-> int :: mostrarPorConsola
 * Descripcion: si parametro es true muestra metricas actuales por consola,
 * sino en archivo de log de métricas.
 * Return:
 * 	-> :: void  */
void informarMetricas(int mostrarPorConsola) {
	// si es vacio no mostrar basura
	pthread_mutex_lock(&semMMetricas);
	double readLatencySC = tiempoSelectSC/cantidadSelectSC;
	double readLatencySHC = tiempoSelectSHC/cantidadSelectSHC;
	double readLatencyEC = tiempoSelectEC/cantidadSelectEC;
	double writeLatencySC = tiempoInsertSC/cantidadInsertSC;
	double writeLatencySHC = tiempoInsertSHC/cantidadInsertSHC;
	double writeLatencyEC = tiempoInsertEC/cantidadInsertEC;
	void mostrarCargaMemoria(estadisticaMemoria* estadisticaAMostrar) {
		log_info(logger_KERNEL, "select insert %d y total %d", estadisticaAMostrar->cantidadSelectInsert, estadisticaAMostrar->cantidadTotal);
		double estadistica = (double)estadisticaAMostrar->cantidadSelectInsert / estadisticaAMostrar->cantidadTotal;
		if (mostrarPorConsola == TRUE) {
			log_info(logger_KERNEL, "La cantidad de Select - Insert respecto del resto de las operaciones de la memoria %s es %f",
					estadisticaAMostrar->numeroMemoria, estadistica);
		} else {
			log_info(logger_METRICAS_KERNEL, "La cantidad de Select - Insert respecto del resto de las operaciones de la memoria %s es %f",
					estadisticaAMostrar->numeroMemoria, estadistica);
		}
	}
	if (mostrarPorConsola == TRUE) {
		log_info(logger_KERNEL, "Read Latency de SC: %f", readLatencySC);
		log_info(logger_KERNEL, "Read Latency de SHC: %f", readLatencySHC);
		log_info(logger_KERNEL, "Read Latency de EC: %f", readLatencyEC);
		log_info(logger_KERNEL, "Write Latency de SC: %f", writeLatencySC);
		log_info(logger_KERNEL, "Write Latency de SHC: %f", writeLatencySHC);
		log_info(logger_KERNEL, "Write Latency de EC: %f", writeLatencyEC);
		if (list_size(cargaMemoriaSC) > 0) {
			log_info(logger_KERNEL, "El memory load de SC es:");
			list_iterate(cargaMemoriaSC, (void*) mostrarCargaMemoria);
		}
		if (list_size(cargaMemoriaSHC) > 0) {
			log_info(logger_KERNEL, "El memory load de SHC es:");
			list_iterate(cargaMemoriaSHC, (void*) mostrarCargaMemoria);
		}
		if (list_size(cargaMemoriaEC) > 0) {
			log_info(logger_KERNEL, "El memory load de EC es:");
			list_iterate(cargaMemoriaEC, (void*) mostrarCargaMemoria);
		}
		log_info(logger_KERNEL, "Cantidad de Reads de SC: %d", cantidadSelectSC);
		log_info(logger_KERNEL, "Cantidad de Reads de SHC: %d", cantidadSelectSHC);
		log_info(logger_KERNEL, "Cantidad de Reads de EC: %d", cantidadSelectEC);
		log_info(logger_KERNEL, "Cantidad de Writes de SC: %d", cantidadInsertSC);
		log_info(logger_KERNEL, "Cantidad de Writes de SHC: %d", cantidadInsertSHC);
		log_info(logger_KERNEL, "Cantidad de Writes de EC: %d", cantidadInsertEC);
	} else {
		log_info(logger_METRICAS_KERNEL, "Read Latency de SC: %f", readLatencySC);
		log_info(logger_METRICAS_KERNEL, "Read Latency de SHC: %f", readLatencySHC);
		log_info(logger_METRICAS_KERNEL, "Read Latency de EC: %f", readLatencyEC);
		log_info(logger_METRICAS_KERNEL, "Write Latency de SC: %f", writeLatencySC);
		log_info(logger_METRICAS_KERNEL, "Write Latency de SHC: %f", writeLatencySHC);
		log_info(logger_METRICAS_KERNEL, "Write Latency de EC: %f", writeLatencyEC);
		if (list_size(cargaMemoriaSC) > 0) {
			log_info(logger_METRICAS_KERNEL, "El memory load de SC es:");
			list_iterate(cargaMemoriaSC, (void*) mostrarCargaMemoria);
		}
		if (list_size(cargaMemoriaSHC) > 0) {
			log_info(logger_METRICAS_KERNEL, "El memory load de SHC es:");
			list_iterate(cargaMemoriaSHC, (void*) mostrarCargaMemoria);
		}
		if (list_size(cargaMemoriaEC) > 0) {
			log_info(logger_METRICAS_KERNEL, "El memory load de EC es:");
			list_iterate(cargaMemoriaEC, (void*) mostrarCargaMemoria);
		}
		log_info(logger_METRICAS_KERNEL, "Cantidad de Reads de SC: %d", cantidadSelectSC);
		log_info(logger_METRICAS_KERNEL, "Cantidad de Reads de SHC: %d", cantidadSelectSHC);
		log_info(logger_METRICAS_KERNEL, "Cantidad de Reads de EC: %d", cantidadSelectEC);
		log_info(logger_METRICAS_KERNEL, "Cantidad de Writes de SC: %d", cantidadInsertSC);
		log_info(logger_METRICAS_KERNEL, "Cantidad de Writes de SHC: %d", cantidadInsertSHC);
		log_info(logger_METRICAS_KERNEL, "Cantidad de Writes de EC: %d", cantidadInsertEC);
	}
	pthread_mutex_unlock(&semMMetricas);
}

/* liberarEstadisticaMemoria()
 * Parametros:
 * 	-> estadisticaMemoria* :: memoria
 * Descripcion: recibe una estadisticaMemoria y la libera.
 * Return:
 * 	-> :: void  */
void liberarEstadisticaMemoria(estadisticaMemoria* memoria) {
	free(memoria->numeroMemoria);
	free(memoria);
}
/* liberarConfigMemoria()
 * Parametros:
 * 	-> config_memoria* :: configALiberar
 * Descripcion: recibe una configMemoria y la libera.
 * Return:
 * 	-> :: void  */

void liberarConfigMemoria(config_memoria* configALiberar) {
	if (configALiberar != NULL) {
		free(configALiberar->ip);
		free(configALiberar->numero);
		free(configALiberar->puerto);
	}
	free(configALiberar)
;}

/* aumentarContadores()
 * Parametros:
 * 	-> consistencia :: consistenciaRequest
 * 	-> char* :: numeroMemoria
 * 	-> cod_request* :: codigo
 * 	-> int :: cantidadTiempo
 * Descripcion: aumenta las variables para las métricas.
 * Return:
 * 	-> :: void  */
void aumentarContadores(consistencia consistenciaRequest, char* numeroMemoria, cod_request codigo, double cantidadTiempo) {
	estadisticaMemoria* memoriaCorrespondiente;
	// si es un describe global va a entrar al "NINGUNA"
	// cargo en el criterio segun el request o segun la memoria?
	// o sea si tengo una memory que es SC y EC y le hago dos select que son de una tabla
	// EC, en la cantidad total de request en mi lista de SC de esa memoria es de 2 o 0? porque
	// no se hizo nada SC pero si se hizo a esa memoria PREGUNTAR
	int encontrarTabla(estadisticaMemoria* memoria) {
		return string_equals_ignore_case(memoria->numeroMemoria, numeroMemoria);
	}
	int cantidadASumarSelectInsert = codigo == SELECT || codigo == INSERT;
	pthread_mutex_lock(&semMMetricas);
	switch (consistenciaRequest) {
		case SC:
			memoriaCorrespondiente = list_find(cargaMemoriaSC, (void*)encontrarTabla);
			if (memoriaCorrespondiente == NULL) {
				memoriaCorrespondiente = NULL;
				memoriaCorrespondiente = (estadisticaMemoria*) malloc(sizeof(estadisticaMemoria));
				memoriaCorrespondiente->cantidadSelectInsert = cantidadASumarSelectInsert;
				memoriaCorrespondiente->cantidadTotal = 1;
				memoriaCorrespondiente->numeroMemoria = strdup(numeroMemoria);
				list_add(cargaMemoriaSC, memoriaCorrespondiente);
			} else {
				// se actualiza solo? porque no es un puntero un int, ver si hay que hacer otra cosa
				memoriaCorrespondiente->cantidadSelectInsert += cantidadASumarSelectInsert;
				memoriaCorrespondiente->cantidadTotal = memoriaCorrespondiente->cantidadTotal + 1;
			}
			log_info(logger_KERNEL, "voy a sumarle a select insert %d", cantidadASumarSelectInsert);
			break;
		case SHC:
			memoriaCorrespondiente = list_find(cargaMemoriaSHC, (void*)encontrarTabla);
			if (memoriaCorrespondiente == NULL) {
				memoriaCorrespondiente = NULL;
				memoriaCorrespondiente = (estadisticaMemoria*) malloc(sizeof(estadisticaMemoria));
				memoriaCorrespondiente->cantidadSelectInsert = cantidadASumarSelectInsert;
				memoriaCorrespondiente->cantidadTotal = 1;
				memoriaCorrespondiente->numeroMemoria = strdup(numeroMemoria);
				list_add(cargaMemoriaSHC, memoriaCorrespondiente);
			} else {
				// se actualiza solo? porque no es un puntero un int, ver si hay que hacer otra cosa
				memoriaCorrespondiente->cantidadSelectInsert += cantidadASumarSelectInsert;
				memoriaCorrespondiente->cantidadTotal = memoriaCorrespondiente->cantidadTotal + 1;
			}
			break;
		case EC:
			memoriaCorrespondiente = list_find(cargaMemoriaEC, (void*)encontrarTabla);
			if (memoriaCorrespondiente == NULL) {
				memoriaCorrespondiente = NULL;
				memoriaCorrespondiente = (estadisticaMemoria*) malloc(sizeof(estadisticaMemoria));
				memoriaCorrespondiente->cantidadSelectInsert = cantidadASumarSelectInsert;
				memoriaCorrespondiente->cantidadTotal = 1;
				memoriaCorrespondiente->numeroMemoria = strdup(numeroMemoria);
				list_add(cargaMemoriaEC, memoriaCorrespondiente);
			} else {
				// se actualiza solo? porque no es un puntero un int, ver si hay que hacer otra cosa
				memoriaCorrespondiente->cantidadSelectInsert += cantidadASumarSelectInsert;
				memoriaCorrespondiente->cantidadTotal = memoriaCorrespondiente->cantidadTotal + 1;
			}
			break;
		case NINGUNA:
			// se agrega en todos los criterios? o en el criterio de la memoria? PREGUNTAR
			break;
		default:
			// error
			break;
	}
	if (codigo == SELECT) {
		switch(consistenciaRequest) {
		case SC:
			tiempoSelectSC+=cantidadTiempo;
			cantidadSelectSC++;
			break;
		case SHC:
			tiempoSelectSHC+=cantidadTiempo;
			cantidadSelectSHC++;
			break;
		case EC:
			tiempoSelectEC+=cantidadTiempo;
			cantidadSelectEC++;
			break;
		case NINGUNA:
			// que se hace en el caso de describe global?
			break;
		default:
			break;
		}
	} else if (codigo == INSERT) {
		switch(consistenciaRequest) {
		case SC:
			tiempoInsertSC+=cantidadTiempo;
			cantidadInsertSC++;
			break;
		case SHC:
			tiempoInsertSHC+=cantidadTiempo;
			cantidadInsertSHC++;
			break;
		case EC:
			tiempoInsertEC+=cantidadTiempo;
			cantidadInsertEC++;
			break;
		case NINGUNA:
			// que se hace en el caso de describe global?
			break;
		default:
			break;
		}
	}
	pthread_mutex_unlock(&semMMetricas);
}

/*
 Planificación
*/

/* planificarNewAReady()
 * Parametros:
 * 	-> void
 * Descripcion: hilo que, luego de que un request es agregado a la cola de new, lo toma
 * y reserva recursos si y solo si es válido - en el caso de RUN solo valida si el RUN es válido,
 * no las requests dentro del archivo.
 * Return:
 * 	-> :: void  */
void planificarNewAReady(void) {
	while(1) {
		sem_wait(&semRequestNew);
		pthread_mutex_lock(&semMColaNew);
		char* request = strdup((char*) queue_peek(new));
		free((char*) queue_pop(new));
		pthread_mutex_unlock(&semMColaNew);
		if(validarRequest(request) == TRUE) {
			//cuando es run, en vez de pushear request, se pushea array de requests, antes se llama a reservar recursos que hace eso
			reservarRecursos(request);
		} else {
			//error
		}
	}
}

/* planificarReadyAExec()
 * Parametros:
 * 	-> void
 * Descripcion: hilo que, luego de que un request es agregado a la cola de ready, lo toma
 * y si no se superó el nivel de multiprocesamiento, lo ejecuta y lo agrega a la cola de exec.
 * Return:
 * 	-> :: void  */
void planificarReadyAExec(void) {
	pthread_t hiloRequest;
	pthread_attr_t attr;
	request_procesada* request;
	int threadProcesar;
	while(1) {
		sem_wait(&semRequestReady);
		sem_wait(&semMultiprocesamiento);
		request = (request_procesada*) malloc(sizeof(request_procesada));
		pthread_mutex_lock(&semMColaReady);
		request->codigo = ((request_procesada*) queue_peek(ready))->codigo;
		if(request->codigo == RUN) {
			request->request = (t_queue*) malloc(sizeof(((request_procesada*)queue_peek(ready))->request));
			request->request = ((request_procesada*)queue_peek(ready))->request;
		} else {
			request->request = strdup((char*)((request_procesada*)queue_peek(ready))->request);
		}
		queue_pop(ready);
		pthread_mutex_unlock(&semMColaReady);
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		threadProcesar = pthread_create(&hiloRequest, &attr, (void*)procesarRequest, request);
		if(threadProcesar == 0){
			pthread_attr_destroy(&attr);
		} else {
			//informar error
		}
	}
}

/* reservarRecursos()
 * Parametros:
 * 	-> char* :: mensaje
 * Descripcion: toma request y reserva recursos. Si es RUN lee el archivo y va agregando a una cola,
 * despues esta es agregada a la cola de ready,
 * sino, simplemente lo agrega a la cola de ready.
 * Return:
 * 	-> :: void  */
void reservarRecursos(char* mensaje) {
	char** request = string_n_split(mensaje, 2, " ");
	cod_request codigo = obtenerCodigoPalabraReservada(request[0], KERNEL);
	request_procesada* _request = (request_procesada*) malloc(sizeof(request_procesada));
	if (codigo == RUN) {
		//desarmo archivo y lo pongo en una cola
		FILE *archivoLql;
		char** parametros;
		parametros = separarRequest(mensaje);
		archivoLql = fopen(parametros[1], "r");
		if (archivoLql == NULL) {
			log_error(logger_KERNEL, "No existe un archivo en esa ruta");
		} else {
			_request->request = queue_create();
			char letra;
			char* request;
			char** requestDividida;
			int i = 1;
			size_t posicion = 0;
			request = (char*) malloc(sizeof(char));
		    *request = '\0';
			while((letra = fgetc(archivoLql)) != EOF) {
				if (letra != '\n') {
					// concateno
					i++;
					request = (char*) realloc(request, i * sizeof(char));
					posicion = strlen(request);
					request[posicion] = letra;
					request[posicion + 1] = '\0';
				} else {
					request_procesada* otraRequest = (request_procesada*) malloc(sizeof(request_procesada));
					requestDividida = string_n_split(request, 2, " ");
					cod_request _codigo = obtenerCodigoPalabraReservada(requestDividida[0], KERNEL);
					otraRequest->codigo = _codigo;
					otraRequest->request = strdup(request);
					queue_push(_request->request, otraRequest);
					liberarArrayDeChar(requestDividida);
					i = 1;
					free(request);
					request = NULL;
					request = (char*) malloc(sizeof(char));
				    *request = '\0';
				}
			}
			if (i != 1) { //hack horrible para que funcione cuando los lql tienen salto de linea al final y cuando no tmb
				//esto esta para la ultima linea, porque como no tiene salto de linea no entra al else y no se guarda sino
				request_procesada* otraRequest = (request_procesada*) malloc(sizeof(request_procesada));
				requestDividida = string_n_split(request, 2, " ");
				cod_request _codigo = obtenerCodigoPalabraReservada(requestDividida[0], KERNEL);
				otraRequest->codigo = _codigo;
				otraRequest->request = strdup(request);
				queue_push(_request->request, otraRequest);
				liberarArrayDeChar(requestDividida);
			}
			free(request);
			request = NULL;
			liberarArrayDeChar(parametros);
			fclose(archivoLql);
		}
		_request->codigo = RUN;
		pthread_mutex_lock(&semMColaReady);
		queue_push(ready, _request);
		pthread_mutex_unlock(&semMColaReady);
	} else {
		_request->request = strdup(mensaje);
		_request->codigo = codigo;
		pthread_mutex_lock(&semMColaReady);
		queue_push(ready, _request);
		pthread_mutex_unlock(&semMColaReady);
	}
	liberarArrayDeChar(request);
	sem_post(&semRequestReady);
}

/* liberarRequestProcesada()
 * Parametros:
 * 	-> request_procesada* :: request
 * Descripcion: libera los recursos de la request.
 * Return:
 * 	-> void  */
void liberarRequestProcesada(request_procesada* request) {
	if(request->codigo != RUN) {
       free((char*) request->request);
       request->request = NULL;
	}
	// en el caso del run ya se libera la cola en su funcion
	// solo hacer este free si request fue a exit
	free(request);
	request = NULL;
}

/* liberarColaRequest()
 * Parametros:
 *     -> request_procesada* :: requestCola
 * Descripcion: recibe una request que se encuentra dentro de una cola y la libera.
 * Return:
 *     -> void  */
void liberarColaRequest(request_procesada* requestCola) {
	//printf("Voy a liberar: %s    ", (char*) requestCola->request);
    free((char*) requestCola->request);
    requestCola->request = NULL;
    free(requestCola);
    requestCola = NULL;
}

/* procesarRequest()
 * Parametros:
 * 	-> request_procesada* :: request
 * Descripcion: hilo que se crea dinámicamente, desde planificarReadyAExec, maneja la request y libera sus recursos.
 * Return:
 * 	-> :: void  */
void procesarRequest(request_procesada* request) {
	manejarRequest(request);
	liberarRequestProcesada(request);
	sem_post(&semMultiprocesamiento);
}

/* validarRequest()
 * Parametros:
 * 	-> char* :: mensaje
 * Descripcion: toma un request y se fija si es válido.
 * Return:
 * 	-> requestEsValida :: bool  */
int validarRequest(char* mensaje) {
	int codValidacion = validarMensaje(mensaje, KERNEL, logger_KERNEL);
	switch(codValidacion) {
		case EXIT_SUCCESS:
			return TRUE;
			break;
		case EXIT_FAILURE:
		case NUESTRO_ERROR:
			return FALSE;
			//informar error
			break;
		default:
			return FALSE;
			break;
	}
}

/*
 * Se ocupa de delegar la request
*/

/* manejarRequest()
 * Parametros:
 * 	-> request_procesada* :: request
 * Descripcion: toma un request y se fija a quién delegarselo, toma la respuesta y la devuelve.
 * Return:
 * 	-> respuesta :: t_paquete*  */
int manejarRequest(request_procesada* request) {
	int respuesta = 0;
	//devolver la respuesta para usarla en el run
	switch(request->codigo) {
		case SELECT:
		case INSERT:
		case CREATE:
		case DESCRIBE:
		case DROP:
			respuesta = enviarMensajeAMemoria(request->codigo, (char*) request->request);
			break;
		case JOURNAL:
			procesarJournal(FALSE);
			// solo a memorias que tengan un criterio
			break;
		case ADD:
			respuesta = procesarAdd((char*) request->request);
			break;
		case RUN:
			procesarRun((t_queue*) request->request);
			break;
		case METRICS:
			informarMetricas(TRUE);
			break;
		default:
			// aca puede entrar solo si viene de run, porque sino antes siempre fue validada
			// la request. en tal caso se devuelve error para cortar ejecucion
			break;
	}
	usleep(config_get_int_value(config, "SLEEP_EJECUCION")*1000);
	return respuesta;
}

/* actualizarTablas()
 * Parametros:
 * 	-> char* :: respuesta
 * Descripcion: toma una rta del DESCRIBE y actualiza data de las tablas.
 * Return:
 * 	-> void */
void actualizarTablas(char* respuesta) {
	// formato de rta: tabla consistencia particiones tiempoDeCompactacion;
	// separo por ; y despues itero sobre eso
	char** respuestaParseada = string_split(respuesta, ";");
	int esDescribeGlobal = longitudDeArrayDeStrings(respuestaParseada) != 1;
	// solo limpiar cuando es global!!!
	if (esDescribeGlobal == TRUE) {
		list_clean_and_destroy_elements(tablasSC, (void*)liberarTabla);
		list_clean_and_destroy_elements(tablasSHC, (void*)liberarTabla);
		list_clean_and_destroy_elements(tablasEC, (void*)liberarTabla);
		string_iterate_lines(respuestaParseada, (void*)agregarTablaACriterio);
	} else {
		agregarTablaACriterio(respuestaParseada[0]);
	}
	liberarArrayDeChar(respuestaParseada);
}

/* recorrerTabla()
 * Parametros:
 * 	-> char* :: tabla
 * Descripcion: toma una tabla, se fija que consistencia tiene y la guarda donde corresponde si no existe.
 * Si ya existe no hace nada y si la tiene que actualizar lo hace.
 * Return:
 * 	-> void */
void agregarTablaACriterio(char* tabla) {
	// busco la tabla y actualizo si cambio la consistencia y si no la creo
	char** tablaParseada = string_split(tabla, " ");
	char* nombreTabla = strdup(tablaParseada[0]);
	char* consistenciaTabla = tablaParseada[1];
	consistencia tipoConsistencia = obtenerEnumConsistencia(consistenciaTabla);
	int hayQueAgregar = FALSE;
	int esTabla(char* tablaActual) {
		return string_equals_ignore_case(nombreTabla, tablaActual);
	}
	if (list_find(tablasSC, (void*)esTabla) != NULL) {
		if (tipoConsistencia != SC) {
			hayQueAgregar = TRUE;
			list_remove_and_destroy_by_condition(tablasSC, (void*)esTabla, (void*)liberarTabla);
		} else {
			free(nombreTabla);
		}
	} else if (list_find(tablasSHC, (void*)esTabla) != NULL) {
		if (tipoConsistencia != SHC) {
			hayQueAgregar = TRUE;
			list_remove_and_destroy_by_condition(tablasSHC, (void*)esTabla, (void*)liberarTabla);
		} else {
			free(nombreTabla);
		}
	} else if (list_find(tablasEC, (void*)esTabla) != NULL) {
		if (tipoConsistencia != EC) {
			hayQueAgregar = TRUE;
			list_remove_and_destroy_by_condition(tablasEC, (void*)esTabla, (void*)liberarTabla);
		} else {
			free(nombreTabla);
		}
	} else {
		hayQueAgregar = TRUE;
	}
	if (hayQueAgregar == TRUE) {
		// tabla no existe en estructura o hay que agregarla en otra
		switch(tipoConsistencia) {
			case SC:
				list_add(tablasSC, nombreTabla);
				log_info(logger_KERNEL, "Agregue la tabla %s al criterio SC", nombreTabla);
				break;
			case SHC:
				list_add(tablasSHC, nombreTabla);
				log_info(logger_KERNEL, "Agregue la tabla %s al criterio SHC", nombreTabla);
				break;
			case EC:
				list_add(tablasEC, nombreTabla);
				log_info(logger_KERNEL, "Agregue la tabla %s al criterio EC", nombreTabla);
				break;
			default:
				log_error(logger_KERNEL, "La tabla %s no tiene asociada un criterio válido y no se actualizó en la estructura de datos",
						nombreTabla);
				break;
		}
	}
	liberarArrayDeChar(tablaParseada);
}

/* liberarTabla()
 * Parametros:
 * 	-> char* :: tabla
 * Descripcion: toma una tabla y la libera.
 * Return:
 * 	-> void */
void liberarTabla(char* tabla) {
	free(tabla);
	tabla = NULL;
}

/* obtenerConsistenciaTabla()
 * Parametros:
 * 	-> char* :: tabla
 * Descripcion: toma una tabla y devuelve el criterio correspondiente.
 * Return:
 * 	-> consistenciaCorrespondiente :: consistencia*  */
consistencia obtenerConsistenciaTabla(char* tabla) {
	int esTabla(char* tablaActual) {
		return string_equals_ignore_case(tabla, tablaActual);
	}
	if (list_find(tablasSC, (void*) esTabla) != NULL) {
		return SC;
	} else if (list_find(tablasSHC, (void*) esTabla) != NULL) {
		return SHC;
	} else if(list_find(tablasEC, (void*) esTabla) != NULL) {
		return EC;
	} else {
		return SC;
	}
}

/* encontrarMemoriaSegunConsistencia()
 * Parametros:
 * 	-> char* :: tabla
 * 	-> char* :: key
 * Descripcion: toma una tabla y devuelve la memoria según el criterio.
 * En el caso de SHC necesita la key para calcular a qué memoria redirigir.
 * Return:
 * 	-> memoriaCorrespondiente :: config_memoria*  */
config_memoria* encontrarMemoriaSegunConsistencia(consistencia tipoConsistencia) {
	config_memoria* memoriaCorrespondiente = NULL;
	switch(tipoConsistencia) {
		case SC:
			if (memoriaSc == NULL) {
				log_error(logger_KERNEL, "No se puede resolver el request porque no hay memorias asociadas al criterio SC");
			} else {
				memoriaCorrespondiente = memoriaSc;
			}
			break;
		case SHC:
			// todo: funcion hash
			break;
		case EC:
			// todo: funcion random
			break;
		default:
			//error
			log_error(logger_KERNEL, "No se puede resolver el request porque no tengo la metadata de la tabla");
			break;
	}
	return memoriaCorrespondiente;
}

/* encontrarMemoriaPpal()
 * Parametros:
 * 	-> config_memoria* :: memoria
 * Descripcion: devuelve TRUE cuando encuentra la memoria ppal.
 * Return:
 * 	-> int  */
int encontrarMemoriaPpal(config_memoria* memoria) {
	return string_equals_ignore_case(memoria->ip, config_get_string_value(config, "IP_MEMORIA"))
			&& string_equals_ignore_case(memoria->puerto, config_get_string_value(config, "PUERTO_MEMORIA"));
}
/*
 * Funciones que procesan requests
*/

/* enviarMensajeAMemoria()
 * Parametros:
 * 	-> cod_request :: codRequest
 * 	-> char* :: mensaje
 * Descripcion: recibe una request, se la manda a memoria y recibe la respuesta.
 * Return:
 * 	-> paqueteRecibido :: t_paquete*  */
int enviarMensajeAMemoria(cod_request codigo, char* mensaje) {
	clock_t tiempo;
	double tiempoQueTardo = 0.0;
	if (codigo == SELECT || codigo == INSERT) {
		tiempo = clock();
	}
	t_paquete* paqueteRecibido;
	int respuesta;
	consistencia consistenciaTabla = NINGUNA;
	int conexionTemporanea = conexionMemoria;
	char** parametros = separarRequest(mensaje);
	// si es un describe global no hay tabla
	int cantidadParametros = longitudDeArrayDeStrings(parametros) - 1;
	config_memoria* memoriaCorrespondiente;
	if (codigo == DESCRIBE && cantidadParametros == PARAMETROS_DESCRIBE_GLOBAL) {
		// describe global va siempre a la ppal, todo: que pasa si se desconecta?
		memoriaCorrespondiente = list_find(memorias, (void*)encontrarMemoriaPpal);
	} else {
		if (codigo == CREATE) {
			consistenciaTabla = obtenerEnumConsistencia(parametros[2]);
		} else {
			consistenciaTabla = obtenerConsistenciaTabla(parametros[1]);
		}
		memoriaCorrespondiente = encontrarMemoriaSegunConsistencia(consistenciaTabla);
		if(memoriaCorrespondiente == NULL) {
			respuesta = ERROR_GENERICO;
			liberarArrayDeChar(parametros);
			return respuesta;
		} else {
			if (!string_equals_ignore_case(memoriaCorrespondiente->ip, config_get_string_value(config, "IP_MEMORIA"))
				&& !string_equals_ignore_case(memoriaCorrespondiente->puerto, config_get_string_value(config, "PUERTO_MEMORIA"))) {
				conexionTemporanea = crearConexion(memoriaCorrespondiente->ip, memoriaCorrespondiente->puerto);
			}
		}
	}
	enviar(consistenciaTabla, mensaje, conexionTemporanea);
	paqueteRecibido = recibir(conexionTemporanea);
	respuesta = paqueteRecibido->palabraReservada;
	// todo: si la respuesta es full, forzar journal y mandar request de vuelta?
	if (respuesta == SUCCESS) {
		log_info(logger_KERNEL, "La respuesta del request %s es %s \n", mensaje, paqueteRecibido->request);
		if (codigo == DESCRIBE) {
			actualizarTablas(paqueteRecibido->request);
		}
	} else {
		log_error(logger_KERNEL, "El request %s no es válido y me llegó como rta %s", mensaje, paqueteRecibido->request);
	}
	if (conexionTemporanea != conexionMemoria) {
		liberar_conexion(conexionTemporanea);
	}
	eliminar_paquete(paqueteRecibido);
	//free(memoriaCorrespondiente);
	liberarArrayDeChar(parametros);
	if (codigo == SELECT || codigo == INSERT) {
		tiempo = clock() - tiempo;
		tiempoQueTardo = ((double)tiempo)/CLOCKS_PER_SEC; // in seconds
	}
	aumentarContadores(consistenciaTabla, memoriaCorrespondiente->numero, codigo, tiempoQueTardo);
	return respuesta;
}

/* procesarJournal()
 * Parametros:
 * 	-> soloASHC :: int
 * Descripcion: manda journal a todas las memorias que estén asociadas a un criterio.
 * En el caso de que una memoria es agregada al criterio SHC, se manda
 * un JOURNAL solo a esas memorias.
 * Return:
 * 	-> void */
void procesarJournal(int soloASHC) {
	// ahora recorro la lista filtrada y creo las conexiones para mandar journal
	// si la memoria es la ppal que ya estoy conectada, no me tengo que conectar
	void enviarJournal(config_memoria* memoriaAConectarse) {
		int conexionTemporanea = conexionMemoria;
		if (!string_equals_ignore_case(memoriaAConectarse->ip, config_get_string_value(config, "IP_MEMORIA"))
			&& !string_equals_ignore_case(memoriaAConectarse->puerto, config_get_string_value(config, "PUERTO_MEMORIA"))) {
			conexionTemporanea = crearConexion(memoriaAConectarse->ip, memoriaAConectarse->puerto);
		}
		enviar(NINGUNA, "JOURNAL", conexionTemporanea);
		t_paquete* paqueteRecibido = recibir(conexionTemporanea);
		int respuesta = paqueteRecibido->palabraReservada;
		if (respuesta == SUCCESS) {
			log_info(logger_KERNEL, "La respuesta del request %s es %s \n", "JOURNAL", paqueteRecibido->request);
		} else {
			log_error(logger_KERNEL, "El request %s no es válido", "JOURNAL");
		}
		if (conexionTemporanea != conexionMemoria) {
			liberar_conexion(conexionTemporanea);
		}
		eliminar_paquete(paqueteRecibido);
	}
	if(soloASHC == TRUE) {
		list_iterate(memoriasShc, (void*)enviarJournal);
	} else {
		t_list* memoriasSinRepetir = list_create();
		void agregarMemoriaSinRepetir(config_memoria* memoria) {
			int existeUnaIgual(config_memoria* memoriaAgregada) {
				return string_equals_ignore_case(memoriaAgregada->ip, memoria->ip) &&
						string_equals_ignore_case(memoriaAgregada->puerto, memoria->puerto);
			}
			if (!list_any_satisfy(memoriasSinRepetir, (void*)existeUnaIgual)) {
				list_add(memoriasSinRepetir, memoria);
			}
		}
		// me guardo una lista con puerto e ip sii no existe en la lista
		// porque una memoria puede tener + de 1 criterio
		if(memoriaSc != NULL) {
			list_add(memoriasSinRepetir, memoriaSc);
		}
		list_iterate(memoriasShc,(void*)agregarMemoriaSinRepetir);
		list_iterate(memoriasEc,(void*)agregarMemoriaSinRepetir);
		list_iterate(memoriasSinRepetir, (void*)enviarJournal);
		list_destroy(memoriasSinRepetir);
	}

}

/* procesarRun()
 * Parametros:
 * 	-> t_queue* :: colaRun
 * Descripcion: recibe una cola de requests, procesa cada una,
 * si es válida la ejecuta. Cuando es fin de quantum, vuelve a ready.
 * Return:
 * 	-> void  */
void procesarRun(t_queue* colaRun) {
	// usar inotify por si cambia Q
	request_procesada* request;
	int quantumActual = 0;
	//el while es fin de Q o fin de cola
	while(!queue_is_empty(colaRun) && quantumActual < quantum) {
		request = (request_procesada*) malloc(sizeof(request_procesada));
		request->codigo = ((request_procesada*) queue_peek(colaRun))->codigo;
		request->request = strdup((char*)((request_procesada*)queue_peek(colaRun))->request);

		if (validarRequest((char*) request->request) == TRUE) {
			if (manejarRequest(request) != SUCCESS) {
				break;
				//libero recursos, mato hilo, lo saco de la cola, e informo error
			}
		} else {
			break;
			//libero recursos, mato hilo, lo saco de la cola, e informo error
		}
		queue_pop(colaRun);
		liberarRequestProcesada(request);
		quantumActual++;
	}
	if (quantumActual == quantum && queue_is_empty(colaRun) == FALSE) {
		//termino por fin de q
		log_info(logger_KERNEL, "Vuelvo a ready");
		request_procesada* _request = (request_procesada*)(malloc(sizeof(request_procesada)));
		_request->request = (t_queue*) (malloc(sizeof(t_queue)));
		_request->request = colaRun;
		_request->codigo = RUN;
		pthread_mutex_lock(&semMColaReady);
		queue_push(ready, _request);
		pthread_mutex_unlock(&semMColaReady);
		sem_post(&semRequestReady);
	} else if (queue_is_empty(colaRun) == TRUE){
		// si estoy aca es porque ya ejecuto toda la cola
		log_info(logger_KERNEL, "Finalizó la ejecución del script");
		queue_destroy(colaRun);
	} else {
		log_error(logger_KERNEL, "Se cancelo el script");
		liberarRequestProcesada(request);
		queue_destroy_and_destroy_elements(colaRun, (void*)liberarColaRequest);
		// liberar recursos que ocupa el struct este con sus colas o sea el request - es lo de arriba
	}
}

/* procesarAdd()
 * Parametros:
 * 	-> char* :: mensaje
 * Descripcion: recibe el request y asigna criterio a memoria.
 * Luego le informa a memoria que se le asignó ese criterio.
 * Return:
 * 	-> void  */
int procesarAdd(char* mensaje) {
	int estado = SUCCESS;
	char** requestDividida = separarRequest(mensaje);
	config_memoria* memoria = (config_memoria*) malloc(sizeof(config_memoria));
	consistencia _consistencia;
	_consistencia = obtenerEnumConsistencia(requestDividida[4]);
	if (_consistencia == CONSISTENCIA_INVALIDA) {
		estado = ERROR_GENERICO;
		log_error(logger_KERNEL, "El criterio %s no es válido", requestDividida[4]);
	}
	int esMemoriaCorrecta(config_memoria* memoriaActual) {
		return string_equals_ignore_case(memoriaActual->numero, requestDividida[2]);
	}
	memoria = (config_memoria*) list_find(memorias, (void*)esMemoriaCorrecta);
	if (memoria == NULL) {
		estado = ERROR_GENERICO;
		log_error(logger_KERNEL, "No encontré la memoria %s", requestDividida[2]);
	} else {
		log_info(logger_KERNEL, "voy a asignar a la mem ip %s puerto %s", memoria->ip, memoria->puerto);
		switch (_consistencia) {
			case SC:
				memoriaSc = memoria;
				break;
			case SHC:
				list_add(memoriasShc, memoria);
				//procesarJournal(TRUE);
				break;
			case EC:
				list_add(memoriasEc, memoria);
				break;
			default:
				break;
		}
	}
	//free(mensaje);
	liberarArrayDeChar(requestDividida);
	//free(memoria); hay que hacer este free???
	return estado;
}
