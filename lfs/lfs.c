#include "lfs.h"
#include <errno.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>

int main(void) {
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/lfs/lfs.config");
	logger_LFS = log_create("lfs.log", "Lfs", 1, LOG_LEVEL_DEBUG);
	log_info(logger_LFS, "----------------INICIO DE LISSANDRA FS--------------");

	inicializarLfs();

	if(!pthread_create(&hiloLeerDeConsola, NULL, leerDeConsola, NULL)){
		log_info(logger_LFS, "Hilo de consola creado");
	}else{
		log_error(logger_LFS, "Error al crear hilo de consola");
	}

	if(!pthread_create(&hiloRecibirMemorias, NULL, recibirMemorias, NULL)){
		log_info(logger_LFS, "Hilo de recibir memorias creado");
	}else{
		log_error(logger_LFS, "Error al crear hilo recibir memorias");
	}

	if(!pthread_create(&hiloDumpeo, NULL, hiloDump, NULL)){
		log_info(logger_LFS, "Hilo de Dump iniciado");
		pthread_detach(hiloDumpeo);
	}else{
		log_error(logger_LFS, "Error al crear hilo Dumpeo");
	}

	pthread_join(hiloLeerDeConsola, NULL);
	log_info(logger_LFS, "Hilo de consola finalizado");
	pthread_join(hiloRecibirMemorias, NULL);
	log_info(logger_LFS, "Hilo recibir memorias finalizado");

	free(pathRaiz);
	log_destroy(logger_LFS);
	config_destroy(config);
	return EXIT_SUCCESS;
}


void* leerDeConsola(void* arg) {
	while (1) {
		log_info(logger_LFS, "Mete algo vieja");
		mensaje = readline(">");
		if (!(strncmp(mensaje, "", 1) != 0)) {
			free(mensaje);
			break;
		}
		if(!validarMensaje(mensaje, LFS, logger_LFS)){
			char** request = string_n_split(mensaje, 2, " ");
			cod_request palabraReservada = obtenerCodigoPalabraReservada(request[0], LFS);
			interpretarRequest(palabraReservada, mensaje, NULL);
			for(int i = 0; request[i] != NULL; i++){
				free(request[i]);
			}
			free(request);
		}else{
			log_error(logger_LFS, "Request invalida");
		}
		free(mensaje);
	}
	return NULL;
}

void* recibirMemorias(void* arg) {
	char* puerto = config_get_string_value(config, "PUERTO");
	char* ip = config_get_string_value(config, "IP");
	int lissandraFS_fd = iniciar_servidor(puerto, ip);
	log_info(logger_LFS, "Lissandra lista para recibir Memorias");
	free(puerto);
	free(ip);

	pthread_t hiloRequest;

	while (1) {
		int memoria_fd = esperar_cliente(lissandraFS_fd);
		if(memoria_fd > 0) {

			if(!pthread_create(&hiloRequest, NULL, (void*) conectarConMemoria, (void*) memoria_fd)) {
				char* mensaje = string_from_format("Se conecto la memoria %d", memoria_fd);
				log_info(logger_LFS, mensaje);
				pthread_detach(hiloRequest);
				free(mensaje);
			} else {
				char* error = string_from_format("Error al iniciar el hilo de la memoria %d", memoria_fd);
				log_error(logger_LFS, error);
				free(error);
			}
		}
	}
	return NULL;
}

void* conectarConMemoria(void* arg) {
	int memoria_fd = (int)arg;
	while (1) {
		t_paquete* paqueteRecibido = recibir(memoria_fd);
		cod_request palabraReservada = paqueteRecibido->palabraReservada;
		printf("De la memoria nro: %d \n", memoria_fd);
		//TODO ver de interpretar si es -1 q onda
		interpretarRequest(palabraReservada, paqueteRecibido->request, memoria_fd);
		if (palabraReservada == -1) break;
		enviar(palabraReservada, paqueteRecibido->request, memoria_fd);
	}
	return NULL;
}

void interpretarRequest(cod_request palabraReservada, char* request, int memoria_fd) {
	char** requestSeparada = separarRequest(request);
	errorNo retorno = SUCCESS;
	switch (palabraReservada){
		case SELECT:
			log_info(logger_LFS, "Me llego un SELECT");
			retorno = procesarSelect(requestSeparada[1], requestSeparada[2]);
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
			//TODO validar los tipos de los parametros (ejemplo, SC, cantidad de particiones, etc.)
			retorno = procesarCreate(requestSeparada[1], requestSeparada[2], requestSeparada[3], requestSeparada[4]);
			break;
		case DESCRIBE:
			log_info(logger_LFS, "Me llego un DESCRIBE");
			break;
		case DROP:
			log_info(logger_LFS, "Me llego un DROP");
			break;
		case NUESTRO_ERROR:
			if (memoria_fd != NULL) {
				log_error(logger_LFS, "El cliente se desconecto");
			}
			break;
	}

	char* mensajeDeError;
	switch(retorno){
		case SUCCESS:
			mensajeDeError = string_from_format("Request recibida correctamente");
			//mensajeDeError = string_from_format("La tabla %s fue creada correctamente", requestSeparada[1]);
			log_info(logger_LFS, mensajeDeError);
			break;
		case TABLA_EXISTE:
			mensajeDeError = string_from_format("La tabla %s ya existe", requestSeparada[1]);
			log_error(logger_LFS, mensajeDeError);
			break;
		case ERROR_CREANDO_ARCHIVO:
			mensajeDeError = string_from_format("Error al crear un archivo de la tabla %s", requestSeparada[1]);
			log_error(logger_LFS, mensajeDeError);
			break;
		case TABLA_NO_EXISTE:
			mensajeDeError = string_from_format("La tabla %s no existe", requestSeparada[1]);
			log_info(logger_LFS, mensajeDeError);
	}

	free(mensajeDeError);

	if (memoria_fd != NULL) {
		char* retorno_string = string_itoa(retorno);
		enviar(palabraReservada, retorno_string, memoria_fd);
		free(retorno_string);
	}

	for (int i = 0; requestSeparada[i] != NULL; i++) {
		free(requestSeparada[i]);
	}
	free(requestSeparada);
	puts("\n");
}

/* procesarCreate() [API]
 * Parametros:
 * 	-> nombreTabla :: char*
 * 	-> tipoDeConsistencia :: char*
 * 	-> numeroDeParticiones :: int
 * 	-> tiempoDeCompactacion :: int
 * Descripcion: permite la creación de una nueva tabla dentro del file system
 * Return: codigos de error o success*/
errorNo procesarCreate(char* nombreTabla, char* tipoDeConsistencia,	char* numeroDeParticiones, char* tiempoDeCompactacion) {

	char* pathTabla = string_from_format("%sTablas/%s", pathRaiz, nombreTabla);
	errorNo error = SUCCESS;
	//TODO PASAR NOMBRE DE TABLA A MAYUSCULA

	/* Validamos si la tabla existe */
	DIR *dir = opendir(pathTabla);
	if (dir) {
		error = TABLA_EXISTE;
	} else {
		/* Creamos la carpeta de la tabla */
		int resultadoCreacionDirectorio = mkdir(pathTabla, S_IRWXU);
		if (resultadoCreacionDirectorio == -1) {
			error = ERROR_CREANDO_ARCHIVO;
		} else {

			char* metadataPath = string_from_format("%s/Metadata.bin", pathTabla);

			/* Creamos el archivo Metadata */
			int metadataFileDescriptor = open(metadataPath, O_CREAT, S_IRWXU);
			if (metadataFileDescriptor == -1) {
				error = ERROR_CREANDO_ARCHIVO;
			} else {
				t_config *metadataConfig = config_create(metadataPath);
				config_set_value(metadataConfig, "CONSISTENCY",	tipoDeConsistencia);
				config_set_value(metadataConfig, "PARTITIONS", numeroDeParticiones);
				config_set_value(metadataConfig, "COMPACTION_TIME", tiempoDeCompactacion);
				config_save(metadataConfig);
				config_destroy(metadataConfig);
				error = crearParticiones(pathTabla, numeroDeParticiones);
			}


			free(metadataPath);

			close(metadataFileDescriptor);
		//hiloRequest = malloc(sizeof(pthread_t));
		}
	}
	free(dir);
	free(pathTabla);
	return error;
	}

/* crearParticiones()
 * Parametros:
 * -> pathTabla :: char*
 * -> numeroDeParticiones :: char*
 * Descripcion: crea las particiones de una tabla
 * Return: codigo de error o success */
errorNo crearParticiones(char* pathTabla, char* numeroDeParticiones){
	/* Creamos las particiones */
	errorNo errorNo = SUCCESS;

	int _numeroDeParticiones = strtol(numeroDeParticiones, NULL, 10);
	for (int i = 0; i < _numeroDeParticiones; i++) {
		char* pathParticion = string_from_format("%s/%d.bin", pathTabla, i);

		int particionFileDescriptor = open(pathParticion, O_CREAT, S_IRWXU);
		if (particionFileDescriptor == -1) {
			errorNo = ERROR_CREANDO_ARCHIVO;
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
		free(pathParticion);
	}
	return errorNo;
}

int obtenerBloqueDisponible() {
	//TODO retornar un bloque posta
	return 1;
}

/* inicializarLfs() [API]
 * Parametros:
 * Descripcion: crea el punto de montaje y crea los directorios de: tablas, metadata y bloques
 * Return: */
void inicializarLfs() {
	//TODO catchear todos los errores
	char* puntoDeMontaje = config_get_string_value(config, "PUNTO_MONTAJE");
	pathRaiz = string_from_format("%s%s", PATH , puntoDeMontaje);	

	memtable = (t_memtable*) malloc(sizeof(t_memtable));
	//tabla = (t_tabla*) malloc(sizeof(t_tabla));
	memtable->tablas = list_create();
	//tabla->registro = list_create();


	//if error
	mkdir(pathRaiz, S_IRWXU);

	char* pathTablas = string_from_format("%sTablas", pathRaiz);
	char* pathMetadata = string_from_format("%sMetadata", pathRaiz);
	char* pathBloques = string_from_format("%sBloques", pathRaiz);

	mkdir(pathTablas, S_IRWXU);

	mkdir(pathMetadata, S_IRWXU);
	char* fileMetadata = string_from_format("%s/Metadata.bin", pathMetadata);

	int metadataDescriptor = open(fileMetadata, O_CREAT, S_IRWXU);
	close(metadataDescriptor);

	t_config *configMetadata = config_create(fileMetadata);
	config_set_value(configMetadata, "BLOCK_SIZE", "64");
	config_set_value(configMetadata, "BLOCKS", "10");
	config_set_value(configMetadata, "MAGIC_NUMBER", "LISSANDRA");
	config_save(configMetadata);

	mkdir(pathBloques, S_IRWXU);

	int blocks = config_get_int_value(configMetadata, "BLOCKS");
	char* fileBloque;
	for (int i = 1; i <= blocks; i++) {
		fileBloque = string_from_format("%s/%d.bin", pathBloques, i);
		int bloqueFileDescriptor = open(fileBloque, O_CREAT, S_IRWXU);
		close(bloqueFileDescriptor);
		free(fileBloque);
	}

	config_destroy(configMetadata);

	free(fileMetadata);
	free(pathTablas);
	free(pathMetadata);
	free(pathBloques);
}

/* procesarInsert() [API] [VALGRINDEADO]
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
	char* pathTabla = string_from_format("%s/Tablas/%s", pathRaiz, nombreTabla);
	errorNo error = SUCCESS;


	/* Validamos si la tabla existe */
	DIR *dir = opendir(pathTabla);
	if (dir) {
		t_registro* registro = (t_registro*) malloc(sizeof(t_registro));
		registro->key = key;
		registro->value = strdup(value);
		registro->timestamp = timestamp;
		t_tabla* tabla = list_find(memtable->tablas, (void*) encontrarTabla);
		if (tabla == NULL) {
			log_info(logger_LFS, "Se agrego la tabla a la memtable y se agrego el registro");
			tabla = (t_tabla*) malloc(sizeof(t_tabla));
			tabla->nombreTabla = strdup(nombreTabla);
			tabla->registros = list_create();
			list_add(memtable->tablas, tabla);
		}
		list_add(tabla->registros, registro);
	} else {
		error = TABLA_NO_EXISTE;
	}

	free(pathTabla);
	free(dir);
	return error;
}

errorNo procesarSelect(char* nombreTabla, char* key){
	int encontrarTabla(t_tabla* tabla) {
		return string_equals_ignore_case(tabla->nombreTabla, nombreTabla);
	}

	int encontrarRegistro(t_registro* registro) {
		return registro->key == convertirKey(key);
	}


	char* pathTabla = string_from_format("%s/Tablas/%s", pathRaiz, nombreTabla);
	errorNo error = SUCCESS;

	t_tabla* table = list_find(memtable->tablas, (void*) encontrarTabla);
	if(table !=NULL){
		t_registro* registro = list_find(table->registros, (void*) encontrarRegistro);
		if(registro != NULL){
			log_info(logger_LFS, "el valor encontrado es el de abajo vieja");
			log_info(logger_LFS, registro->value);
		}else{
			log_info(logger_LFS, "no encontre el valor");
		}
	}else{
		log_info(logger_LFS, "no encontre la tabla");
	}
	return error;
}

/* hiloDump()
 * Parametros: void
 * Descripcion: ejecuta la funcion dumpear() cada cierta cantidad de tiempo (tiempoDump) definido en el config
 * Return: void* */
void* hiloDump() {
	int tiempoDump = config_get_int_value(config, "TIEMPO_DUMP");
	while(1) {
		sleep(tiempoDump/1000);
		log_info(logger_LFS, "Dump iniciado");
		errorNo resultado = dumpear();
		switch(resultado) {
			case ERROR_CREANDO_ARCHIVO:
				log_info(logger_LFS, "Error creando archivo temporal");
				break;
			case SUCCESS:
				log_info(logger_LFS, "Dump exitoso");
				break;
			default: break;
		}
	}
}

/* dumpear() [VALGRINDEADO]
 * Parametros: void
 * Descripcion: baja los datos de la memtable a disco
 * Return: codigo de error definido en el enum errorNo */
errorNo dumpear() {
	t_tabla* tabla;
	errorNo error;
	char* pathTmp;

	for(int i = 0; list_get(memtable->tablas,i) != NULL; i++) { // Recorro las tablas de la memtable
		tabla = list_get(memtable->tablas,i);
		char* pathTabla = string_from_format("%sTablas/%s", pathRaiz, tabla->nombreTabla);
		DIR *dir = opendir(pathTabla);
		if(dir) { // Verificamos que exista la tabla (por si hubo un DROP en el medio)
			int fdTmp = 1;
			int numeroTemporal = 0;
			while(fdTmp != -1) { // Nos fijamos que numero de temporal crear
				pathTmp = string_from_format("%s/%d.tmp", pathTabla, numeroTemporal);
				fdTmp = open(pathTmp, O_RDONLY, S_IRWXU);
				numeroTemporal++;
				if(fdTmp != -1) {
					free(pathTmp);
				}
				close(fdTmp);
			}
			free(pathTabla);
			fdTmp = open(pathTmp, O_CREAT | O_RDWR, S_IRWXU);
			free(pathTmp);
			if (fdTmp == -1) {
				error = ERROR_CREANDO_ARCHIVO;
			} else {
				// Guardo lo de la tabla en el archivo temporal
				char* datosADumpear = strdup("");
				for(int j = 0; list_get(tabla->registros,j) != NULL; j++) {
					t_registro* registro = list_get(tabla->registros,j);
					string_append_with_format(&datosADumpear, "%u;%s;%llu\n", registro->key, registro->value, registro->timestamp);
				}
				FILE *fileTmp = fdopen(fdTmp, "w");
				fprintf(fileTmp, "%s", datosADumpear);
				free(datosADumpear);
				fclose(fileTmp);
				close(fdTmp);
				// Vacio la memtable
				list_clean_and_destroy_elements(memtable->tablas, (void*) vaciarTabla);
				error = SUCCESS;
			}
		}
		closedir(dir);
	}
	return error;
}

/* vaciarTabla()
  * Parametros:
 * 	-> tabla :: t_tabla*
 * Descripcion: vacia una tabla y todos sus registros
 * Return: void */
void vaciarTabla(t_tabla *tabla) {
	void eliminarRegistros(t_registro* registro) {
	    free(registro->value);
	    free(registro);
	}
    free(tabla->nombreTabla);
    list_clean_and_destroy_elements(tabla->registros, (void*) eliminarRegistros);
    free(tabla->registros);
    free(tabla);
}
