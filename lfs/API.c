#include "API.h"

/* procesarCreate() [API]
 * Parametros:
 * 	-> nombreTabla :: char*
 * 	-> tipoDeConsistencia :: char*
 * 	-> numeroDeParticiones :: int
 * 	-> tiempoDeCompactacion :: int
 * Descripcion: permite la creación de una nueva tabla dentro del file system
 * Return: codigos de error o success*/

errorNo procesarCreate(char* nombreTabla, char* tipoDeConsistencia,	char* numeroDeParticiones, char* tiempoDeCompactacion) {

	string_to_upper(nombreTabla);
	char* pathTabla = string_from_format("%sTablas/%s", pathRaiz, nombreTabla);
	errorNo error = SUCCESS;

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

	if(error == SUCCESS){
		if(!pthread_create(&hiloDeCompactacion, NULL, (void*) hiloCompactacion, (void*) pathTabla)){
			pthread_detach(hiloDeCompactacion);
			//TODO mutex
			t_hiloTabla* hiloTabla = malloc(sizeof(t_hiloTabla));
			hiloTabla->thread = &hiloDeCompactacion;
			hiloTabla->nombreTabla = strdup(nombreTabla);
			hiloTabla->flag = 1;
			pthread_mutex_lock(&diegote);
			list_add(diegote, hiloTabla);
			pthread_mutex_unlock(&diegote);
			log_info(logger_LFS, "Hilo de compactacion de la tabla %s creado", nombreTabla);
		}else{
			log_error(logger_LFS, "Error al crear hilo de compactacion de la tabla %s", nombreTabla);
		}
	}


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
			int bloqueDeParticion = obtenerBloqueDisponible();
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

	if(errorNo ==SUCCESS){

	}

	return errorNo;
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
	string_to_upper(nombreTabla);
	char* pathTabla = string_from_format("%sTablas/%s", pathRaiz, nombreTabla);
	errorNo error = SUCCESS;


	/* Validamos si la tabla existe */
	DIR *dir = opendir(pathTabla);
	if (dir) {
		t_registro* registro = (t_registro*) malloc(sizeof(t_registro));
		registro->key = key;
		registro->value = strdup(value);
		registro->timestamp = timestamp;
		pthread_mutex_lock(&mutexMemtable);
		t_tabla* tabla = list_find(memtable->tablas, (void*) encontrarTabla);
		if (tabla == NULL) {
			log_info(logger_LFS, "Se agrego la tabla a la memtable y se agrego el registro");
			tabla = (t_tabla*) malloc(sizeof(t_tabla));
			tabla->nombreTabla = strdup(nombreTabla);
			tabla->registros = list_create();
			list_add(memtable->tablas, tabla);
		}
		list_add(tabla->registros, registro);
		pthread_mutex_unlock(&mutexMemtable);
	} else {
		error = TABLA_NO_EXISTE;
	}

	free(pathTabla);
	free(dir);
	return error;
}


errorNo procesarSelect(char* nombreTabla, char* key, char** mensaje){

	int ordenarRegistrosPorTimestamp(t_registro* registro1, t_registro* registro2){
		return registro1->timestamp > registro2->timestamp;
	}

	string_to_upper(nombreTabla);
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
		t_list* listaDeRegistrosDeParticiones = obtenerRegistrosDeParticiones(nombreTabla, _key);
		list_add_all(listaDeRegistros, listaDeRegistrosDeMemtable);
		list_add_all(listaDeRegistros, listaDeRegistrosDeTmp);
		list_add_all(listaDeRegistros, listaDeRegistrosDeParticiones);
		if(!list_is_empty(listaDeRegistros)){
			list_sort(listaDeRegistros, (void*) ordenarRegistrosPorTimestamp);
			t_registro* registro = (t_registro*)listaDeRegistros->head->data;
			string_append_with_format(&*mensaje, "%s %llu %u \"%s\"", nombreTabla, registro->timestamp, registro->key, registro->value);
		}else{
			//https://github.com/sisoputnfrba/foro/issues/1406
			error = KEY_NO_EXISTE;
			string_append_with_format(&*mensaje, "Registro no encontrado salu3");
		}
		list_destroy(listaDeRegistrosDeMemtable);
		list_destroy_and_destroy_elements(listaDeRegistrosDeTmp, (void*)eliminarRegistro);
		list_destroy_and_destroy_elements(listaDeRegistrosDeParticiones, (void*)eliminarRegistro);
	}else{
		error = TABLA_NO_EXISTE;
	}
	list_destroy(listaDeRegistros);
	free(dir);
	return error;
}

t_list* obtenerRegistrosDeTmp(char* nombreTabla, int key){
	char* pathTabla;
	struct dirent* file;
	char* pathFile;
	t_list* listaDeRegistros = list_create();
	t_list* listaDeRegistrosEnBloques;
	string_to_upper(nombreTabla);
	pathTabla = string_from_format("%s/%s", pathTablas, nombreTabla);
	DIR* tabla = opendir(pathTabla);
	if(tabla){
		while ((file = readdir (tabla)) != NULL) {
			//ignora . y ..
			if(strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) continue;

			if(string_ends_with(file->d_name, ".tmp")){
				pathFile = string_from_format("%s/%s", pathTabla, file->d_name);
				t_config* file = config_create(pathFile);
				char** bloques = config_get_array_value(file, "BLOCKS");
				listaDeRegistrosEnBloques = buscarEnBloques(bloques, key);
				list_add_all(listaDeRegistros, listaDeRegistrosEnBloques);
				list_destroy(listaDeRegistrosEnBloques);
				liberarArrayDeChar(bloques);
				free(pathFile);
				config_destroy(file);
			}
		}
		closedir(tabla);
	}
	free(pathTabla);
	return listaDeRegistros;
}

t_list* obtenerRegistrosDeParticiones(char* nombreTabla, int key){
	char* pathTabla;
	struct dirent* file;
	char* pathFile;
	t_list* listaDeRegistros = list_create();
	t_list* listaDeRegistrosEnBloques;
	pathTabla = string_from_format("%s/%s", pathTablas, nombreTabla);
	DIR* tabla = opendir(pathTabla);
	if(tabla){
		while ((file = readdir (tabla)) != NULL) {
			//ignora . y ..
			if(strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0 || strcmp(file->d_name, "Metadata.bin") == 0) continue;

			if(string_ends_with(file->d_name, ".bin")){
				pathFile = string_from_format("%s/%s", pathTabla, file->d_name);
				t_config* file = config_create(pathFile);
				char** bloques = config_get_array_value(file, "BLOCKS");
				listaDeRegistrosEnBloques = buscarEnBloques(bloques, key);
				list_add_all(listaDeRegistros, listaDeRegistrosEnBloques);
				list_destroy(listaDeRegistrosEnBloques);
				liberarArrayDeChar(bloques);
				free(pathFile);
				config_destroy(file);
			}
		}
		closedir(tabla);
	}
	free(pathTabla);
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
	pthread_mutex_lock(&mutexMemtable);
	t_tabla* table = list_find(memtable->tablas, (void*) encontrarTabla);
	if(table == NULL){
		listaDeRegistros = list_create();
	}else{
		listaDeRegistros = list_filter(table->registros, (void*) encontrarRegistro);
	}
	pthread_mutex_unlock(&mutexMemtable);
	return listaDeRegistros;
}


t_list* buscarEnBloques(char** bloques, int key){
	int i = 0, j = 0;
	t_registro* tRegistro;
	char* registro = calloc(1, (size_t) config_get_int_value(config, "TAMAÑO_VALUE") + 25);
	t_list* listaDeRegistros = list_create();
	while (bloques[i] != NULL) {
		char* pathBloque = string_from_format("%s/%s.bin", pathBloques, bloques[i]);
		FILE* bloque = fopen(pathBloque, "r");
		free(pathBloque);
		if (bloque == NULL) {
			perror("Error");
		}
		do {
			char caracterLeido = fgetc(bloque);
			if (feof(bloque)) {
				break;
			}
			if (caracterLeido == '\n') {
				char** registroSeparado = string_n_split(registro, 3, ";");
				tRegistro = (t_registro*) malloc(sizeof(t_registro));
				convertirTimestamp(registroSeparado[0], &(tRegistro->timestamp));
				tRegistro->key = convertirKey(registroSeparado[1]);
				tRegistro->value = strdup(registroSeparado[2]);
				liberarArrayDeChar(registroSeparado);
				if(key == tRegistro->key){
					list_add(listaDeRegistros, tRegistro);
				}else{
					eliminarRegistro(tRegistro);
				}
				j = 0;
				strcpy(registro, "");
			} else {
				registro[j] = caracterLeido;
				j++;
			}
		} while (1);
		fclose(bloque);
		i++;
	}
	free(registro);
	return listaDeRegistros;
}

errorNo procesarDescribe(char* nombreTabla, char** mensaje){
	errorNo error = SUCCESS;
	char* pathTablas = string_from_format("%sTablas", pathRaiz);
	char* pathTabla;
	string_to_upper(nombreTabla);
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
			perror("Error al abrir directorio de tablas");
		}
	}
	free(pathTablas);
	return error;
}

char* obtenerMetadata(char* pathTabla){
	char* mensaje = strdup("");
	DIR* dir = opendir(pathTabla);
	if(dir != NULL){
		closedir(dir);
		char* pathMetadata = string_from_format("%s/%s", pathTabla, "Metadata.bin");
		t_config* metadata = config_create(pathMetadata);
		free(pathMetadata);
		if(metadata != NULL){
			if(config_has_property(metadata, "CONSISTENCY") && config_has_property(metadata, "PARTITIONS") && config_has_property(metadata, "COMPACTION_TIME")){
				string_append_with_format(&mensaje,"%s %i %i", config_get_string_value(metadata, "CONSISTENCY"), config_get_int_value(metadata, "PARTITIONS"), config_get_int_value(metadata, "COMPACTION_TIME"));
			}else{
				log_error(logger_LFS,"El metadata de la tabla %s no tiene alguna de las config", pathTabla);
			}
			config_destroy(metadata);
		}else{
			log_error(logger_LFS, "No se pudo levantar como config el metadata de la tabla %s", pathTabla);
		}
	}else{
		log_error(logger_LFS, "No se pudo abrir el metadata de la tabla %s", pathTabla);
		perror("Error");
	}
	return mensaje;
}

errorNo procesarDrop(char* nombreTabla){
	string_to_upper(nombreTabla);
	int encontrarTabla(t_hiloTabla* tabla) {
		return string_equals_ignore_case(tabla->nombreTabla, nombreTabla);
	}


	errorNo error = SUCCESS;
	char* pathTabla = string_from_format("%s/%s", pathTablas, nombreTabla);
	DIR* tabla = opendir(pathTabla);
	if(tabla){
		pthread_mutex_lock(&diegote);
		t_hiloTabla* tablaEncontrada = list_find(diegote, (void*)encontrarTabla);
		tablaEncontrada->flag = 0;
		pthread_mutex_unlock(&diegote);
		borrarArchivosYLiberarBloques(tabla, pathTabla);
		closedir(tabla);
		rmdir(pathTabla);
	}else{
		error = TABLA_NO_EXISTE;
	}
	free(pathTabla);

	return error;
}

void borrarArchivosYLiberarBloques(DIR* tabla, char* pathTabla){
	char* pathFile;
	struct dirent* file;
	char* pathTablaMetadata = string_from_format("%s/%s", pathTabla, "/Metadata.bin");
	remove(pathTablaMetadata);
	free(pathTablaMetadata);
	while ((file = readdir (tabla)) != NULL) {
		//ignora . y ..
		if(strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) continue;

		if(string_ends_with(file->d_name, ".bin") || string_ends_with(file->d_name, ".tmp")) {
			pathFile = string_from_format("%s/%s", pathTabla, file->d_name);
			t_config* file = config_create(pathFile);
			char** bloques = config_get_array_value(file, "BLOCKS");
			int i = 0;
			while (bloques[i] != NULL) {
				char* pathBloque = string_from_format("%s/%s.bin", pathBloques, bloques[i]);
				FILE* bloque = fopen(pathBloque, "w");
				free(pathBloque);
				fclose(bloque);
				bitarray_clean_bit(bitarray, strtol(bloques[i], NULL, 10));
				i++;
			}
			liberarArrayDeChar(bloques);
			remove(pathFile);
			free(pathFile);
			config_destroy(file);
		}
	}
}
