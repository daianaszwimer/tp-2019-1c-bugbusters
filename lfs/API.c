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
