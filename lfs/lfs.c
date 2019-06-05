#include "lfs.h"
#include <errno.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>


int main(void) {
	configLFS = leer_config("/home/utnso/tp-2019-1c-bugbusters/lfs/lfs.config");
	logger_LFS = log_create("lfs.log", "Lfs", 1, LOG_LEVEL_DEBUG);
	log_info(logger_LFS, "----------------INICIO DE LISSANDRA FS--------------");

	inicializarLfs();

	if(!pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL)){
		log_info(logger_LFS, "Hilo de consola creado");
	}else{
		log_error(logger_LFS, "Error al crear hilo de consola");
	}


	if(!pthread_create(&hiloRecibirMemorias, NULL, recibirMemorias, NULL)){
		log_info(logger_LFS, "Hilo de recibir memorias creado");
	}else{
		log_error(logger_LFS, "Error al crear hilo recibir memorias");
	}


	pthread_join(hiloLeerDeConsola, NULL);
	log_info(logger_LFS, "Hilo de consola finalizado");
	pthread_join(hiloRecibirMemorias, NULL);
	log_info(logger_LFS, "Hilo recibir memorias finalizado");

	free(pathRaiz);
	log_destroy(logger_LFS);
	config_destroy(configMetadata);
	config_destroy(configLFS);
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
	char* puerto = config_get_string_value(configLFS, "PUERTO");
	char* ip = config_get_string_value(configLFS, "IP");
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
		case ERROR_CREANDO_DIRECTORIO:
			mensajeDeError = string_from_format("Error al crear el directorio de la tabla %s", requestSeparada[1]);
			log_error(logger_LFS, mensajeDeError);
			break;
		case ERROR_CREANDO_METADATA:
			mensajeDeError = string_from_format("Error al crear el metadata de la tabla %s", requestSeparada[1]);
			log_error(logger_LFS, mensajeDeError);
			break;
		case ERROR_CREANDO_PARTICIONES:
			mensajeDeError = string_from_format("Error al crear las particiones de la tabla %s", requestSeparada[1]);
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
			error = ERROR_CREANDO_DIRECTORIO;
		} else {

			char* metadataPath = string_from_format("%s/Metatada.bin", pathTabla);

			/* Creamos el archivo Metadata */
			int metadataFileDescriptor = open(metadataPath, O_CREAT, S_IRWXU);
			if (metadataFileDescriptor == -1) {
				error = ERROR_CREANDO_METADATA;
			} else {
				t_config *metadataConfig = config_create(metadataPath);
				config_set_value(metadataConfig, "CONSISTENCY",	tipoDeConsistencia);
				config_set_value(metadataConfig, "PARTITIONS", numeroDeParticiones);
				config_set_value(metadataConfig, "COMPACTION_TIME", tiempoDeCompactacion);
				config_save(metadataConfig);
				config_destroy(metadataConfig);
				error = crearParticiones(pathTabla, strtol(numeroDeParticiones, NULL, 10));
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
errorNo crearParticiones(char* pathTabla, int numeroDeParticiones){
	/* Creamos las particiones */
	errorNo errorNo = SUCCESS;

	for (int i = 0; i < numeroDeParticiones && errorNo == SUCCESS; i++) {
		char* pathParticion = string_from_format("%s/%d.bin", pathTabla, i);

		int particionFileDescriptor = open(pathParticion, O_CREAT, S_IRWXU);
		if (particionFileDescriptor == -1) {
			errorNo = ERROR_CREANDO_PARTICIONES;
		} else {
			int bloqueDeParticion = obtenerBloqueDisponible(&errorNo); //si hay un error se setea en errorNo
			if(bloqueDeParticion == -1){
				log_info(logger_LFS, "no hay bloques disponibles");
			}else{
				char* bloquesParticion = string_from_format("[%d]", bloqueDeParticion);
				t_config *configParticion = config_create(pathParticion);
				config_set_value(configParticion, "SIZE", "0");
				config_set_value(configParticion, "BLOCKS", bloquesParticion);
				config_save(configParticion);
				free(bloquesParticion);
				config_destroy(configParticion);
			}
		}
		close(particionFileDescriptor);
		free(pathParticion);
	}
	return errorNo;
}

int obtenerBloqueDisponible(errorNo* errorNo) {
	//TODO sacar el hardcode de abajo xD
	char* fileBitmap = string_from_format("%s/FS_LISSANDRA/Metadata/Bitmap.bin", PATH);
	int index = 0;
	int fdBitmap = open(fileBitmap, O_CREAT | O_RDWR, S_IRWXU);
	if(fdBitmap == -1){
		*errorNo = ERROR_CREANDO_ARCHIVO;
		index = -1;
	}else{
		//TODO CAMBIAR SIZE en mmap y bitarray_create (segundo parametro de mmap y de bit array). se cambia teniendo las configo globales
		char* bitmap = mmap(NULL, 2, PROT_READ | PROT_WRITE, MAP_SHARED, fdBitmap, 0);
		t_bitarray* bitarray = bitarray_create_with_mode(bitmap, 2, LSB_FIRST);

		//TODO CAMBIAR TODOS LOS CONFIG A GLOBALES
		while(index < config_get_int_value(configMetadata, "BLOCKS") && bitarray_test_bit(bitarray, index)) index++;
		if(index >= config_get_int_value(configMetadata, "BLOCKS")) {
			index = -1;
		}else{
			bitarray_set_bit(bitarray, index);
		}

		bitarray_destroy(bitarray);
	}
	free(fileBitmap);
	close(fdBitmap);
	return index;
}

/* inicializarLfs() [API]
 * Parametros:
 * Descripcion: crea el punto de montaje y crea los directorios de: tablas, metadata y bloques
 * Return: */
void inicializarLfs() {
	//TODO catchear todos los errores
	char* puntoDeMontaje = config_get_string_value(configLFS, "PUNTO_MONTAJE");
	pathRaiz = string_from_format("%s%s", PATH , puntoDeMontaje);	

	memtable = (t_memtable*) malloc(sizeof(t_memtable));
	//tabla = (t_tabla*) malloc(sizeof(t_tabla));
	memtable->tabla = list_create();
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

	configMetadata = config_create(fileMetadata);
	config_set_value(configMetadata, "BLOCK_SIZE", "64");
	config_set_value(configMetadata, "BLOCKS", "16");
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


	char* fileBitmap = string_from_format("%s/Bitmap.bin", pathMetadata);

	int bitmapDescriptor = open(fileBitmap, O_CREAT | O_RDWR | O_TRUNC , S_IRWXU);
	if(ftruncate(bitmapDescriptor, blocks/8)){
		//todo error al truncar archivo
	}
	if(bitmapDescriptor == -1){
		log_error(logger_LFS, "Error al crear bitmap");
	}else{
		char* bitmap = mmap(NULL, blocks/8, PROT_READ | PROT_WRITE, MAP_SHARED, bitmapDescriptor ,0);
		t_bitarray* bitarray = bitarray_create_with_mode(bitmap, blocks/8, LSB_FIRST);
		bitarray_destroy(bitarray);
	}
	close(bitmapDescriptor);

	free(fileMetadata);
	free(pathTablas);
	free(pathMetadata);
	free(pathBloques);
	free(fileBitmap);
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
	char* pathTabla = string_from_format("%s/Tablas/%s", pathRaiz, nombreTabla);
	errorNo error = SUCCESS;


	/* Validamos si la tabla existe */
	DIR *dir = opendir(pathTabla);
	if (dir) {
		t_registro* registro = (t_registro*) malloc(sizeof(t_registro));

		registro->key = key;
		registro->value = strdup(value);
		registro->timestamp = timestamp;
		if (list_find(memtable->tabla, (void*) encontrarTabla) == NULL) {
			log_info(logger_LFS, "Se agrego la tabla a la memtable y se agrego el registro");
			//puts("No existe");
			t_tabla* tabla = (t_tabla*) malloc(sizeof(t_tabla));
			tabla->nombreTabla = strdup(nombreTabla);
			tabla->registro = list_create();
			list_add(tabla->registro, registro);
			list_add(memtable->tabla, tabla);
		} else {
			//puts("Existe la tabla campeon");
			log_info(logger_LFS, "Se agrego el registro");
		}
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

	t_tabla* table = list_find(memtable->tabla, (void*) encontrarTabla);
	if(table !=NULL){
		t_registro* registro = list_find(table->registro, (void*) encontrarRegistro);
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

