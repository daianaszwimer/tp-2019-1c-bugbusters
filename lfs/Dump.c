#include "Dump.h"

/* hiloDump()
 * Parametros: void
 * Descripcion: ejecuta la funcion dumpear() cada cierta cantidad de tiempo (tiempoDump) definido en el config
 * Return: void* */
void* hiloDump(void* args) {
	int tiempo_dump;

	t_int* fd = malloc(sizeof(t_int));
	fd->valor = 2;

	pthread_mutex_lock(&mutexMemorias);
	list_add(memorias, fd);
	pthread_mutex_unlock(&mutexMemorias);

	void agregarMemoria(t_hiloTabla* hiloTabla) {
		t_bloqueo* idYMutex = malloc(sizeof(t_bloqueo));
		idYMutex->id = 2;
		pthread_mutex_init(&(idYMutex->mutex), NULL);
		list_add(hiloTabla->cosasABloquear, idYMutex);
	}

	pthread_mutex_lock(&mutexTablasParaCompactaciones);
	list_iterate(tablasParaCompactaciones, (void*)agregarMemoria);
	pthread_mutex_unlock(&mutexTablasParaCompactaciones);

	while(1) {
		pthread_mutex_lock(&mutexTiempoDump);
		tiempo_dump = tiempoDump;
		pthread_mutex_unlock(&mutexTiempoDump);
		usleep(tiempo_dump*1000);
		errorNo resultado = dumpear();
		switch(resultado) {
			case ERROR_CREANDO_ARCHIVO:
				log_error(logger_LFS, "Error sobre el dump");
				break;
			case SUCCESS:
				log_info(logger_LFS, "Realice un dump con exito");
				break;
			default: break;
		}
	}
}

//TODO que pasa si se intenta dumpear a una tabla que fue dropeada

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
	char* pathMetadata = string_from_format("%sMetadata/Metadata.bin", pathRaiz);
	t_config* configMetadata = config_create(pathMetadata);
	free(pathMetadata);
	int tamanioBloque = config_get_int_value(configMetadata, "BLOCK_SIZE");
	config_destroy(configMetadata);
	// Refactor list_iterate
	pthread_mutex_lock(&mutexMemtable);
	if(!memtable->tablas->elements_count){
		error = ERROR_WILLY;
	}
	for(int i = 0; list_get(memtable->tablas,i) != NULL; i++) { // Recorro las tablas de la memtable
		tabla = list_get(memtable->tablas,i);
		char* pathTabla = string_from_format("%sTablas/%s", pathRaiz, tabla->nombreTabla);
		DIR *dir = opendir(pathTabla);
		if(dir) { // Verificamos que exista la tabla (por si hubo un DROP en el medio)
			int numeroTemporal = 0;

			int encontrarTabla(t_hiloTabla* tabla2) { //IS CHECKED >:v
				return string_equals_ignore_case(tabla2->nombreTabla, tabla->nombreTabla);
			}

			int esDump(t_bloqueo* idYMutex){ //busca el dump en dicha tabla y lo bloquea
				return idYMutex->id == 2;
			}


			pthread_mutex_lock(&mutexTablasParaCompactaciones);
			t_hiloTabla* hiloTabla = list_find(tablasParaCompactaciones, (void*) encontrarTabla);
			t_bloqueo* idYMutex = list_find(hiloTabla->cosasABloquear, (void*) esDump);
			pthread_mutex_unlock(&mutexTablasParaCompactaciones);
			pthread_mutex_lock(&(idYMutex->mutex));

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
			free(pathTmp);
			if (fileTmp == NULL) {
				error = ERROR_CREANDO_ARCHIVO;
			} else {
				// Guardo lo de la tabla en el archivo temporal
				char* datosADumpear = calloc(1, 27 + tamanioValue);
				strcpy(datosADumpear, "");
				for(int j = 0; list_get(tabla->registros,j) != NULL; j++) {
					t_registro* registro = list_get(tabla->registros,j);
					string_append_with_format(&datosADumpear, "%llu;%u;%s\n", registro->timestamp, registro->key, registro->value);
				}
				int cantidadDeBloquesAPedir = strlen(datosADumpear) / tamanioBloque;
				if(strlen(datosADumpear) % tamanioBloque != 0) {
					cantidadDeBloquesAPedir++;
				}
				char* tamanioTmp = string_from_format("SIZE=%d", strlen(datosADumpear));
				char* bloques = strdup("");
				for(int i=0; i<cantidadDeBloquesAPedir;i++) {
					int bloqueDeParticion = obtenerBloqueDisponible();
					if(bloqueDeParticion == -1){
						log_info(logger_LFS, "no hay bloques disponibles");
					} else {
						if(i==cantidadDeBloquesAPedir-1) {
							string_append_with_format(&bloques, "%d", bloqueDeParticion);
						} else {
							string_append_with_format(&bloques, "%d,", bloqueDeParticion);
						}
						char* pathBloque = string_from_format("%sBloques/%d.bin", pathRaiz, bloqueDeParticion);
						FILE* bloque = fopen(pathBloque, "a+");
						if(cantidadDeBloquesAPedir != 1 && i < cantidadDeBloquesAPedir - 1) {
							char* registrosAEscribir = string_substring_until(datosADumpear, tamanioBloque);
							char* stringAuxiliar = string_substring_from(datosADumpear, tamanioBloque);
							free(datosADumpear);
							datosADumpear = stringAuxiliar;
							fprintf(bloque, "%s", registrosAEscribir);
							free(registrosAEscribir);
						} else {
							fprintf(bloque, "%s", datosADumpear);
						}
						fclose(bloque);
						free(pathBloque);
					}
				}
				char* bloquesTmp = string_from_format("BLOCKS=[%s]", bloques);
				free(bloques);
				fprintf(fileTmp, "%s\n%s", tamanioTmp, bloquesTmp);
				free(bloquesTmp);
				free(tamanioTmp);
				free(datosADumpear);
				fclose(fileTmp);
			}
			pthread_mutex_unlock(&(idYMutex->mutex));
			closedir(dir);
		}
	}
	// Vacio la memtable
	list_clean_and_destroy_elements(memtable->tablas, (void*) vaciarTabla);
	pthread_mutex_unlock(&mutexMemtable);
	return error;
}
