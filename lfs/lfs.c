#include "lfs.h"
#include <errno.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>

int main(void) {
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/lfs/lfs.config");
	logger_LFS = log_create("lfs.log", "Lfs", 1, LOG_LEVEL_DEBUG);
	log_info(logger_LFS,
			"----------------INICIO DE LISSANDRA FS--------------");

	inicializarLfs();
	//validarRequest("CREATE TABLAxX SC 5 5000");

	//if(pthread_create(&hiloRecibirDeMemoria, NULL, (void*)recibirConexionesMemoria, NULL)){

	//}

	leerDeConsola();

	//pthread_join(hiloRecibirDeMemoria, NULL);

	free(pathRaiz);
	log_destroy(logger_LFS);
	config_destroy(config);
	return 0;
}

void leerDeConsola(void) {
	while (1) {
		mensaje = readline(">");
		if (!(strncmp(mensaje, "", 1) != 0)) {
			free(mensaje);
			break;
		}
		validarRequest(mensaje);
		free(mensaje);
	}
}

void validarRequest(char* mensaje) {
	int codValidacion;
	char** request = string_n_split(mensaje, 2, " ");
	codValidacion = validarMensaje(mensaje, LFS, logger_LFS);
	cod_request palabraReservada = obtenerCodigoPalabraReservada(request[0],
			LFS);
	switch (codValidacion) {
	case EXIT_SUCCESS:
		interpretarRequest(palabraReservada, mensaje, CONSOLE, NULL);
		break;
	case NUESTRO_ERROR:
		//es la q hay q hacerla generica
		log_error(logger_LFS, "La request no es valida");
		break;
	default:
		break;
	}
	for (int i = 0; request[i] != NULL; i++) {
		free(request[i]);
	}
	free(request);
}

void recibirConexionesMemoria() {
	int lissandraFS_fd = iniciar_servidor(
			config_get_string_value(config, "PUERTO"),
			config_get_string_value(config, "IP"));
	log_info(logger_LFS, "Lissandra lista para recibir Memorias");
	pthread_t hiloRequest;

	while (1) {
		int memoria_fd = esperar_cliente(lissandraFS_fd);
		if (pthread_create(&hiloRequest, NULL, (void*) procesarRequest,
				(int*) memoria_fd)) {
			pthread_detach(hiloRequest);
		} else {
			printf("Error al iniciar el hilo de la memoria con fd = %i",
					memoria_fd);
		}
	}
}

void procesarRequest(int memoria_fd) {
	while (1) {
		t_paquete* paqueteRecibido = recibir(memoria_fd);
		cod_request palabraReservada = paqueteRecibido->palabraReservada;
		//printf("El codigo que recibi de Memoria es: %d \n", palabraReservada);
		printf("De la memoria nro: %d \n", memoria_fd);
		interpretarRequest(palabraReservada, paqueteRecibido->request,
				ANOTHER_COMPONENT, memoria_fd);
		//t_paquete* paquete = armar_paquete(palabraReservada, respuesta);
		enviar(palabraReservada, paqueteRecibido->request, memoria_fd);
	}
}

void interpretarRequest(cod_request palabraReservada, char* request, t_caller caller, int memoria_fd) {
	char** requestSeparada = separarRequest(request, " ");
	errorNo retorno;
	switch (palabraReservada) {
	case SELECT:
		log_info(logger_LFS, "Me llego un SELECT");
		break;
	case INSERT:
		log_info(logger_LFS, "Me llego un INSERT");
		unsigned long long timestamp;
		if(longitudDeArrayDeStrings(requestSeparada) == 5) { //4 parametros + INSERT
			retorno = procesarInsert(requestSeparada[1], convertirKey(requestSeparada[2]), requestSeparada[3], convertirTimestamp(requestSeparada[4], &timestamp));
		} else {
			retorno = procesarInsert(requestSeparada[1], convertirKey(requestSeparada[2]), requestSeparada[3], obtenerHoraActual());
		}
		break;
	case CREATE:
		log_info(logger_LFS, "Me llego un CREATE");
		retorno = procesarCreate(requestSeparada[1], requestSeparada[2], strtol(requestSeparada[3], NULL, 10), strtol(requestSeparada[4], NULL, 10));
		char* mensajeError;
		switch (retorno) {
		case SUCCESS:
			log_info(logger_LFS, "");
			break;
		case TABLA_EXISTE:
		case ERROR_CREANDO_DIRECTORIO:
		case ERROR_CREANDO_METADATA:
		case ERROR_CREANDO_PARTICIONES:
			log_error(logger_LFS, "");
			break;
		}
		if (caller == ANOTHER_COMPONENT) {
			char* retorno_string = string_itoa(retorno);
			enviar(palabraReservada, retorno_string, memoria_fd);
			free(retorno_string);
		}
		break;
	case DESCRIBE:
		log_info(logger_LFS, "Me llego un DESCRIBE");
		break;
	case DROP:
		log_info(logger_LFS, "Me llego un DROP");
		break;
	case NUESTRO_ERROR:
		if (caller == ANOTHER_COMPONENT) {
			log_error(logger_LFS, "El cliente se desconecto");
			break;
		} else {
			break;
		}
	default:
		log_warning(logger_LFS,
				"Operacion desconocida. No quieras meter la pata");
		break;
	}

	for (int i = 0; requestSeparada[i] != NULL; i++) {
		free(requestSeparada[i]);
	}
	free(requestSeparada);
	//list_clean_and_destroy_elements(requestSeparada, (void*)liberarString);
	//free(requestSeparada);
	puts("\n");
}

void liberarString(char* y) {
	free(y);
}

/* procesarCreate() [API]
 * Parametros:
 * 	-> nombreTabla :: char*
 * 	-> tipoDeConsistencia :: char*
 * 	-> numeroDeParticiones :: int
 * 	-> tiempoDeCompactacion :: int
 * Descripcion: permite la creación de una nueva tabla dentro del file system
 * Return: codigos de error o success*/
errorNo procesarCreate(char* nombreTabla, char* tipoDeConsistencia,
		int numeroDeParticiones, int tiempoDeCompactacion) {

	char* pathTabla = concatenar(pathRaiz, "Tablas/", nombreTabla, NULL);
	errorNo errorNo = SUCCESS;
	//TODO PASAR NOMBRE DE TABLA A MAYUSCULA

	/* Validamos si la tabla existe */
	DIR *dir = opendir(pathTabla);
	if (dir) {
		errorNo = TABLA_EXISTE;
	} else {
		/* Creamos la carpeta de la tabla */
		int resultadoCreacionDirectorio = mkdir(pathTabla, S_IRWXU);
		if (resultadoCreacionDirectorio == -1) {
			errorNo = ERROR_CREANDO_DIRECTORIO;
		} else {
			char* metadataPath = concatenar(pathTabla, "/Metadata.bin", NULL);

			/* Creamos el archivo Metadata */
			int metadataFileDescriptor = open(metadataPath, O_CREAT, S_IRWXU);
			if (metadataFileDescriptor == -1) {
				errorNo = ERROR_CREANDO_METADATA;
			} else {
				char* _numeroDeParticiones = string_itoa(numeroDeParticiones);
				char* _tiempoDeCompactacion = string_itoa(tiempoDeCompactacion);

				t_config *metadataConfig = config_create(metadataPath);
				config_set_value(metadataConfig, "CONSISTENCY",
						tipoDeConsistencia);
				config_set_value(metadataConfig, "PARTITIONS",
						_numeroDeParticiones);
				config_set_value(metadataConfig, "COMPACTION_TIME",
						_tiempoDeCompactacion);
				config_save(metadataConfig);
				config_destroy(metadataConfig);
				free(_numeroDeParticiones);
				free(_tiempoDeCompactacion);
				errorNo = crearParticiones(numeroDeParticiones, pathTabla);
			}
			free(metadataPath);
			close(metadataFileDescriptor);
		}
	}
	free(dir);
	free(pathTabla);
	return errorNo;
}

errorNo crearParticiones(int numeroDeParticiones, char* pathTabla) {
	/* Creamos las particiones */
	errorNo errorNo = SUCCESS;
	for (int i = 0; i < numeroDeParticiones; i++) {
		char* _i = string_itoa(i);
		char* pathParticion = concatenar(pathTabla, "/", _i, ".bin", NULL);

		int particionFileDescriptor = open(pathParticion, O_CREAT, S_IRWXU);
		if (particionFileDescriptor == -1) {
			errorNo = ERROR_CREANDO_PARTICIONES;
		} else {
			char* bloqueDisponible = string_itoa(obtenerBloqueDisponible());
			t_config *configParticion = config_create(pathParticion);
			config_set_value(configParticion, "SIZE", "0");
			config_set_value(configParticion, "BLOCKS", bloqueDisponible);
			config_save(configParticion);

			free(bloqueDisponible);
			config_destroy(configParticion);
		}
		close(particionFileDescriptor);
		free(_i);
		free(pathParticion);
	}
	return errorNo;
}

int obtenerBloqueDisponible() {
	//TODO retornar un bloque posta
	return 1;
}

void inicializarLfs() {
	memtable = (t_memtable*) malloc(sizeof(t_memtable));
	//tabla = (t_tabla*) malloc(sizeof(t_tabla));
	memtable->tabla = list_create();
	//tabla->registro = list_create();

	pathRaiz = concatenar(PATH,
			config_get_string_value(config, "PUNTO_MONTAJE"), NULL);
	mkdir(pathRaiz, S_IRWXU);

	char* pathTablas = concatenar(pathRaiz, "Tablas", NULL);
	char* pathMetadata = concatenar(pathRaiz, "Metadata", NULL);
	char* pathBloques = concatenar(pathRaiz, "Bloques", NULL);

	mkdir(pathTablas, S_IRWXU);

	mkdir(pathMetadata, S_IRWXU);
	char* fileMetadata = concatenar(pathMetadata, "/Metadata.bin", NULL);

	int metadataDescriptor = open(fileMetadata, O_CREAT, S_IRWXU);
	close(metadataDescriptor);

	t_config *configMetadata = config_create(fileMetadata);
	config_set_value(configMetadata, "BLOCK_SIZE", "64");
	config_set_value(configMetadata, "BLOCKS", "10");
	config_set_value(configMetadata, "MAGIC_NUMBER", "LISSANDRA");
	config_save(configMetadata);

	mkdir(pathBloques, S_IRWXU);

	int blocks = config_get_int_value(configMetadata, "BLOCKS");
	char* blockNumber;
	char* fileBloque;
	for (int i = 1; i <= blocks; i++) {
		blockNumber = string_itoa(i);
		fileBloque = concatenar(pathBloques, "/", blockNumber, ".bin", NULL);
		int bloqueFileDescriptor = open(fileBloque, O_CREAT, S_IRWXU);
		close(bloqueFileDescriptor);

		free(blockNumber);
		free(fileBloque);
	}

	config_destroy(configMetadata);

	free(fileMetadata);
	free(pathTablas);
	free(pathMetadata);
	free(pathBloques);
}

/* procesarInsert() [API]
 * Parametros:
 * 	-> nombreTabla :: char*
 * 	-> key :: uint16_t
 * 	-> value :: char*
 * 	-> timestamp :: unsigned long long
 * Descripcion: permite la creacion y/o actualizacion del valor de una key dentro de una tabla
 * Return:  */
errorNo procesarInsert(char* nombreTabla, uint16_t key, char* value, unsigned long long timestamp) {
	int encontrarTabla(t_tabla* tabla) {
		return string_equals_ignore_case(tabla->nombreTabla, nombreTabla);
	}
	char* pathTabla = string_from_format("%s%s%s", pathRaiz, "Tablas/",
			nombreTabla);
	errorNo errorNo = SUCCESS;

	/* Validamos si la tabla existe */
	DIR *dir = opendir(pathTabla);
	if (dir) {
		t_registro* registro = (t_registro*) malloc(sizeof(t_registro));

		registro->key = key;
		registro->value = strdup(value);
		registro->timestamp = timestamp;
		if (list_find(memtable->tabla, (void*) encontrarTabla) == NULL) {
			puts("No existe");
			t_tabla* tabla = (t_tabla*) malloc(sizeof(t_tabla));
			tabla->nombreTabla = strdup(nombreTabla);
			tabla->registro = list_create();
			list_add(tabla->registro, registro);
			list_add(memtable->tabla, tabla);
		} else {
			puts("Existe la tabla campeon");
		}
	} else {
		errorNo = TABLA_NO_EXISTE;
	}
	free(dir);
	return errorNo;
}


