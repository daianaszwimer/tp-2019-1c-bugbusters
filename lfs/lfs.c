#include "lfs.h"
#include <errno.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>


/*
 * Parametros del main:
 * 1. Char*: Path del config
 * */
int main(int argc, char* argv[]) {
	iniciarLFS(argv);

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
	}else{
		log_error(logger_LFS, "Error al crear hilo Dumpeo");
	}

	pthread_join(hiloLeerDeConsola, NULL);
	log_info(logger_LFS, "Hilo de consola finalizado");
	pthread_join(hiloRecibirMemorias, NULL);
	log_info(logger_LFS, "Hilo recibir memorias finalizado");
	pthread_join(hiloDumpeo, NULL);
	log_info(logger_LFS, "Hilo dumpeo finalizado");


	liberarMemoriaLFS();
	return EXIT_SUCCESS;
}


void iniciarLFS(char* argv[]){
	logger_LFS = log_create("lfs.log", "Lfs", 1, LOG_LEVEL_DEBUG);
	log_info(logger_LFS, "----------------INICIO DE LISSANDRA FS--------------");

	if(argv[1] != NULL){
		char* configPath = argv[1];
		config = config_create(configPath);
	}else{
		config = config_create("/home/utnso/tp-2019-1c-bugbusters/lfs/lfs.config");
	}

	if(config == NULL){
		log_error(logger_LFS, "Error al leer archivo de configuracion");
		log_info(logger_LFS, "Finalizando Lissandra File System");
		exit(EXIT_FAILURE);
	}
	if(!config_has_property(config, "PUNTO_MONTAJE")){
		log_error(logger_LFS, "Key \"PUNTO_MONTAJE\" no encontrada en config");
		log_info(logger_LFS, "Finalizando Lissandra File System");
		exit(EXIT_FAILURE);
	}
	char* puntoDeMontaje = config_get_string_value(config, "PUNTO_MONTAJE");

	pathRaiz = strdup(puntoDeMontaje);
	pathTablas = string_from_format("%sTablas", pathRaiz);
	pathMetadata = string_from_format("%sMetadata", pathRaiz);
	pathBloques = string_from_format("%sBloques", pathRaiz);
	char* pathBitmap = string_from_format("%s/Bitmap.bin", pathMetadata);
	char* pathFileMetadata = string_from_format("%s/Metadata.bin", pathMetadata);

	DIR* dirPuntoDeMontaje = opendir(pathRaiz);
	if(dirPuntoDeMontaje){
		free(dirPuntoDeMontaje);
		configMetadata = config_create(pathFileMetadata);
		blocks = config_get_int_value(configMetadata, "BLOCKS");
	}else{
		crearFS(pathBitmap, pathFileMetadata);
	}

	free(pathFileMetadata);

	levantarFS(pathBitmap);
	free(pathBitmap);
}

void crearFS(char* pathBitmap, char* pathFileMetadata) {
	//TODO catchear todos los errores
	mkdir(pathRaiz, S_IRWXU);
	mkdir(pathTablas, S_IRWXU);
	mkdir(pathMetadata, S_IRWXU);
	mkdir(pathBloques, S_IRWXU);

	crearFSMetadata(pathBitmap, pathFileMetadata);
	crearBloques();
}

void crearFSMetadata(char* pathBitmap, char* pathFileMetadata){
	FILE* fileMetadata = fopen(pathFileMetadata, "w");
	if(fileMetadata == NULL){
		log_error(logger_LFS, "Error al crear metadata");
		log_info(logger_LFS, "Finalizando Lissandra File System");
		exit(EXIT_FAILURE);
	}
	fclose(fileMetadata);

	configMetadata = config_create(pathFileMetadata);

	char* tamanioDeBloque = readline("Ingresar tamanio de bloque: ");
	char* numeroDeBloques = readline("Ingrese numero de bloques: ");
	char* magicNumber = readline("Ingrese magic number: ");

	config_set_value(configMetadata, "BLOCK_SIZE", tamanioDeBloque);
	config_set_value(configMetadata, "BLOCKS", numeroDeBloques);
	config_set_value(configMetadata, "MAGIC_NUMBER", magicNumber);
	config_save(configMetadata);

	free(tamanioDeBloque);

	free(magicNumber);

	int bitmapDescriptor = open(pathBitmap, O_CREAT | O_RDWR | O_TRUNC , S_IRWXU);
	if(bitmapDescriptor == -1){
		log_error(logger_LFS, "Error al crear bitmap");
		log_info(logger_LFS, "Finalizando Lissandra File System");
		exit(EXIT_FAILURE);
	}

	blocks = atoi(numeroDeBloques);

	if(ftruncate(bitmapDescriptor, atoi(numeroDeBloques)/8)){
		//todo error al truncar archivo
	}

	bitmap = mmap(NULL, blocks/8, PROT_READ | PROT_WRITE, MAP_SHARED, bitmapDescriptor, 0);
	bitarray = bitarray_create_with_mode(bitmap, atoi(numeroDeBloques)/8, LSB_FIRST);
	bitarray_destroy(bitarray);
	munmap(bitmap, blocks/8);
	close(bitmapDescriptor);
	free(numeroDeBloques);
}

void crearBloques(){
	char* fileBloque;
	for (int i = 1; i <= blocks; i++) {
		fileBloque = string_from_format("%s/%d.bin", pathBloques, i);
		FILE* bloqueFile = fopen(fileBloque, "w");
		if(bloqueFile){
			fclose(bloqueFile);
		}
		free(fileBloque);
	}
}

void levantarFS(char* pathBitmap){
	memtable = (t_memtable*) malloc(sizeof(t_memtable));
	memtable->tablas = list_create();

	bitmapDescriptor = open(pathBitmap, O_RDWR);
	bitmap = mmap(NULL, blocks/8, PROT_READ | PROT_WRITE, MAP_SHARED, bitmapDescriptor, 0);
	bitarray = bitarray_create_with_mode(bitmap, blocks/8, LSB_FIRST);
}

void liberarMemoriaLFS(){
	log_info(logger_LFS, "Finalizando LFS");
	free(pathTablas);
	free(pathMetadata);
	free(pathBloques);
	free(pathRaiz);
	list_destroy_and_destroy_elements(memtable->tablas, (void*) vaciarTabla);
	bitarray_destroy(bitarray);
	munmap(bitmap, blocks/8);
	close(bitmapDescriptor);
	free(memtable);
	log_destroy(logger_LFS);

	config_destroy(configMetadata);
	config_destroy(config);
}

void* leerDeConsola(void* arg) {
	while (1) {
		mensaje = readline(">");
		if (!(strncmp(mensaje, "", 1) != 0)) {
			pthread_cancel(hiloRecibirMemorias);
			pthread_cancel(hiloDumpeo);
			free(mensaje);
			break;
		}
		if(!validarMensaje(mensaje, LFS, logger_LFS)){
			char** request = string_n_split(mensaje, 2, " ");
			cod_request palabraReservada = obtenerCodigoPalabraReservada(request[0], LFS);
			interpretarRequest(palabraReservada, mensaje, NULL);
			liberarArrayDeChar(request);

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

	pthread_t hiloRequest;

	while (1) {
		int memoria_fd = esperar_cliente(lissandraFS_fd);
		if(memoria_fd > 0) {
			if(!pthread_create(&hiloRequest, NULL, (void*) conectarConMemoria, (void*) memoria_fd)) {
				char* mensaje = string_from_format("Se conecto la memoria %d", memoria_fd);
				enviarHandshakeLFS(30, memoria_fd);
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
		//si viene -1 es porque se desconecto la memoria
		if (palabraReservada == -1){
			eliminar_paquete(paqueteRecibido);
			log_error(logger_LFS, "el cliente se desconecto. Terminando servidor");
			close(memoria_fd);
			break;
		}
		printf("De la memoria nro: %d \n", memoria_fd);
		interpretarRequest(palabraReservada, paqueteRecibido->request, &memoria_fd);
		eliminar_paquete(paqueteRecibido);
	}
	return NULL;
}

void interpretarRequest(cod_request palabraReservada, char* request, int* memoria_fd) {
	char** requestSeparada = separarRequest(request);
	errorNo errorNo = SUCCESS;
	char* mensaje = strdup("");
	//TODO case memoria se desconecto
	switch (palabraReservada){
		case SELECT:
			log_info(logger_LFS, "Me llego un SELECT");
			errorNo = procesarSelect(requestSeparada[1], requestSeparada[2], &mensaje);
			break;
		case INSERT:
			log_info(logger_LFS, "Me llego un INSERT");
			unsigned long long timestamp;
			if(longitudDeArrayDeStrings(requestSeparada) == 5) { //4 parametros + INSERT
				convertirTimestamp(requestSeparada[4], &timestamp);
				errorNo = procesarInsert(requestSeparada[1], convertirKey(requestSeparada[2]), requestSeparada[3], timestamp);
			} else {
				errorNo = procesarInsert(requestSeparada[1], convertirKey(requestSeparada[2]), requestSeparada[3], obtenerHoraActual());
			}
			break;
		case CREATE:
			log_info(logger_LFS, "Me llego un CREATE");
			//TODO validar los tipos de los parametros (ejemplo, SC, cantidad de particiones, etc.)
			errorNo = procesarCreate(requestSeparada[1], requestSeparada[2], requestSeparada[3], requestSeparada[4]);
			break;
		case DESCRIBE:
			if(longitudDeArrayDeStrings(requestSeparada) == 2){
				errorNo = procesarDescribe(requestSeparada[1], &mensaje);
			}else{
				errorNo = procesarDescribe(NULL, &mensaje);
			}

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
		default:
			break;
	}

	char* mensajeDeError;
	//TODO case memoria se desconecto
	switch(errorNo){
		case SUCCESS:
			mensajeDeError=strdup("Request ejecutada correctamente");
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
			break;
		case KEY_NO_EXISTE:
			mensajeDeError = string_from_format("La KEY %s no existe", requestSeparada[2]); // ODO mostrar bien mensaje de error
			log_info(logger_LFS, mensajeDeError);
			break;
		default:
			break;
	}

	free(mensajeDeError);



	if (memoria_fd != NULL) {
		enviar(errorNo, mensaje, *memoria_fd);
	}else{
		log_info(logger_LFS, mensaje);
	}
	free(mensaje);
	liberarArrayDeChar(requestSeparada);
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
		free(dir);
	} else {
		/* Creamos la carpeta de la tabla */
		int resultadoCreacionDirectorio = mkdir(pathTabla, S_IRWXU);
		if (resultadoCreacionDirectorio == -1) {
			error = ERROR_CREANDO_DIRECTORIO;
		} else {

			char* metadataPath = string_from_format("%s/Metadata.bin", pathTabla);

			/* Creamos el archivo Metadata */
			FILE* metadataFile = fopen(metadataPath, "w");
			if (metadataFile == NULL) {
				error = ERROR_CREANDO_ARCHIVO;
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
			fclose(metadataFile);
		}
	}
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

		FILE* particionFile = fopen(pathParticion, "w");
		if (particionFile == NULL) {
			errorNo = ERROR_CREANDO_ARCHIVO;
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
		fclose(particionFile);
		free(pathParticion);
	}
	return errorNo;
}

int obtenerBloqueDisponible(errorNo* errorNo) {

	int index = 0;
	while (index < blocks && bitarray_test_bit(bitarray, index) == 1) index++;
	if(index >= blocks) {
		index = -1;
	}else{
		bitarray_set_bit(bitarray, index);
	}
	return index;
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
	char* pathTabla = string_from_format("%sTablas/%s", pathRaiz, nombreTabla);
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

//falta buscar en los bloques posta
errorNo procesarSelect(char* nombreTabla, char* key, char** mensaje){

	int ordenarRegistrosPorTimestamp(t_registro* registro1, t_registro* registro2){
		return registro1->timestamp > registro2->timestamp;
	}

	void eliminarRegistro(t_registro* registro) {
		free(registro->value);
		free(registro);
	}

	errorNo error = SUCCESS;
	t_list* listaDeRegistros = list_create();

	char* pathTabla = string_from_format("%s/Tablas/%s", pathRaiz, nombreTabla);
	DIR* dir = opendir(pathTabla);
	free(pathTabla);
	if(dir){
		//TODO validar q onda la funcion convertirKey, retorna -1 si hay error
		int _key = convertirKey(key);
		t_list* listaDeRegistrosDeMemtable = obtenerRegistrosDeMemtable(nombreTabla, _key);
		t_list* listaDeRegistrosDeTmp = obtenerRegistrosDeTmp(nombreTabla, _key);
		list_add_all(listaDeRegistros, listaDeRegistrosDeMemtable);
		list_add_all(listaDeRegistros, listaDeRegistrosDeTmp);
		if(!list_is_empty(listaDeRegistros)){
			list_sort(listaDeRegistros, (void*) ordenarRegistrosPorTimestamp);
			t_registro* registro = (t_registro*)listaDeRegistros->head->data;
			string_append_with_format(&*mensaje, "%s %u %s %llu", nombreTabla, registro->key, registro->value, registro->timestamp);
		}else{
			error = KEY_NO_EXISTE;
		}
		list_destroy(listaDeRegistrosDeMemtable);
		list_destroy_and_destroy_elements(listaDeRegistrosDeTmp, (void*)eliminarRegistro);
	}else{
		error = TABLA_NO_EXISTE;
	}
	list_destroy(listaDeRegistros);
	free(dir);
	return error;
}

t_list* obtenerRegistrosDeTmp(char* nombreTabla, int key){

	FILE* fileTmp;
	int numeroTemporal = 0;
	char* pathTmp;
	t_list* listaDeRegistros = list_create();
	char* datos;
	pathTmp = string_from_format("%sTablas/%s/%d.tmp", pathRaiz, nombreTabla, numeroTemporal);
	fileTmp = fopen(pathTmp, "r");

	while(fileTmp != NULL){
		free(pathTmp);
		//validar si el datos de abajo se libera si el archivo esta vacio
		datos = (char*) malloc(sizeof(uint16_t) + (size_t) config_get_int_value(config, "TAMAÑO_VALUE") + sizeof(unsigned long long));
		strcpy(datos, "");
		while(fscanf(fileTmp, "%s", datos) != EOF){
			char** dato = string_split(datos, ";");
			free(datos);
			int _key = convertirKey(dato[0]);
			if(_key == key){
				t_registro* registro = (t_registro*) malloc(sizeof(t_registro));
				registro->key = key;
				registro->value = strdup(dato[1]);
				registro->timestamp = strtoull(dato[2], NULL, 10);
				list_add(listaDeRegistros, registro);
			}
			liberarArrayDeChar(dato);
			datos = (char*) malloc(sizeof(uint16_t) + (size_t) config_get_int_value(config, "TAMAÑO_VALUE") + sizeof(unsigned long long));
			strcpy(datos, "");
		}
		free(datos);
		fclose(fileTmp);
		numeroTemporal++;
		pathTmp = string_from_format("%sTablas/%s/%d.tmp", pathRaiz, nombreTabla, numeroTemporal);
		fileTmp = fopen(pathTmp, "r");
	}
	free(pathTmp);
	return listaDeRegistros;
}

t_list* obtenerRegistrosDeMemtable(char* nombreTabla, int key){
	int encontrarTabla(t_tabla* tabla) {
		return string_equals_ignore_case(tabla->nombreTabla, nombreTabla);
	}

	int encontrarRegistro(t_registro* registro) {
		return registro->key == key;
	}

	t_list* listaDeRegistros;
	t_tabla* table = list_find(memtable->tablas, (void*) encontrarTabla);
	if(table == NULL){
		listaDeRegistros = list_create();
	}else{
		listaDeRegistros = list_filter(table->registros, (void*) encontrarRegistro);
	}
	return listaDeRegistros;
}

errorNo procesarDescribe(char* nombreTabla, char** mensaje){
	errorNo error = SUCCESS;
	char* pathTablas = string_from_format("%sTablas", pathRaiz);
	char* pathTabla;
	if(nombreTabla != NULL){
		pathTabla = string_from_format("%s/%s", pathTablas, nombreTabla);
		char* metadata = obtenerMetadata(pathTabla);
		string_append_with_format(&*mensaje, "%s %s", nombreTabla, metadata);
		free(metadata);
		free(pathTabla);
	}else{
		DIR *dir;
		struct dirent* tabla;
		if ((dir = opendir(pathTablas)) != NULL) {
			while ((tabla = readdir (dir)) != NULL) {
				struct stat st;

				//ignora . y ..
				if(strcmp(tabla->d_name, ".") == 0 || strcmp(tabla->d_name, "..") == 0) continue;

				//esta funcion carga en st la informacion del file "tabla", que esta dentro de "dir"
				if (fstatat(dirfd(dir), tabla->d_name, &st, 0) < 0) continue;

				if (S_ISDIR(st.st_mode)) {
					pathTabla = string_from_format("%s/%s", pathTablas, tabla->d_name);
					char* metadata = obtenerMetadata(pathTabla);
					string_append_with_format(&*mensaje,"%s %s;", tabla->d_name, metadata);
					free(metadata);
					metadata = NULL;
					free(pathTabla);
				}
			}

			closedir (dir);
			(*mensaje)[strlen(*mensaje) - 1] = 0;
		} else {
			// todo error desconocido, no pudo abrir Tablas

		}
	}
	free(pathTablas);
	return error;
}

char* obtenerMetadata(char* pathTabla){
	char* mensaje;
	DIR* dir = opendir(pathTabla);
	if(dir != NULL){
		closedir(dir);
		char* pathMetadata = string_from_format("%s/%s", pathTabla, "Metadata.bin");
		t_config* metadata = config_create(pathMetadata);
		free(pathMetadata);
		if(metadata != NULL){
			if(config_has_property(metadata, "CONSISTENCY") && config_has_property(metadata, "PARTITIONS") && config_has_property(metadata, "COMPACTION_TIME")){
				mensaje = string_from_format("%s %i %i", config_get_string_value(metadata, "CONSISTENCY"), config_get_int_value(metadata, "PARTITIONS"), config_get_int_value(metadata, "COMPACTION_TIME"));
			}else{
				//todo no posee alguna de las keys
			}
			config_destroy(metadata);
		}else{
			//todo no se pudo levantar la metadata XD
		}
	}else{
		//TODO tabla no existe o error
	}
	return mensaje;
}




/* hiloDump()
 * Parametros: void
 * Descripcion: ejecuta la funcion dumpear() cada cierta cantidad de tiempo (tiempoDump) definido en el config
 * Return: void* */
void* hiloDump(void* args) {
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
 * Return: codigo de error definido en el enum errorNo
 * Parametros: void
 * Descripcion: baja los datos de la memtable a disco
 * Return: codigo de error definido en el enum errorNo */
errorNo dumpear() {
	t_tabla* tabla;
	errorNo error = SUCCESS;
	char* pathTmp;
	FILE* fileTmp;
	char* puntoDeMontaje = config_get_string_value(config, "PUNTO_MONTAJE");
	char* pathMetadata = string_from_format("%sMetadata/Metadata.bin", puntoDeMontaje);
	t_config* configMetadata = config_create(pathMetadata);
	int tamanioBloque = config_get_int_value(configMetadata, "BLOCK_SIZE");

	// Refactor list_iterate
	for(int i = 0; list_get(memtable->tablas,i) != NULL; i++) { // Recorro las tablas de la memtable
		tabla = list_get(memtable->tablas,i);
		char* pathTabla = string_from_format("%sTablas/%s", pathRaiz, tabla->nombreTabla);
		DIR *dir = opendir(pathTabla);
		if(dir) { // Verificamos que exista la tabla (por si hubo un DROP en el medio)
			int numeroTemporal = 0;
			do { // Nos fijamos que numero de temporal crear
				pathTmp = string_from_format("%s/%d.tmp", pathTabla, numeroTemporal);
				fileTmp = fopen(pathTmp, "r");
				numeroTemporal++;
				if(fileTmp != NULL) {
					free(pathTmp);
					fclose(fileTmp);
				}
			} while(fileTmp != NULL);
			free(pathTabla);
			fileTmp = fopen(pathTmp, "a+");
			if (fileTmp == NULL) {
				error = ERROR_CREANDO_ARCHIVO;
			} else {
				// Guardo lo de la tabla en el archivo temporal
				char* datosADumpear = malloc(sizeof(uint16_t) + (size_t) config_get_int_value(config, "TAMAÑO_VALUE") + sizeof(unsigned long long));
				strcpy(datosADumpear, "");
				for(int j = 0; list_get(tabla->registros,j) != NULL; j++) {
					t_registro* registro = list_get(tabla->registros,j);
					string_append_with_format(&datosADumpear, "%u;%s;%llu\n", registro->key, registro->value, registro->timestamp);
				}
				//fprintf(fileTmp, "%s", datosADumpear);
				int cantidadDeBloquesAPedir = strlen(datosADumpear) / tamanioBloque;
				if(strlen(datosADumpear) % tamanioBloque != 0) {
					cantidadDeBloquesAPedir++;
				}
				char* tamanioTmp = string_from_format("SIZE=%d", strlen(datosADumpear));
				char* bloques = strdup("BLOCKS=[");
				for(int i=0; i<cantidadDeBloquesAPedir;i++) {
					int bloqueDeParticion = obtenerBloqueDisponible(&error); //si hay un error se setea en errorNo
					if(bloqueDeParticion == -1){
						log_info(logger_LFS, "no hay bloques disponibles");
					} else {
						if(i==cantidadDeBloquesAPedir-1) {
							bloques = string_from_format("%s%d", bloques, bloqueDeParticion);
						} else {
							bloques = string_from_format("%s%d,", bloques, bloqueDeParticion);
						}
						char* pathBloque = string_from_format("%sBloques/%d.bin", puntoDeMontaje, bloqueDeParticion);
						FILE* bloque = fopen(pathBloque, "a+");
						if(cantidadDeBloquesAPedir != 1 && i < cantidadDeBloquesAPedir - 1) {
							char* registrosAEscribir = string_substring_until(datosADumpear, tamanioBloque);
							datosADumpear = string_substring_from(datosADumpear, tamanioBloque);
							fprintf(bloque, "%s", registrosAEscribir);
						} else {
							fprintf(bloque, "%s", datosADumpear);
						}
						fclose(bloque);
						free(pathBloque);
					}
				}
				bloques = string_from_format("%s]", bloques);
				fprintf(fileTmp, "%s\n%s", tamanioTmp, bloques);
				free(tamanioTmp);
				free(bloques);
				free(datosADumpear);
				fclose(fileTmp);
				// Vacio la memtable
				list_clean_and_destroy_elements(memtable->tablas, (void*) vaciarTabla);
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

void compactacion(char* pathTabla) {
	int numeroTemporal = 0;
	char* puntoDeMontaje = config_get_string_value(config, "PUNTO_MONTAJE");
	char* pathMetadataTabla = string_from_format("%s/Metadata.bin", pathTabla);
	char* pathMetadata = string_from_format("%sMetadata/Metadata.bin", puntoDeMontaje);
	t_config* configMetadata = config_create(pathMetadata);
	t_config* configMetadataTabla = config_create(pathMetadataTabla);
	int tamanioValue = config_get_int_value(config, "TAMAÑO_VALUE");
	int numeroDeParticiones = config_get_int_value(configMetadataTabla, "PARTITIONS");
	FILE* fileTmp;

	DIR *tabla;
	struct dirent *archivoDeLaTabla;

	if((tabla = opendir(pathTabla)) == NULL) {
		perror("opendir");
	}

	while((archivoDeLaTabla = readdir(tabla)) != NULL) {
		if(string_ends_with(archivoDeLaTabla->d_name, ".tmp")) {
			char* pathTmp = string_from_format("%s/%s", pathTabla, archivoDeLaTabla->d_name);
			fileTmp = fopen(pathTmp, "r");
			if (fileTmp != NULL) { // Crear tmpc de los tmp
				char* pathTmpC = string_from_format("%s%c", pathTmp, 'c');
				FILE* fileTmpC = fopen(pathTmpC, "w+");
				char* datosDelArchivoTemporal = malloc((int) (sizeof(uint16_t) + tamanioValue + sizeof(unsigned long long)));
				while (fscanf(fileTmp, "%s", datosDelArchivoTemporal) == 1) {
					fprintf(fileTmpC, "%s", datosDelArchivoTemporal);
					fputc('\n', fileTmpC);
				}
				fclose(fileTmp);
				rewind(fileTmpC);
			}
		}
	}

	if (closedir(tabla) == -1) {
		perror("closedir");
	}
	/*do {
		char* pathTmp = string_from_format("%s/%d.tmp", pathTabla,
				numeroTemporal);
		fileTmp = fopen(pathTmp, "r");
		if (fileTmp != NULL) { // Crear tmpc de los tmp
			char* pathTmpC = string_from_format("%s/%d.tmpc", pathTabla,
					numeroTemporal);
			FILE* fileTmpC = fopen(pathTmpC, "w+");
			char* datosDelArchivoTemporal = malloc(
					(int) (sizeof(uint16_t) + tamanioValue
							+ sizeof(unsigned long long)));
			while (fscanf(fileTmp, "%s", datosDelArchivoTemporal) == 1) {
				fprintf(fileTmpC, "%s", datosDelArchivoTemporal);
				fputc('\n', fileTmpC);
			}
			fclose(fileTmp);
			rewind(fileTmpC);
			while (fscanf(fileTmpC, "%s", datosDelArchivoTemporal) == 1) {
				int escrito = 0;
				char** registro = string_n_split(datosDelArchivoTemporal, 3,
						";");
				int particionDeEstaKey = convertirKey(registro[0])
						% numeroDeParticiones;
				char* pathParticion = string_from_format("%s/%d.bin", pathTabla,
						particionDeEstaKey);
				t_config* particion = config_create(pathParticion);
				char** bloques = config_get_array_value(particion, "BLOCKS");
				for (int i = 0; escrito == 0; i++) {
					if (bloques[i] != NULL) {
						char* pathBloque = string_from_format(
								"%sBloques/%s.bin", puntoDeMontaje, bloques[i]);
						FILE* bloque = fopen(pathBloque, "w+");
						fseek(bloque, 0L, SEEK_END);
						long int tamanioBloqueReal = ftell(bloque);
						fclose(bloque);
						bloque = fopen(pathBloque, "a");
						if ((tamanioBloqueReal + sizeof(uint16_t) + tamanioValue
								+ sizeof(unsigned long long) + 1)
								<= config_get_int_value(configMetadata,
										"BLOCK_SIZE")) {
							fprintf(bloque, "%s", datosDelArchivoTemporal);
							escrito = 1;
						}
						free(pathBloque);
						fclose(bloque);
					} else {
						errorNo errorNo;
						int bloqueDeParticion = obtenerBloqueDisponible(
								&errorNo);
						if (bloqueDeParticion == -1) {
							log_info(logger_LFS, "no hay bloques disponibles");
						} else {
							char* bloquesString = config_get_string_value(
									particion, "BLOCKS");
							bloquesString = strtok(bloquesString, "]");
							string_append_with_format(&bloquesString, "%d]",
									bloqueDeParticion);
							config_set_value(particion, "BLOCKS",
									bloquesString);
							free(bloquesString);
						}
					}
				}
				free(bloques);
				free(registro);
			}
			fclose(fileTmpC);
			free(pathTmpC);
			free(datosDelArchivoTemporal);
		}
		numeroTemporal++;
		free(pathTmp);
	} while (fileTmp != NULL);*/

		// TODO: Separar la lista de datos del tmp en particiones
		// TODO: Comparar con los .bin (leer primero el bin y guardarlo en una lista para comparar listas es mas rapido?
		// TODO: Quedarse con los timestamps mas recientes
		// TODO: Bloquear la tabla: (chmod o mutex?)
		// TODO: Liberar bloques
		// TODO: Pedir mas bloques
		// TODO: Generar nuevo .bin
		// TODO: Desbloquear la tabla y dejar un registro de cuanto tardo la compactacion
}
