#include "kernel.h"

int main(void) {
	inicializarVariables();

	log_info(logger_KERNEL, "----------------INICIO DE KERNEL--------------");


	//Hilos
	pthread_create(&hiloConectarAMemoria, NULL, (void*)hacerGossiping, NULL);
	pthread_create(&hiloPlanificarNew, NULL, (void*)planificarNewAReady, NULL);
	pthread_create(&hiloPlanificarExec, NULL, (void*)planificarReadyAExec, NULL);
	pthread_create(&hiloMetricas, NULL, (void*)loguearMetricas, NULL);
	pthread_create(&hiloDescribe, NULL, (void*)hacerDescribe, NULL);
	pthread_create(&hiloCambioEnConfig, NULL, (void*)escucharCambiosEnConfig, NULL);
	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);

	pthread_join(hiloConectarAMemoria, NULL);
	pthread_join(hiloPlanificarNew, NULL);
	pthread_join(hiloPlanificarExec, NULL);
	pthread_join(hiloMetricas, NULL);
	pthread_join(hiloDescribe, NULL);
	pthread_join(hiloCambioEnConfig, NULL);
	pthread_join(hiloLeerDeConsola, NULL);

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
	logger_KERNEL = log_create("kernel.log", "Kernel", 0, LOG_LEVEL_DEBUG);
	logger_METRICAS_KERNEL = log_create("metricas.log", "MetricasKernel", 0, LOG_LEVEL_DEBUG);

	// Configs
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config");
	int multiprocesamiento = config_get_int_value(config, "MULTIPROCESAMIENTO");
	quantum = config_get_int_value(config, "QUANTUM");
	sleepEjecucion = config_get_int_value(config, "SLEEP_EJECUCION");
	metadataRefresh = config_get_int_value(config, "METADATA_REFRESH");
	numeroSeedRandom = config_get_int_value(config, "DAIU_NUMBER");
	puertoMemoria = strdup(config_get_string_value(config, "PUERTO_MEMORIA"));
	ipMemoria = strdup(config_get_string_value(config, "IP_MEMORIA"));
	srand(numeroSeedRandom);
	sleepGossiping = config_get_int_value(config, "SLEEP_GOSSIPING");
	ipPpal = strdup(config_get_string_value(config, "IP_MEMORIA"));
	puertoPpal = strdup(config_get_string_value(config, "PUERTO_MEMORIA"));

	// Semáforos
	sem_init(&semRequestNew, 0, 0);
	sem_init(&semRequestReady, 0, 0);
	sem_init(&semMultiprocesamiento, 0, multiprocesamiento);
	pthread_mutex_init(&semMColaNew, NULL);
	pthread_mutex_init(&semMColaReady, NULL);
	pthread_mutex_init(&semMMetricas, NULL);
	pthread_mutex_init(&semMTablasSC, NULL);
	pthread_mutex_init(&semMTablasSHC, NULL);
	pthread_mutex_init(&semMTablasEC, NULL);
	pthread_mutex_init(&semMQuantum, NULL);
	pthread_mutex_init(&semMSleepEjecucion, NULL);
	pthread_mutex_init(&semMMetadataRefresh, NULL);
	pthread_mutex_init(&semMMemoriaSC, NULL);
	pthread_mutex_init(&semMMemoriasSHC, NULL);
	pthread_mutex_init(&semMMemoriasEC, NULL);
	pthread_mutex_init(&semMMemorias, NULL);
	pthread_mutex_init(&semMConfig, NULL);
	pthread_mutex_init(&semMSleepGossiping, NULL);

	// Colas de planificacion
	new = queue_create();
	ready = queue_create();
	// Memorias
	memoriasEc = list_create();
	memoriasShc = list_create();
	memorias = list_create();
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
	log_info(logger_KERNEL, "Estoy liberando toda la memoria...");
	config_destroy(config);
	free(ipMemoria);
	free(puertoMemoria);
	free(ipPpal);
	free(puertoPpal);

	pthread_mutex_destroy(&semMColaNew);
	pthread_mutex_destroy(&semMColaReady);
	pthread_mutex_destroy(&semMMetricas);
	pthread_mutex_destroy(&semMTablasSC);
	pthread_mutex_destroy(&semMTablasSHC);
	pthread_mutex_destroy(&semMTablasEC);
	pthread_mutex_destroy(&semMQuantum);
	pthread_mutex_destroy(&semMSleepEjecucion);
	pthread_mutex_destroy(&semMMetadataRefresh);
	pthread_mutex_destroy(&semMMemoriaSC);
	pthread_mutex_destroy(&semMMemoriasSHC);
	pthread_mutex_destroy(&semMMemoriasEC);
	pthread_mutex_destroy(&semMMemorias);
	pthread_mutex_destroy(&semMConfig);
	pthread_mutex_destroy(&semMSleepGossiping);

	sem_destroy(&semRequestNew);
	sem_destroy(&semRequestReady);
	sem_destroy(&semMultiprocesamiento);

	queue_destroy(new);
	queue_destroy(ready);

	list_destroy_and_destroy_elements(memorias, (void*)liberarConfigMemoria);
	liberarConfigMemoria(memoriaSc);
	list_destroy_and_destroy_elements(memoriasShc, (void*)liberarConfigMemoria);
	list_destroy_and_destroy_elements(memoriasEc, (void*)liberarConfigMemoria);
	list_destroy_and_destroy_elements(tablasSC, (void*)liberarTabla);
	list_destroy_and_destroy_elements(tablasSHC, (void*)liberarTabla);
	list_destroy_and_destroy_elements(tablasEC, (void*)liberarTabla);
	list_destroy_and_destroy_elements(cargaMemoriaSC, (void*)liberarEstadisticaMemoria);
	list_destroy_and_destroy_elements(cargaMemoriaSHC, (void*)liberarEstadisticaMemoria);
	list_destroy_and_destroy_elements(cargaMemoriaEC, (void*)liberarEstadisticaMemoria);

	inotify_rm_watch(file_descriptor, watch_descriptor);
	close(file_descriptor);

	log_info(logger_KERNEL, "Memoria liberada, chau");
	log_destroy(logger_KERNEL);
	log_destroy(logger_METRICAS_KERNEL);
}

/* hacerGossiping()
 * Parametros:
 * 	-> void
 * Descripcion: hace gossiping cada x cantidad de tiempo.
 * Return:
 * 	-> :: void  */
void hacerGossiping(void) {
	t_gossiping* gossiping;
	int sleep;
	// data de la mem ppal
	char* numeroPpal = strdup("");
	// data de la memoria a la que estoy conectada
	char* ipActual = strdup(ipPpal);
	char* puertoActual = strdup(puertoPpal);
	char* numeroActual = strdup("");
	int indice = 0;
	int maximo = 0;
	int resultado;
	int conexion = conectarseAMemoria(GOSSIPING, puertoPpal, ipPpal, "");
	int encontrarMemPpal(config_memoria* unaMemoria) {
		return string_equals_ignore_case(unaMemoria->ip, ipPpal) && string_equals_ignore_case(unaMemoria->puerto, puertoPpal);
	}
	if (conexion == FAILURE) {
		log_error(logger_KERNEL, "La mem ppal no está levantada");
	} else {
		resultado = enviarGossiping("", "", "", conexion);
		if (resultado == COMPONENTE_CAIDO) {
			log_error(logger_KERNEL, "La mem ppal no está levantada");
			eliminarMemoria(puertoActual, ipActual, numeroActual);
		} else {
			gossiping = recibirGossiping(conexion, &resultado);
			if (resultado == COMPONENTE_CAIDO) {
				log_error(logger_KERNEL, "La mem ppal no está levantada");
				eliminarMemoria(puertoActual, ipActual, numeroActual);
			} else {
				// solo procesar si resultado es success
				procesarGossiping(gossiping);
				liberar_conexion(conexion);
				liberarHandshakeMemoria(gossiping);
				// guardo numero de memoria para mandar como param
				pthread_mutex_lock(&semMMemorias);
				config_memoria* memPpal = list_find(memorias, (void*)encontrarMemPpal);
				numeroPpal = strdup(memPpal->numero);
				pthread_mutex_unlock(&semMMemorias);
				// como la ppal esta levanta va a ser la mem actual
				free(numeroActual);
				numeroActual = NULL;
				numeroActual = strdup(numeroPpal);
			}
		}
	}
	while(1) {
		pthread_mutex_lock(&semMSleepGossiping);
		sleep = sleepGossiping;
		pthread_mutex_unlock(&semMSleepGossiping);
		usleep(sleep * 1000);
		log_info(logger_KERNEL, "Haciendo gossiping");
		conexion = conectarseAMemoria(GOSSIPING, puertoActual, ipActual, numeroActual);
		if (conexion == FAILURE) {
			log_error(logger_KERNEL, "La mem %s no está levantada, me voy a conectar con otra memoria", numeroActual);
		} else {
			//todo: chequear que memoria no se haya caido
			resultado = enviarGossiping("", "", "", conexion);
			if (resultado == COMPONENTE_CAIDO) {
				eliminarMemoria(puertoActual, ipActual, numeroActual);
			} else {
				int resultado2;
				gossiping = recibirGossiping(conexion, &resultado2);
				if (resultado2 == COMPONENTE_CAIDO) {
					eliminarMemoria(puertoActual, ipActual, numeroActual);
				} else {
					procesarGossiping(gossiping);
					if (string_equals_ignore_case(numeroPpal, "")) {
						free(numeroPpal);
						numeroPpal = NULL;
						// si no tengo el num de la mem ppal la tengo que conseguir en alguna vuelta de gossiping
						pthread_mutex_lock(&semMMemorias);
						config_memoria* memPpal = list_find(memorias, (void*)encontrarMemPpal);
						numeroPpal = strdup(memPpal->numero);
						pthread_mutex_unlock(&semMMemorias);
						numeroActual = strdup(numeroPpal);
					}
				}
				liberarHandshakeMemoria(gossiping);
			}
			liberar_conexion(conexion);
		}
		// obtengo data de otra memoria para pedirle el gossiping a otra
		pthread_mutex_lock(&semMMemorias);
		maximo = list_size(memorias);
		pthread_mutex_unlock(&semMMemorias);
		// me voy a conectar a otra memoria
		free(ipActual);
		free(puertoActual);
		free(numeroActual);
		ipActual = NULL;
		puertoActual = NULL;
		numeroActual = NULL;
		if (maximo == 0) {
			// no hay memorias levantadas
			log_info(logger_KERNEL, "No hay ninguna memoria levantada, me voy a conectar con la mem ppal");
			ipActual = strdup(ipPpal);
			puertoActual = strdup(puertoPpal);
			numeroActual = strdup(numeroPpal);
		} else {
			indice = obtenerIndiceRandom(maximo);
			pthread_mutex_lock(&semMMemorias);
			config_memoria* memoriaAConectar = list_get(memorias, indice);
			if (memoriaAConectar != NULL) {
				ipActual = strdup(memoriaAConectar->ip);
				puertoActual = strdup(memoriaAConectar->puerto);
				numeroActual = strdup(memoriaAConectar->numero);
				pthread_mutex_unlock(&semMMemorias);
			} else {
				pthread_mutex_unlock(&semMMemorias);
				ipActual = strdup(ipPpal);
				puertoActual = strdup(puertoPpal);
				numeroActual = strdup(numeroPpal);
			}
		}
	}
	free(ipPpal);
	free(puertoPpal);
	free(numeroPpal);
}

/* procesarGossiping()
 * Parametros:
 * 	-> t_gossiping* :: gossipingRecibido
 * Descripcion: guarda las memorias levantadas en una lista.
 * Return:
 * 	-> :: void  */
void procesarGossiping(t_gossiping* gossipingRecibido) {
	size_t i = 0;
	log_warning(logger_KERNEL, "%s --- %s ---- %s", gossipingRecibido->ips, gossipingRecibido->puertos, gossipingRecibido->numeros);
	char** ips = string_split(gossipingRecibido->ips, ",");
	char** puertos = string_split(gossipingRecibido->puertos, ",");
	char** numeros = string_split(gossipingRecibido->numeros, ",");
	for(i = 0; ips[i] != NULL; i++)
	{
		config_memoria* memoriaNueva = (config_memoria*) malloc(sizeof(config_memoria));
		memoriaNueva->ip = strdup(ips[i]);
		memoriaNueva->puerto = strdup(puertos[i]);
		memoriaNueva->numero = strdup(numeros[i]);

		int existeUnaIgual(config_memoria* memoriaAgregada) {
			return string_equals_ignore_case(memoriaAgregada->ip, memoriaNueva->ip) &&
					string_equals_ignore_case(memoriaAgregada->puerto, memoriaNueva->puerto) &&
					string_equals_ignore_case(memoriaAgregada->numero, memoriaNueva->numero);
		}

		pthread_mutex_lock(&semMMemorias);
		if (!list_any_satisfy(memorias, (void*)existeUnaIgual)) {
			// agrego memoria si no existe en mi lista de memorias
			list_add(memorias, memoriaNueva);
			pthread_mutex_unlock(&semMMemorias);
			log_info(logger_KERNEL, "Me llegó una nueva memoria en el gossiping, num: %s", memoriaNueva->numero);
		} else {
			pthread_mutex_unlock(&semMMemorias);
			liberarConfigMemoria(memoriaNueva);
		}
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
			pthread_cancel(hiloCambioEnConfig);
			free(mensaje);
			break;
		}
		planificarRequest(mensaje);
	}
}

/* planificarRequest()
 * Parametros:
 * 	-> char* :: request
 * Descripcion: recibe una request, si es ADD o METRICS lo ejecuta directamente, sino lo agrega a la cola de new.
 * Return:
 * 	-> :: void  */
void planificarRequest(char* request) {
	char** requestSeparada = string_n_split(request, 2, " ");
	cod_request codigo = obtenerCodigoPalabraReservada(requestSeparada[0], KERNEL);
	if (codigo == ADD || codigo == METRICS) {
		// https://github.com/sisoputnfrba/foro/issues/1382
		pthread_t hiloRequest;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		int threadProcesar = pthread_create(&hiloRequest, &attr, (void*)procesarRequestSinPlanificar, request);
		if(threadProcesar == 0){
			pthread_attr_destroy(&attr);
		} else {
			log_error(logger_KERNEL, "Hubo un error al crear el hilo que ejecuta la request");
		}
	} else {
		pthread_mutex_lock(&semMColaNew);
		//agregar request a la cola de new
		queue_push(new, request);
		pthread_mutex_unlock(&semMColaNew);
		sem_post(&semRequestNew);
	}
	liberarArrayDeChar(requestSeparada);
}

void procesarRequestSinPlanificar(char* request) {
	char** requestSeparada = string_n_split(request, 2, " ");
	cod_request codigo = obtenerCodigoPalabraReservada(requestSeparada[0], KERNEL);
	request_procesada* requestArmada = (request_procesada*) malloc(sizeof(request_procesada));
	requestArmada->codigo = codigo;
	requestArmada->request = strdup(request);
	manejarRequest(requestArmada, FALSE);
	liberarRequestProcesada(requestArmada);
	free(request);
	liberarArrayDeChar(requestSeparada);
}

/* escucharCambiosEnConfig()
 * Parametros:
 * 	-> void
 * Descripcion: es un hilo que escucha cambios en el config y actualiza las variables correspodientes.
 * Return:
 * 	-> :: void  */
void escucharCambiosEnConfig(void) {
	char buffer[BUF_LEN];
	file_descriptor = inotify_init();
	if (file_descriptor < 0) {
		log_error(logger_KERNEL, "Inotify no se pudo inicializar correctamente");
	}
	int hayError = 0;
	watch_descriptor = inotify_add_watch(file_descriptor, "/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config", IN_MODIFY);
	while(1) {
		log_info(logger_KERNEL, "Watch vale %d", watch_descriptor);
		int length = read(file_descriptor, buffer, BUF_LEN);
		log_info(logger_KERNEL, "Cambió el archivo de config");
		pthread_mutex_lock(&semMConfig);
		config_destroy(config);
		config = leer_config("/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config");
		pthread_mutex_unlock(&semMConfig);
		if (length < 0) {
			log_error(logger_KERNEL, "Error en inotify");
		} else {
			pthread_mutex_lock(&semMQuantum);
			if(config_has_property(config, "QUANTUM")){
				quantum = config_get_int_value(config, "QUANTUM");
				pthread_mutex_unlock(&semMQuantum);
			}else{
				pthread_mutex_unlock(&semMQuantum);
				hayError = 1;
			}
			pthread_mutex_lock(&semMSleepEjecucion);
			if(config_has_property(config, "SLEEP_EJECUCION")){
				sleepEjecucion = config_get_int_value(config, "SLEEP_EJECUCION");
				pthread_mutex_unlock(&semMSleepEjecucion);
			}else{
				pthread_mutex_unlock(&semMSleepEjecucion);
				hayError = 1;
			}
			pthread_mutex_lock(&semMMetadataRefresh);
			if(config_has_property(config, "METADATA_REFRESH")){
				metadataRefresh = config_get_int_value(config, "METADATA_REFRESH");
				pthread_mutex_unlock(&semMMetadataRefresh);
			}else{
				pthread_mutex_unlock(&semMMetadataRefresh);
				hayError = 1;
			}
			pthread_mutex_lock(&semMSleepGossiping);
			if(config_has_property(config, "SLEEP_GOSSIPING")){
				sleepGossiping = config_get_int_value(config, "SLEEP_GOSSIPING");
				pthread_mutex_unlock(&semMSleepGossiping);
			}else{
				pthread_mutex_unlock(&semMSleepGossiping);
				hayError = 1;
			}
			if (hayError) {
				log_error(logger_KERNEL, "Error en inotify, sus cambios no han sido tomados, por vuelva a cambiar el archivo");
			}
			hayError = 0;
		}

		inotify_rm_watch(file_descriptor, watch_descriptor);
		close(file_descriptor);


		file_descriptor = inotify_init();
		if (file_descriptor < 0) {
			log_error(logger_KERNEL, "Inotify no se pudo inicializar correctamente");
		}

		watch_descriptor = inotify_add_watch(file_descriptor, "/home/utnso/tp-2019-1c-bugbusters/kernel/kernel.config", IN_MODIFY);
	}
}

/* hacerDescribe()
 * Parametros:
 * 	-> void
 * Descripcion: cada x segundos hace describe global.
 * Return:
 * 	-> :: void  */
void hacerDescribe(void) {
	int tiempoSleep = 0;
	while(1) {
		log_info(logger_KERNEL, "Haciendo DESCRIBE automático");
		char* request = strdup("DESCRIBE");
		procesarRequestSinPlanificar(request);
		pthread_mutex_lock(&semMMetadataRefresh);
		tiempoSleep = metadataRefresh;
		pthread_mutex_unlock(&semMMetadataRefresh);
		usleep(tiempoSleep * 1000);
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
		informarMetricas(FALSE);
		// limpio variables para empezar a contar de nuevo
		pthread_mutex_lock(&semMMetricas);
		tiempoSelectSC = 0.0;
		tiempoInsertSC = 0.0;
		cantidadSelectSC = 0;
		cantidadInsertSC = 0;
		if (list_size(cargaMemoriaSC) > 0) {
			list_clean_and_destroy_elements(cargaMemoriaSC, (void*)liberarEstadisticaMemoria);
		}
		tiempoSelectSHC = 0.0;
		tiempoInsertSHC = 0.0;
		cantidadSelectSHC = 0;
		cantidadInsertSHC = 0;
		if (list_size(cargaMemoriaSHC) > 0) {
			list_clean_and_destroy_elements(cargaMemoriaSHC, (void*)liberarEstadisticaMemoria);
		}
		// https://github.com/sisoputnfrba/foro/issues/1434#issuecomment-510291991
		// todo: memory load es en total o 30secs? https://github.com/sisoputnfrba/foro/issues/1434#issuecomment-510291991
		tiempoSelectEC = 0.0;
		tiempoInsertEC = 0.0;
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
	int cantidadTotalSelectInsert = cantidadSelectSC + cantidadSelectSHC + cantidadSelectEC + cantidadInsertSC + cantidadInsertSHC + cantidadInsertEC;
	void mostrarCargaMemoria(estadisticaMemoria* estadisticaAMostrar) {
		// https://github.com/sisoputnfrba/foro/issues/1419
		double estadistica = (double)estadisticaAMostrar->cantidadSelectInsert / (double)cantidadTotalSelectInsert;
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
		log_info(logger_KERNEL, "El memory load de SC es:");
		list_iterate(cargaMemoriaSC, (void*) mostrarCargaMemoria);
		log_info(logger_KERNEL, "El memory load de SHC es:");
		list_iterate(cargaMemoriaSHC, (void*) mostrarCargaMemoria);
		log_info(logger_KERNEL, "El memory load de EC es:");
		list_iterate(cargaMemoriaEC, (void*) mostrarCargaMemoria);
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
		log_info(logger_METRICAS_KERNEL, "El memory load de SC es:");
		list_iterate(cargaMemoriaSC, (void*) mostrarCargaMemoria);
		log_info(logger_METRICAS_KERNEL, "El memory load de SHC es:");
		list_iterate(cargaMemoriaSHC, (void*) mostrarCargaMemoria);
		log_info(logger_METRICAS_KERNEL, "El memory load de EC es:");
		list_iterate(cargaMemoriaEC, (void*) mostrarCargaMemoria);
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
		configALiberar->ip = NULL;
		configALiberar->puerto = NULL;
		configALiberar->numero = NULL;
	}
	free(configALiberar);
	configALiberar = NULL;
}

/* aumentarContadores()
 * Parametros:
 * 	-> consistencia :: consistenciaRequest
 * 	-> char* :: numeroMemoria
 * 	-> cod_request* :: codigo
 * 	-> int :: cantidadTiempo
 * Descripcion: aumenta las variables para las métricas.
 * Return:
 * 	-> :: void  */
void aumentarContadores(char* numeroMemoria, cod_request codigo, double cantidadTiempo, consistencia consistenciaRequest) {
	estadisticaMemoria* memoriaCorrespondiente;
		int encontrarTabla(estadisticaMemoria* memoria) {
			//todo: ver si sigue rompiendo en la memoria
			// AHHH cuando una memoria se cae no se borra de aca ? es eso? o ya lo habia arreglado porque era un free de mas mio?
			return string_equals_ignore_case(memoria->numeroMemoria, numeroMemoria);
		}
		pthread_mutex_lock(&semMMetricas);
		switch (consistenciaRequest) {
			case SC:
				memoriaCorrespondiente = list_find(cargaMemoriaSC, (void*)encontrarTabla);
				if (memoriaCorrespondiente == NULL) {
					memoriaCorrespondiente = NULL;
					memoriaCorrespondiente = (estadisticaMemoria*) malloc(sizeof(estadisticaMemoria));
					memoriaCorrespondiente->cantidadSelectInsert = 1;
					memoriaCorrespondiente->numeroMemoria = strdup(numeroMemoria);
					list_add(cargaMemoriaSC, memoriaCorrespondiente);
				} else {
					memoriaCorrespondiente->cantidadSelectInsert ++;
				}
				break;
			case SHC:
				memoriaCorrespondiente = list_find(cargaMemoriaSHC, (void*)encontrarTabla);
				if (memoriaCorrespondiente == NULL) {
					memoriaCorrespondiente = NULL;
					memoriaCorrespondiente = (estadisticaMemoria*) malloc(sizeof(estadisticaMemoria));
					memoriaCorrespondiente->cantidadSelectInsert = 1;
					memoriaCorrespondiente->numeroMemoria = strdup(numeroMemoria);
					list_add(cargaMemoriaSHC, memoriaCorrespondiente);
				} else {
					memoriaCorrespondiente->cantidadSelectInsert ++;
				}
				break;
			case EC:
				memoriaCorrespondiente = list_find(cargaMemoriaEC, (void*)encontrarTabla);
				if (memoriaCorrespondiente == NULL) {
					memoriaCorrespondiente = NULL;
					memoriaCorrespondiente = (estadisticaMemoria*) malloc(sizeof(estadisticaMemoria));
					memoriaCorrespondiente->cantidadSelectInsert = 1;
					memoriaCorrespondiente->numeroMemoria = strdup(numeroMemoria);
					list_add(cargaMemoriaEC, memoriaCorrespondiente);
				} else {
					memoriaCorrespondiente->cantidadSelectInsert ++;
				}
				break;
			default:
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
			free(request);
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
		log_debug(logger_KERNEL, "Nuevo request en exec");
		request = (request_procesada*) malloc(sizeof(request_procesada));
		pthread_mutex_lock(&semMColaReady);
		request->codigo = ((request_procesada*) queue_peek(ready))->codigo;
		if(request->codigo == RUN) {
			request->request = ((request_procesada*)queue_peek(ready))->request;
		} else {
			request->request = strdup((char*)((request_procesada*)queue_peek(ready))->request);
			free((char*)((request_procesada*)queue_peek(ready))->request);
		}
		free(queue_pop(ready));
		pthread_mutex_unlock(&semMColaReady);
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		threadProcesar = pthread_create(&hiloRequest, &attr, (void*)procesarRequest, request);
		if(threadProcesar == 0){
			pthread_attr_destroy(&attr);
		} else {
			log_error(logger_KERNEL, "Hubo un error al crear el hilo que ejecuta la request");
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
			liberarArrayDeChar(parametros);
			liberarArrayDeChar(request);
			free(mensaje);
			free(_request);
			return;
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
					// validar qye request dividida no sea null
					cod_request _codigo = obtenerCodigoPalabraReservada(requestDividida[0], KERNEL);
					otraRequest->codigo = _codigo;
					otraRequest->request = strdup(request);
					queue_push(_request->request, otraRequest);
					liberarArrayDeChar(requestDividida);
					i = 1;
					otraRequest = NULL;
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
	free(mensaje);
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
       free(request->request);
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
	manejarRequest(request, FALSE);
	liberarRequestProcesada(request);
	request = NULL;
	sem_post(&semMultiprocesamiento);
}

/* validarRequest()
 * Parametros:
 * 	-> char* :: mensaje
 * Descripcion: toma un request y se fija si es válido.
 * Return:
 * 	-> requestEsValida :: bool  */
int validarRequest(char* mensaje) {
	char* mensajeError;
	char* req = strdup(mensaje);
	if(validarMensaje(mensaje, KERNEL, &mensajeError) == SUCCESS){
		free(req);
		return TRUE;
	}else{
		log_error(logger_KERNEL, "%s. Request que generó error: %s", mensajeError, mensaje);
		return FALSE;
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
int manejarRequest(request_procesada* request, int fromRun) {
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
			if (fromRun) {
				log_error(logger_KERNEL, "El JOURNAL no va adentro de un RUN");
				respuesta = FAILURE;
			} else {
				procesarJournal(FALSE);
				// solo a memorias que tengan un criterio
			}
			break;
		case ADD:
			if (fromRun) {
				log_error(logger_KERNEL, "El ADD no va adentro de un RUN");
				respuesta = FAILURE;
			} else {
				respuesta = procesarAdd((char*) request->request);
			}
			break;
		case RUN:
			if (fromRun) {
				log_error(logger_KERNEL, "Metiste un run adentro de un run y es ilegal, no lo voy a ejecutar!");
				respuesta = FAILURE;
			} else {
				procesarRun((t_queue*) request->request);
			}
			break;
		case METRICS:
			if (fromRun) {
				log_error(logger_KERNEL, "El METRICS no va adentro de un RUN");
				respuesta = FAILURE;
			} else {
				informarMetricas(TRUE);
			}
			break;
		default:
			// aca puede entrar solo si viene de run, porque sino antes siempre fue validada
			// la request. en tal caso se devuelve error para cortar ejecucion
			break;
	}
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
	// todo: chequear que variable este funcionando como se espera?
	// solo limpiar cuando es global!!!
	if (esDescribeGlobal == TRUE) {
		pthread_mutex_lock(&semMTablasSC);
		list_clean_and_destroy_elements(tablasSC, (void*)liberarTabla);
		pthread_mutex_unlock(&semMTablasSC);
		pthread_mutex_lock(&semMTablasSHC);
		list_clean_and_destroy_elements(tablasSHC, (void*)liberarTabla);
		pthread_mutex_unlock(&semMTablasSHC);
		pthread_mutex_lock(&semMTablasEC);
		list_clean_and_destroy_elements(tablasEC, (void*)liberarTabla);
		pthread_mutex_unlock(&semMTablasEC);
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
	pthread_mutex_lock(&semMTablasSC);
	if (list_find(tablasSC, (void*)esTabla) != NULL) {
		if (tipoConsistencia != SC) {
			list_remove_and_destroy_by_condition(tablasSC, (void*)esTabla, (void*)liberarTabla);
			pthread_mutex_unlock(&semMTablasSC);
			hayQueAgregar = TRUE;
		} else {
			pthread_mutex_unlock(&semMTablasSC);
			free(nombreTabla);
		}
	} else {
		pthread_mutex_unlock(&semMTablasSC);
		pthread_mutex_lock(&semMTablasSHC);
		if (list_find(tablasSHC, (void*)esTabla) != NULL) {
			if (tipoConsistencia != SHC) {
				list_remove_and_destroy_by_condition(tablasSHC, (void*)esTabla, (void*)liberarTabla);
				pthread_mutex_unlock(&semMTablasSHC);
				hayQueAgregar = TRUE;
			} else {
				pthread_mutex_unlock(&semMTablasSHC);
				free(nombreTabla);
			}
		} else {
			pthread_mutex_unlock(&semMTablasSHC);
			pthread_mutex_lock(&semMTablasEC);
			if (list_find(tablasEC, (void*)esTabla) != NULL) {
				if (tipoConsistencia != EC) {
					list_remove_and_destroy_by_condition(tablasEC, (void*)esTabla, (void*)liberarTabla);
					pthread_mutex_unlock(&semMTablasEC);
					hayQueAgregar = TRUE;
				} else {
					pthread_mutex_unlock(&semMTablasEC);
					free(nombreTabla);
				}
			} else {
				pthread_mutex_unlock(&semMTablasEC);
				hayQueAgregar = TRUE;
			}
		}
	}

	if (hayQueAgregar == TRUE) {
		// tabla no existe en estructura o hay que agregarla en otra
		switch(tipoConsistencia) {
			case SC:
				pthread_mutex_lock(&semMTablasSC);
				list_add(tablasSC, nombreTabla);
				pthread_mutex_unlock(&semMTablasSC);
				log_info(logger_KERNEL, "Agregue la tabla %s al criterio SC", nombreTabla);
				break;
			case SHC:
				pthread_mutex_lock(&semMTablasSHC);
				list_add(tablasSHC, nombreTabla);
				pthread_mutex_unlock(&semMTablasSHC);
				log_info(logger_KERNEL, "Agregue la tabla %s al criterio SHC", nombreTabla);
				break;
			case EC:
				pthread_mutex_lock(&semMTablasEC);
				list_add(tablasEC, nombreTabla);
				pthread_mutex_unlock(&semMTablasEC);
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
	pthread_mutex_lock(&semMTablasSC);
	if (list_find(tablasSC, (void*) esTabla) != NULL) {
		pthread_mutex_unlock(&semMTablasSC);
		return SC;
	} else {
		pthread_mutex_unlock(&semMTablasSC);
		pthread_mutex_lock(&semMTablasSHC);
		if (list_find(tablasSHC, (void*) esTabla) != NULL) {
			pthread_mutex_unlock(&semMTablasSHC);
			return SHC;
		} else {
			pthread_mutex_unlock(&semMTablasSHC);
			pthread_mutex_lock(&semMTablasEC);
			if(list_find(tablasEC, (void*) esTabla) != NULL) {
				pthread_mutex_unlock(&semMTablasEC);
				return EC;
			} else {
				pthread_mutex_unlock(&semMTablasEC);
				log_error(logger_KERNEL, "No sé que consistencia tiene la tabla %s", tabla);
				return CONSISTENCIA_INVALIDA;
			}
		}
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
config_memoria* encontrarMemoriaSegunConsistencia(consistencia tipoConsistencia, int key) {
	config_memoria* memoriaCorrespondiente = NULL;
	switch(tipoConsistencia) {
		case SC:
			pthread_mutex_lock(&semMMemoriaSC);
			if (memoriaSc == NULL) {
				pthread_mutex_unlock(&semMMemoriaSC);
				log_error(logger_KERNEL, "No se puede resolver el request porque no hay memorias asociadas al criterio SC");
			} else {
				memoriaCorrespondiente = (config_memoria*) malloc(sizeof(config_memoria));
				memoriaCorrespondiente->ip = strdup(memoriaSc->ip);
				memoriaCorrespondiente->puerto = strdup(memoriaSc->puerto);
				memoriaCorrespondiente->numero = strdup(memoriaSc->numero);
				pthread_mutex_unlock(&semMMemoriaSC);
			}
			break;
		case SHC:
			pthread_mutex_lock(&semMMemoriasSHC);
			if (list_size(memoriasShc) != 0) {
				unsigned int indiceSHC = obtenerIndiceHash(key, list_size(memoriasShc));
				config_memoria* memAux = list_get(memoriasShc, indiceSHC);
				memoriaCorrespondiente = (config_memoria*) malloc(sizeof(config_memoria));
				memoriaCorrespondiente->ip = strdup(memAux->ip);
				memoriaCorrespondiente->puerto = strdup(memAux->puerto);
				memoriaCorrespondiente->numero = strdup(memAux->numero);
				pthread_mutex_unlock(&semMMemoriasSHC);
			} else {
				pthread_mutex_unlock(&semMMemoriasSHC);
				log_error(logger_KERNEL, "No se puede resolver el request porque no hay memorias asociadas al criterio SHC");
			}
			break;
		case EC:
			pthread_mutex_lock(&semMMemoriasEC);
			if (list_size(memoriasEc) != 0) {
				unsigned int indiceEC = obtenerIndiceRandom(list_size(memoriasEc));
				config_memoria* memAux = list_get(memoriasEc, indiceEC);
				memoriaCorrespondiente = (config_memoria*) malloc(sizeof(config_memoria));
				memoriaCorrespondiente->ip = strdup(memAux->ip);
				memoriaCorrespondiente->puerto = strdup(memAux->puerto);
				memoriaCorrespondiente->numero = strdup(memAux->numero);
				pthread_mutex_unlock(&semMMemoriasEC);
			} else {
				pthread_mutex_unlock(&semMMemoriasEC);
				log_error(logger_KERNEL, "No se puede resolver el request porque no hay memorias asociadas al criterio EC");
			}
			break;
		default:
			log_error(logger_KERNEL, "No se puede resolver el request porque no tengo la metadata de la tabla");
			break;
	}
	return memoriaCorrespondiente;
}

/* obtenerIndiceHash()
 * Parametros:
 * 	-> int :: key
 * 	-> int :: maximo
 * Descripcion: devuelve un numero en base a la key. Si la key es 0 devuelve cualquier número.
 * El valor de retorno siempre es entre 0 y maximo
 * Return:
 * 	-> numero :: unsigned int  */
unsigned int obtenerIndiceHash(int key, int maximo) {
	if (key == 0) {
		// https://github.com/sisoputnfrba/foro/issues/1326
		return obtenerIndiceRandom(maximo);
	} else {
		unsigned int valorHash = 0;
		unsigned int j = (key + 3) % 5 + maximo;
		//for (int i = 0; i < j; i++) {
		//	valorHash += (key << j);
		//}
		return (valorHash + j) % maximo;
	}
}

/* obtenerIndiceRandom()
 * Parametros:
 * 	-> int :: maximo
 * Descripcion: devuelve un numero random entre 0 y el max y
 * usa como seed el DAIU_NUMBER de la config.
 * Return:
 * 	-> numero :: unsigned int  */
unsigned int obtenerIndiceRandom(int maximo) {
	unsigned int numero = 0;
	numero = rand() % maximo;
	return numero;
}

void eliminarMemoria(char* puerto, char* ip, char* numero) {
	// la busco en mi lista de memorias y en mi listas de criterios
	int esMemoriaAEliminar(config_memoria* memoriaEnLista) {
		return string_equals_ignore_case(memoriaEnLista->ip, ip) &&
				string_equals_ignore_case(memoriaEnLista->puerto, puerto) &&
				string_equals_ignore_case(memoriaEnLista->numero, numero);
	}
	pthread_mutex_lock(&semMMemorias);
	list_remove_and_destroy_by_condition(memorias,(void*)esMemoriaAEliminar, (void*)liberarConfigMemoria);
	pthread_mutex_unlock(&semMMemorias);
	pthread_mutex_lock(&semMMemoriaSC);
	if (memoriaSc != NULL && string_equals_ignore_case(memoriaSc->ip, ip) &&
				string_equals_ignore_case(memoriaSc->puerto, puerto) &&
				string_equals_ignore_case(memoriaSc->numero, numero)) {
		liberarConfigMemoria(memoriaSc);
		memoriaSc = NULL;
	}
	pthread_mutex_unlock(&semMMemoriaSC);
	pthread_mutex_lock(&semMMemoriasSHC);
	list_remove_and_destroy_by_condition(memoriasShc,(void*)esMemoriaAEliminar, (void*)liberarConfigMemoria);
	pthread_mutex_unlock(&semMMemoriasSHC);
	pthread_mutex_lock(&semMMemoriasEC);
	list_remove_and_destroy_by_condition(memoriasEc,(void*)esMemoriaAEliminar, (void*)liberarConfigMemoria);
	pthread_mutex_unlock(&semMMemoriasEC);
}

int conectarseAMemoria(rol tipoRol, char* puerto, char* ip, char* numero) {
	int conexionTemporanea = crearConexion(ip, puerto);
	if (conexionTemporanea == COMPONENTE_CAIDO) {
		// eliminar memoria de lista de memorias y de criterios
		eliminarMemoria(puerto, ip, numero);
		return FAILURE;
	}
	int rta = enviarHandshakeMemoria(tipoRol, KERNEL, conexionTemporanea);
	if (rta == COMPONENTE_CAIDO) {
		// eliminar memoria de lista de memorias y de criterios
		eliminarMemoria(puerto, ip, numero);
		liberar_conexion(conexionTemporanea);
		return FAILURE;
	}
	return conexionTemporanea;
}

int reintentarConexion(consistencia tipoConsistencia, int key, int memoriaRandom, char** numMemoria) {
	char* ip;
	char* puerto;
	char* numAux;
	int conexionTemporanea;
	while(1) {
		// si la respuesta es distinto de componente caido hago return
		config_memoria* memoriaCorrespondiente;
		pthread_mutex_lock(&semMMemorias);
		if (list_size(memorias) == 0) {
			pthread_mutex_unlock(&semMMemorias);
			log_error(logger_KERNEL, "No hay memorias levantadas y no puedo realizar el request");
			return FAILURE;
		}
		pthread_mutex_unlock(&semMMemorias);
		if (memoriaRandom) {
			pthread_mutex_lock(&semMMemorias);
			if (list_size(memorias) == 0) {
				pthread_mutex_unlock(&semMMemorias);
				log_error(logger_KERNEL, "No hay memorias levantadas y no puedo realizar el request");
				return FAILURE;
			}
			unsigned int indice = obtenerIndiceRandom(list_size(memorias));
			memoriaCorrespondiente = list_get(memorias, indice);
			*numMemoria = strdup(memoriaCorrespondiente->numero);
			ip = strdup(memoriaCorrespondiente->ip);
			puerto = strdup(memoriaCorrespondiente->puerto);
			numAux = strdup(memoriaCorrespondiente->numero);
			pthread_mutex_unlock(&semMMemorias);
			conexionTemporanea = conectarseAMemoria(REQUEST, puerto, ip, numAux);
			free(numAux);
			free(ip);
			free(puerto);
			ip = NULL;
			puerto = NULL;
			numAux = NULL;

		} else {
			memoriaCorrespondiente = encontrarMemoriaSegunConsistencia(tipoConsistencia, key);
			if(memoriaCorrespondiente == NULL) {
				return FAILURE;
			} else {
				conexionTemporanea = conectarseAMemoria(REQUEST, memoriaCorrespondiente->puerto, memoriaCorrespondiente->ip, memoriaCorrespondiente->numero);
				*numMemoria = strdup(memoriaCorrespondiente->numero);
				liberarConfigMemoria(memoriaCorrespondiente);
			}
		}
		if(conexionTemporanea != FAILURE) {
			return conexionTemporanea;
		}
		free(*numMemoria);
		*numMemoria = NULL;
	}
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
	pthread_mutex_lock(&semMMemorias);
	if (list_size(memorias) == 0) {
		pthread_mutex_unlock(&semMMemorias);
		log_error(logger_KERNEL, "No puedo hacer %s porque no hay memorias levantadas", mensaje);
		return FAILURE;
	}
	pthread_mutex_unlock(&semMMemorias);
	time_t tiempo;
	if (codigo == SELECT || codigo == INSERT) {
		tiempo = time(NULL);
	}
	t_paquete* paqueteRecibido;
	int respuesta;
	consistencia consistenciaTabla = NINGUNA;
	int conexionTemporanea;
	char** parametros = separarRequest(mensaje);
	config_memoria* memoriaCorrespondiente;
	char* numMemoria;
	char* puerto;
	char* ip;
	int key = 0;
	if (codigo == DESCRIBE) {
		// https://github.com/sisoputnfrba/foro/issues/1391 chequearlo
		pthread_mutex_lock(&semMMemorias);
		int tamanioMem = list_size(memorias);
		if (list_size(memorias) == 0) {
			pthread_mutex_unlock(&semMMemorias);
			liberarArrayDeChar(parametros);
			log_error(logger_KERNEL, "No puedo hacer %s porque no hay memorias levantadas", mensaje);
			return FAILURE;
		}
		unsigned int indice = obtenerIndiceRandom(tamanioMem);
		memoriaCorrespondiente = list_get(memorias, indice);
		numMemoria = strdup(memoriaCorrespondiente->numero);
		ip = strdup(memoriaCorrespondiente->ip);
		puerto = strdup(memoriaCorrespondiente->puerto);
		pthread_mutex_unlock(&semMMemorias);
		conexionTemporanea = conectarseAMemoria(REQUEST, puerto, ip, numMemoria);
		if(conexionTemporanea == FAILURE) {
			conexionTemporanea = reintentarConexion(consistenciaTabla, 0, 1, &numMemoria);
			free(ip);
			free(puerto);
			if (conexionTemporanea == FAILURE) {
				// todo: probar reintento de conexiones
				log_info(logger_KERNEL, "La request %s no se pudo ejecutar porque no hay memorias para esa request", mensaje);
				free(numMemoria);
				numMemoria = NULL;
				liberarArrayDeChar(parametros);
				return SUCCESS;
			}
		}
	} else {
		if (codigo == CREATE) {
			consistenciaTabla = obtenerEnumConsistencia(parametros[2]);
		} else {
			consistenciaTabla = obtenerConsistenciaTabla(parametros[1]);
		}
		if (codigo == SELECT || codigo == INSERT) {
			key = (int) parametros[2];
		}
		memoriaCorrespondiente = encontrarMemoriaSegunConsistencia(consistenciaTabla, key);
		if(memoriaCorrespondiente == NULL) {
			respuesta = SUCCESS;
			liberarArrayDeChar(parametros);
			return respuesta;
		} else {
			numMemoria = strdup(memoriaCorrespondiente->numero);
			ip = strdup(memoriaCorrespondiente->ip);
			puerto = strdup(memoriaCorrespondiente->puerto);
			conexionTemporanea = conectarseAMemoria(REQUEST, puerto, ip, numMemoria);
			if(conexionTemporanea == FAILURE) {
				conexionTemporanea = reintentarConexion(consistenciaTabla, 0, 1, &numMemoria);
				free(ip);
				free(puerto);
				ip = NULL;
				puerto = NULL;
				if (conexionTemporanea == FAILURE) {
					log_info(logger_KERNEL, "La request %s no se pudo ejecutar porque no hay memorias para esa request", mensaje);
					free(numMemoria);
					liberarArrayDeChar(parametros);
					numMemoria = NULL;
					return SUCCESS;
				}
			}
			liberarConfigMemoria(memoriaCorrespondiente);
		}
	}
	int respuestaEnviar = enviar(consistenciaTabla, mensaje, conexionTemporanea);
	if (respuestaEnviar == COMPONENTE_CAIDO) {
		// https://github.com/sisoputnfrba/foro/issues/1433#issuecomment-510293161
		// no se puede hacer request no mandar error
		log_info(logger_KERNEL, "La request %s no se pudo ejecutar porque se cayó la memoria", mensaje);
		liberarArrayDeChar(parametros);
		free(ip);
		free(puerto);
		free(numMemoria);
		ip = NULL;
		puerto = NULL;
		return SUCCESS;
	} else {
		paqueteRecibido = recibir(conexionTemporanea);
	}
	respuesta = paqueteRecibido->palabraReservada;
	if (respuesta == SUCCESS) {
		if (codigo == DESCRIBE) {
			actualizarTablas(paqueteRecibido->request);
		}
		if(codigo == SELECT || codigo == DESCRIBE) {
			log_info(logger_KERNEL, "El request %s se ejecutó y me llegó como rta %s", mensaje, paqueteRecibido->request);
		} else {
			log_info(logger_KERNEL, "El request %s se realizó con éxito", mensaje);
		}
		if (codigo == CREATE) {
			char* consistenciaTablaString = strdup("");
			string_append_with_format(&consistenciaTablaString, "%s %s;", parametros[1], parametros[2]);
			actualizarTablas(consistenciaTablaString);
			free(consistenciaTablaString);
		}
	} else if (respuesta == MEMORIA_FULL) {
		log_info(logger_KERNEL, "La memoria está FULL, forzando JOURNAL...");
		char* req = strdup("JOURNAL");
		enviarHandshakeMemoria(REQUEST, KERNEL, conexionTemporanea);
		respuestaEnviar = enviar(NINGUNA, req, conexionTemporanea);
		eliminar_paquete(paqueteRecibido);
		free(req);
		paqueteRecibido = recibir(conexionTemporanea);
		respuesta = paqueteRecibido->palabraReservada;
		if (respuesta == SUCCESS) {
			log_info(logger_KERNEL, "JOURNAL exitoso");
			enviarHandshakeMemoria(REQUEST, KERNEL, conexionTemporanea);
			enviar(consistenciaTabla, mensaje, conexionTemporanea);
			eliminar_paquete(paqueteRecibido);
			paqueteRecibido = recibir(conexionTemporanea);
			respuesta = paqueteRecibido->palabraReservada;
			if (respuesta == SUCCESS) {
				if(codigo == SELECT || codigo == DESCRIBE) {
					log_error(logger_KERNEL, "El request %s se ejecutó y me llegó como rta %s", mensaje, paqueteRecibido->request);

				} else {
					log_info(logger_KERNEL, "El request %s se realizó con éxito", mensaje);
				}
				if (codigo == CREATE) {
					char* consistenciaTablaString = strdup("");
					string_append_with_format(&consistenciaTablaString, "%s %s;", parametros[1], parametros[2]);
					actualizarTablas(consistenciaTablaString);
					free(consistenciaTablaString);
				}
			} else {
				log_error(logger_KERNEL, "El request %s no es válido y me llegó como rta %s", mensaje, paqueteRecibido->request);
			}
		} else {
			log_info(logger_KERNEL, "El JOURNAL falló");
			//log_error(logger_KERNEL, "El request %s no es válido y me llegó como rta %s", mensaje, paqueteRecibido->request);
		}
	} else if (respuesta == COMPONENTE_CAIDO) {
		// https://github.com/sisoputnfrba/foro/issues/1433#issuecomment-510293161
		// no mandar error
		log_info(logger_KERNEL, "La request %s no se pudo ejecutar porque se cayó la memoria", mensaje);
		respuesta = SUCCESS;
	} else if(respuesta == KEY_NO_EXISTE && codigo == SELECT) {
		respuesta = SUCCESS;
		log_info(logger_KERNEL, "El request %s se ejecutó y me llegó como rta %s", mensaje, paqueteRecibido->request);
	} else if(respuesta == TABLA_EXISTE && codigo == CREATE) {
		respuesta = SUCCESS;
		log_info(logger_KERNEL, "El request %s se ejecutó y me llegó como rta la tabla ya existía", mensaje);
	} else if(respuesta == JOURNALTIME) {
		respuesta = SUCCESS;
		log_info(logger_KERNEL, "La memoria %s dice: %s", numMemoria, paqueteRecibido->request);
	} else {
		log_error(logger_KERNEL, "El request %s no es válido y me llegó como rta %s", mensaje, paqueteRecibido->request);
	}
	liberar_conexion(conexionTemporanea);
	eliminar_paquete(paqueteRecibido);
	liberarArrayDeChar(parametros);
	if (codigo == SELECT || codigo == INSERT) {
		tiempo = time(NULL) - tiempo;
		aumentarContadores(numMemoria, codigo, tiempo, consistenciaTabla);
	}
	log_debug(logger_KERNEL, "Le mande a la mem %s el request %s", numMemoria, mensaje);
	free(numMemoria);
	free(ip);
	free(puerto);
	numMemoria = NULL;
	ip = NULL;
	puerto = NULL;
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
	// todo: sumar a metrica
	// ahora recorro la lista filtrada y creo las conexiones para mandar journal
	// si la memoria es la ppal que ya estoy conectada, no me tengo que conectar
	void enviarJournal(config_memoria* memoriaAConectarse) {
		int	conexionTemporanea = crearConexion(memoriaAConectarse->ip, memoriaAConectarse->puerto);
		if (conexionTemporanea == COMPONENTE_CAIDO) {
			log_info(logger_KERNEL, "La memoria con ip %s, puerto %s y num %s se cayó", memoriaAConectarse->ip, memoriaAConectarse->puerto, memoriaAConectarse->numero);
			eliminarMemoria(memoriaAConectarse->puerto, memoriaAConectarse->ip, memoriaAConectarse->numero);
			return;
		}
		int resul1 = enviarHandshakeMemoria(REQUEST, KERNEL, conexionTemporanea);
		if (resul1 == COMPONENTE_CAIDO) {
			log_info(logger_KERNEL, "La memoria con ip %s, puerto %s y num %s se cayó", memoriaAConectarse->ip, memoriaAConectarse->puerto, memoriaAConectarse->numero);
			liberar_conexion(conexionTemporanea);
			eliminarMemoria(memoriaAConectarse->puerto, memoriaAConectarse->ip, memoriaAConectarse->numero);
			return;
		}
		int resul2 = enviar(NINGUNA, "JOURNAL", conexionTemporanea);
		if (resul2 == COMPONENTE_CAIDO) {
			log_info(logger_KERNEL, "La memoria con ip %s, puerto %s y num %s se cayó", memoriaAConectarse->ip, memoriaAConectarse->puerto, memoriaAConectarse->numero);
			liberar_conexion(conexionTemporanea);
			eliminarMemoria(memoriaAConectarse->puerto, memoriaAConectarse->ip, memoriaAConectarse->numero);
			return;
		}
		log_info(logger_KERNEL, "Se envió el JOURNAL a la memoria con numero %s", memoriaAConectarse->numero);
		t_paquete* paqueteRecibido = recibir(conexionTemporanea);
		int respuesta = paqueteRecibido->palabraReservada;
		if (respuesta == SUCCESS) {
			log_info(logger_KERNEL, "La respuesta del request JOURNAL es %s", paqueteRecibido->request);
		} else if(respuesta == COMPONENTE_CAIDO) {
			log_info(logger_KERNEL, "La memoria con ip %s, puerto %s y num %s se cayó", memoriaAConectarse->ip, memoriaAConectarse->puerto, memoriaAConectarse->numero);
			eliminarMemoria(memoriaAConectarse->puerto, memoriaAConectarse->ip, memoriaAConectarse->numero);
		} else {
			log_error(logger_KERNEL, "El request JOURNAL falló");
		}
		liberar_conexion(conexionTemporanea);
		eliminar_paquete(paqueteRecibido);
	}
	void journalSoloAShc(config_memoria* memoriaAConectarse) {
		config_memoria* memAux;
		memAux = malloc(sizeof(config_memoria));
		memAux->ip = strdup(memoriaAConectarse->ip);
		memAux->puerto = strdup(memoriaAConectarse->puerto);
		memAux->numero = strdup(memoriaAConectarse->numero);
		pthread_mutex_unlock(&semMMemoriasSHC);
		enviarJournal(memAux);
		liberarConfigMemoria(memAux);
		pthread_mutex_lock(&semMMemoriasSHC);
	}
	if(soloASHC == TRUE) {
		// paso memorias a lista auxiliar para no bloquear
		pthread_mutex_lock(&semMMemoriasSHC);
		list_iterate(memoriasShc, (void*)journalSoloAShc);
		pthread_mutex_unlock(&semMMemoriasSHC);
	} else {
		t_list* memoriasSinRepetir = list_create();
		void agregarMemoriaSinRepetir(config_memoria* memoria) {
			int existeUnaIgual(config_memoria* memoriaAgregada) {
				return string_equals_ignore_case(memoriaAgregada->ip, memoria->ip) &&
						string_equals_ignore_case(memoriaAgregada->puerto, memoria->puerto) &&
						string_equals_ignore_case(memoriaAgregada->numero, memoria->numero);
			}
			if (!list_any_satisfy(memoriasSinRepetir, (void*)existeUnaIgual)) {
				config_memoria* memAux = malloc(sizeof(config_memoria));
				memAux->ip = strdup(memoria->ip);
				memAux->puerto = strdup(memoria->puerto);
				memAux->numero = strdup(memoria->numero);
				list_add(memoriasSinRepetir, memAux);
			}
		}
		// cuando itero tengo que agregar listas a una lista aux porque sino se frena todo
		// me guardo una lista con puerto e ip sii no existe en la lista
		// porque una memoria puede tener + de 1 criterio
		pthread_mutex_lock(&semMMemoriaSC);
		if(memoriaSc != NULL) {
			config_memoria* memAux = malloc(sizeof(config_memoria));
			memAux->ip = strdup(memoriaSc->ip);
			memAux->puerto = strdup(memoriaSc->puerto);
			memAux->numero = strdup(memoriaSc->numero);
			pthread_mutex_unlock(&semMMemoriaSC);
			list_add(memoriasSinRepetir, memAux);
		} else {
			pthread_mutex_unlock(&semMMemoriaSC);
		}
		pthread_mutex_lock(&semMMemoriasSHC);
		list_iterate(memoriasShc,(void*)agregarMemoriaSinRepetir);
		pthread_mutex_unlock(&semMMemoriasSHC);
		pthread_mutex_lock(&semMMemoriasEC);
		list_iterate(memoriasEc,(void*)agregarMemoriaSinRepetir);
		pthread_mutex_unlock(&semMMemoriasEC);

		list_iterate(memoriasSinRepetir, (void*)enviarJournal);
		list_destroy_and_destroy_elements(memoriasSinRepetir, (void*)liberarConfigMemoria);
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
	int sleep = 0;
	//el while es fin de Q o fin de cola
	pthread_mutex_lock(&semMQuantum);
	int quantumPlanificacion = quantum;
	pthread_mutex_unlock(&semMQuantum);
	while(!queue_is_empty(colaRun) && quantumActual < quantumPlanificacion) {
		request = (request_procesada*) malloc(sizeof(request_procesada));
		request->codigo = ((request_procesada*) queue_peek(colaRun))->codigo;
		request->request = strdup((char*)((request_procesada*)queue_peek(colaRun))->request);

		if (validarRequest((char*) request->request) == TRUE) {
			if (manejarRequest(request, TRUE) != SUCCESS) {
				break;
				//libero recursos, mato hilo, lo saco de la cola, e informo error
			}
		} else {
			break;
			//libero recursos, mato hilo, lo saco de la cola, e informo error
		}
		free((char*)((request_procesada*)queue_peek(colaRun))->request);
		free(queue_pop(colaRun));
		liberarRequestProcesada(request);
		quantumActual++;
		pthread_mutex_lock(&semMSleepEjecucion);
		sleep = sleepEjecucion;
		pthread_mutex_unlock(&semMSleepEjecucion);
		usleep(sleep*1000);
	}
	if (quantumActual == quantumPlanificacion && queue_is_empty(colaRun) == FALSE) {
		//termino por fin de q
		log_info(logger_KERNEL, "Vuelvo a ready");
		request_procesada* _request = (request_procesada*)(malloc(sizeof(request_procesada)));
		_request->request = colaRun;
		_request->codigo = RUN;
		pthread_mutex_lock(&semMColaReady);
		queue_push(ready, _request);
		pthread_mutex_unlock(&semMColaReady);
		sem_post(&semRequestReady);
	} else if (queue_is_empty(colaRun) == TRUE){
		// si estoy aca es porque ya ejecuto toda la cola
		log_info(logger_KERNEL, "...........................................................");
		log_info(logger_KERNEL, "Finalizó la ejecución del script");
		log_info(logger_KERNEL, "...........................................................");
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
	int estado = validarRequest(mensaje);
	if (estado != TRUE) {
		return estado;
	} else {
		estado = SUCCESS;
	}
	char** requestDividida = separarRequest(mensaje);
	config_memoria* memoria;
	consistencia _consistencia;
	_consistencia = obtenerEnumConsistencia(requestDividida[4]);
	if (_consistencia == CONSISTENCIA_INVALIDA) {
		estado = FAILURE;
		log_error(logger_KERNEL, "El criterio %s no es válido", requestDividida[4]);
	}
	int esMemoriaCorrecta(config_memoria* memoriaActual) {
		return string_equals_ignore_case(memoriaActual->numero, requestDividida[2]);
	}
	pthread_mutex_lock(&semMMemorias);
	memoria = (config_memoria*) list_find(memorias, (void*)esMemoriaCorrecta);
	if (memoria == NULL) {
		pthread_mutex_unlock(&semMMemorias);
		estado = FAILURE;
		log_error(logger_KERNEL, "No encontré la memoria %s", requestDividida[2]);
	} else {
		config_memoria* memAux = (config_memoria*) malloc(sizeof(config_memoria));
		memAux->ip = strdup(memoria->ip);
		memAux->puerto = strdup(memoria->puerto);
		memAux->numero = strdup(memoria->numero);
		pthread_mutex_unlock(&semMMemorias);
		int existeUnaIgual(config_memoria* memoriaEnLista) {
			return string_equals_ignore_case(memoriaEnLista->ip, memAux->ip) &&
					string_equals_ignore_case(memoriaEnLista->puerto, memAux->puerto) &&
					string_equals_ignore_case(memoriaEnLista->numero, memAux->numero);
		}
		switch (_consistencia) {
			case SC:
				pthread_mutex_lock(&semMMemoriaSC);
				if (memoriaSc != NULL) {
					liberarConfigMemoria(memoriaSc);
					memoriaSc = NULL;
				}
				memoriaSc = memAux;
				pthread_mutex_unlock(&semMMemoriaSC);
				log_info(logger_KERNEL, "Se agregó la memoria %s al criterio SC", memAux->numero);
				break;
			case SHC:
				pthread_mutex_lock(&semMMemoriasSHC);
				if (!list_any_satisfy(memoriasShc, (void*)existeUnaIgual)) {
					list_add(memoriasShc, memAux);
					pthread_mutex_unlock(&semMMemoriasSHC);
					log_info(logger_KERNEL, "Se agregó la memoria %s al criterio SHC", memAux->numero);
					procesarJournal(TRUE);
				} else {
					pthread_mutex_unlock(&semMMemoriasSHC);
					log_info(logger_KERNEL, "La memoria %s ya existe en la lista de memorias SHC", memAux->numero);
					liberarConfigMemoria(memAux);
				}
				break;
			case EC:
				pthread_mutex_lock(&semMMemoriasEC);
				if (!list_any_satisfy(memoriasEc, (void*)existeUnaIgual)) {
					list_add(memoriasEc, memAux);
					pthread_mutex_unlock(&semMMemoriasEC);
					log_info(logger_KERNEL, "Se agregó la memoria %s al criterio EC", memAux->numero);
				} else {
					pthread_mutex_unlock(&semMMemoriasEC);
					log_info(logger_KERNEL, "La memoria %s ya existe en la lista de memorias EC", memAux->numero);
					liberarConfigMemoria(memAux);
				}
				break;
			default:
				liberarConfigMemoria(memAux);
				log_info(logger_KERNEL, "Ocurrió un error al agregar la memoria al criterio");
				break;
		}
	}
	liberarArrayDeChar(requestDividida);
	return estado;
}
