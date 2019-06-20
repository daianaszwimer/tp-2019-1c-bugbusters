#include "lfs.h"
#include <errno.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>


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

/*	if(!pthread_create(&hiloDumpeo, NULL, hiloDump, NULL)){
		log_info(logger_LFS, "Hilo de Dump iniciado");
		pthread_detach(hiloDumpeo);
	}else{
		log_error(logger_LFS, "Error al crear hilo Dumpeo");
	}*/

	pthread_join(hiloLeerDeConsola, NULL);
	log_info(logger_LFS, "Hilo de consola finalizado");
	pthread_join(hiloRecibirMemorias, NULL);
	log_info(logger_LFS, "Hilo recibir memorias finalizado");

	free(pathRaiz);
	log_destroy(logger_LFS);
	config_destroy(configMetadata);
	config_destroy(config);
	return EXIT_SUCCESS;
}




void* leerDeConsola(void* arg) {
	while (1) {
		mensaje = readline(">");
		if (!(strncmp(mensaje, "", 1) != 0)) {
			//signal(hiloRecibirMemorias, )
			pthread_cancel(hiloRecibirMemorias);
			free(mensaje);
			break;
			//dumpear();
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
		if (palabraReservada == -1){
			eliminar_paquete(paqueteRecibido);
			log_error(logger_LFS, "el cliente se desconecto. Terminando servidor");
			break;
		}
		printf("De la memoria nro: %d \n", memoria_fd);
		//TODO ver de interpretar si es -1 q onda
		interpretarRequest(palabraReservada, paqueteRecibido->request, &memoria_fd);
		eliminar_paquete(paqueteRecibido);
	}
	return NULL;
}

void interpretarRequest(cod_request palabraReservada, char* request, int* memoria_fd) {
	char** requestSeparada = separarRequest(request);
	errorNo retorno = SUCCESS;
	char* retorno_string = strdup("");
	switch (palabraReservada){
		case SELECT:
			log_info(logger_LFS, "Me llego un SELECT");
			t_registro* registro = NULL;
			retorno = procesarSelect(requestSeparada[1], requestSeparada[2], &registro);
			if(retorno == SUCCESS && registro != NULL){
				string_append_with_format(&retorno_string, "%s %u %s %llu", requestSeparada[1], registro->key, registro->value, registro->timestamp);
			}else{
				string_append_with_format(&retorno_string, "%s", "No se encontro nada");
			}
			break;
		case INSERT:
			log_info(logger_LFS, "Me llego un INSERT");
			unsigned long long timestamp;
			if(longitudDeArrayDeStrings(requestSeparada) == 5) { //4 parametros + INSERT
				convertirTimestamp(requestSeparada[4], &timestamp);
				retorno = procesarInsert(requestSeparada[1], convertirKey(requestSeparada[2]), requestSeparada[3], timestamp);
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
		default:
			break;
	}

	char* mensajeDeError;
	switch(retorno){
		case SUCCESS:
//			string_append_with_format(&mensajeDeError,"%s%s%s%s",requestSeparada[1]," ",requestSeparada[2]," ","value"); // TODO necesito el valor posta
//			log_info(logger_LFS, mensajeDeError);
			mensajeDeError=strdup("");
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
		enviar(retorno, retorno_string, *memoria_fd);
		free(retorno_string);
	}else{
		log_info(logger_LFS, retorno_string);
	}

	for (int i = 0; requestSeparada[i] != NULL; i++) {
		free(requestSeparada[i]);
	}
	free(requestSeparada);
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
	char* fileBitmap = string_from_format("%s/Metadata/Bitmap.bin", pathRaiz);
	int index = 0;
	int fdBitmap = open(fileBitmap, O_CREAT | O_RDWR, S_IRWXU);
	if(fdBitmap == -1){
		*errorNo = ERROR_CREANDO_ARCHIVO;
		index = -1;
	}else{

		char* bitmap = mmap(NULL, blocks/8, PROT_READ | PROT_WRITE, MAP_SHARED, fdBitmap, 0);
		t_bitarray* bitarray = bitarray_create_with_mode(bitmap, blocks/8, LSB_FIRST);
		while(index < blocks && bitarray_test_bit(bitarray, index)) index++;
		if(index >= blocks) {
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
	char* puntoDeMontaje = config_get_string_value(config, "PUNTO_MONTAJE");
	//pathRaiz = string_from_format("%s%s", PATH , puntoDeMontaje);
	pathRaiz = strdup(puntoDeMontaje);

	memtable = (t_memtable*) malloc(sizeof(t_memtable));
	memtable->tablas = list_create();

	//if error
	mkdir(pathRaiz, S_IRWXU);

	char* pathTablas = string_from_format("%sTablas", pathRaiz);
	char* pathMetadata = string_from_format("%sMetadata", pathRaiz);
	char* pathBloques = string_from_format("%sBloques", pathRaiz);


	mkdir(pathTablas, S_IRWXU);

	mkdir(pathMetadata, S_IRWXU);
	char* fileMetadata = string_from_format("%s/Metadata.bin", pathMetadata);

//	FILE* metadata = fopen(fileMetadata, "w");
//	fclose(metadata);

	configMetadata = config_create(fileMetadata);

	mkdir(pathBloques, S_IRWXU);

	blocks = config_get_int_value(configMetadata, "BLOCKS");
	char* fileBloque;
	for (int i = 1; i <= blocks; i++) {
		fileBloque = string_from_format("%s/%d.bin", pathBloques, i);
		FILE* bloqueFile = fopen(fileBloque, "w");
		fclose(bloqueFile);
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

errorNo procesarSelect(char* nombreTabla, char* key, t_registro** registro){

	int ordenarRegistrosPorTimestamp(t_registro* registro1, t_registro* registro2){
		return registro1->timestamp > registro2->timestamp;
	}

	errorNo error = SUCCESS;
	t_list* listaDeRegistros = list_create();

	char* pathTabla = string_from_format("%s/Tablas/%s", pathRaiz, nombreTabla);
	DIR* dir = opendir(pathTabla);
	if(dir){
		//TODO validar q onda la funcion convertirKey, retorna -1 si hay error
		int _key = convertirKey(key);
		//TODO CHEQUEAR ACA ABAJO PORQ PUEDE VENIR NULL OBTENER REGISTROS, CAMBIAR
		t_list* listaDeRegistrosDeMemtable = obtenerRegistrosDeMemtable(nombreTabla, _key);
		//t_list* listaDeRegistrosDeTmp = obtenerRegistrosDeTmp(nombreTabla, _key);
		list_add_all(listaDeRegistros, listaDeRegistrosDeMemtable);
		//list_add_all(listaDeRegistros, listaDeRegistrosDeTmp);
		if(!list_is_empty(listaDeRegistros)){
			list_sort(listaDeRegistros, (void*) ordenarRegistrosPorTimestamp);
			*registro = (t_registro*)listaDeRegistros->head->data;
		}else{
			error = KEY_NO_EXISTE;
		}
	}else{
		error = TABLA_NO_EXISTE;
	}

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
		while(fscanf(fileTmp, "%s", datos) != EOF){
			t_registro* registro = (t_registro*) malloc(sizeof(t_registro));
			//free dato de abajo xD
			char** dato = string_split(datos, ";");
			free(datos);
			int _key = convertirKey(dato[0]);
			if(_key == key){
				registro->key = key;
				registro->value = strdup(dato[1]);
				convertirTimestamp(dato[2], &registro->timestamp);
				list_add(listaDeRegistros, registro);
			}
		}
		fclose(fileTmp);
		numeroTemporal++;
		pathTmp = string_from_format("%sTablas/%s/%d.tmp", pathRaiz, nombreTabla, numeroTemporal);
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

	t_list* listaDeRegistros = NULL;

	t_tabla* table = list_find(memtable->tablas, (void*) encontrarTabla);
	if(table == NULL){
		listaDeRegistros = list_create();
		log_info(logger_LFS, "No se encontro nada en la memtable");
	}else{
		listaDeRegistros = list_filter(table->registros, (void*) encontrarRegistro);
		if(list_is_empty(listaDeRegistros)){
			log_info(logger_LFS, "No se encontro nada en la memtable");
		}
	}
	return listaDeRegistros;
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
	FILE* fileTmp;

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
			fileTmp = fopen(pathTmp, "w+");
			free(pathTmp);
			if (fileTmp == NULL) {
				error = ERROR_CREANDO_ARCHIVO;
			} else {
				// Guardo lo de la tabla en el archivo temporal
				char* datosADumpear = strdup("");
//				TODO NICO: ver si lo de abajo sirve
//				char* datosADumpear = malloc(sizeof(uint16_t) + (size_t) config_get_int_value(config, "TAMAÑO_VALUE") + sizeof(unsigned long long));
//				strcpy(datosADumpear, "");
				for(int j = 0; list_get(tabla->registros,j) != NULL; j++) {
					t_registro* registro = list_get(tabla->registros,j);
					string_append_with_format(&datosADumpear, "%u;%s;%llu\n", registro->key, registro->value, registro->timestamp);
				}
				fprintf(fileTmp, "%s", datosADumpear);
				free(datosADumpear);
				fclose(fileTmp);
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

void compactacion(char* pathTabla) {
	int numeroTemporal = 0;
	int tamanioValue = config_get_int_value(config, "TAMAÑO_VALUE");
	FILE* fileTmp;
	do {
		char* pathTmp = string_from_format("%s/%d.tmp", pathTabla, numeroTemporal);
		fileTmp = fopen(pathTmp, "r");
		if(fileTmp != NULL) { // Crear tmpc de los tmp
			t_list* listaDeDatosDelTmp = list_create();
			char* pathTmpC = string_from_format("%s/%d.tmpc", pathTabla, numeroTemporal);
			FILE* fileTmpC = fopen(pathTmpC, "w");
			char* datosDelArchivoTemporal = malloc((int) (sizeof(uint16_t) + tamanioValue + sizeof(unsigned long long)));
			while(fscanf(fileTmp, "%s", datosDelArchivoTemporal) == 1) {
				fprintf(fileTmpC, "%s", datosDelArchivoTemporal);
				char** dato = string_split(datosDelArchivoTemporal, ";");
				t_registro* registro = (t_registro*) malloc(sizeof(t_registro));
				registro->key = convertirKey(dato[0]);
				registro->value = strdup(dato[1]);
				convertirTimestamp(dato[2], &registro->timestamp);
				list_add(listaDeDatosDelTmp, registro);
				fputc('\n', fileTmpC);
			}
			fclose(fileTmpC);
			fclose(fileTmp);
			free(pathTmpC);
			free(datosDelArchivoTemporal);
		}
		numeroTemporal++;
		free(pathTmp);
	} while(fileTmp != NULL);

	// TODO: Separar la lista de datos del tmp en particiones
	// TODO: Comparar con los .bin (leer primero el bin y guardarlo en una lista para comparar listas es mas rapido?
	// TODO: Quedarse con los timestamps mas recientes
	// TODO: Bloquear la tabla: (chmod o mutex?)
	// TODO: Liberar bloques
	// TODO: Pedir mas bloques
	// TODO: Generar nuevo .bin
	// TODO: Desbloquear la tabla y dejar un registro de cuanto tardo la compactacion

}
