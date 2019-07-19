#include "Helper.h"

void interpretarRequest(cod_request palabraReservada, char* request, int* memoria_fd) {
	char** requestSeparada = separarRequest(request);
	errorNo errorNo = SUCCESS;
	char* mensaje = strdup("");
//	if(memoria_fd != NULL){
//		if(!(*memoria_fd)) {
//			log_info(logger_LFS, "Request de la consola de lfs");
//		} else {
//			log_info(logger_LFS, "Request de la memoria %i", *memoria_fd);
//		}
//	}
	switch (palabraReservada){
		case SELECT:
			//log_info(logger_LFS, "Me llego un SELECT");
			errorNo = procesarSelect(requestSeparada[1], requestSeparada[2], &mensaje, *memoria_fd);
			break;
		case INSERT:
			//log_info(logger_LFS, "Me llego un INSERT");
			if(longitudDeArrayDeStrings(requestSeparada) == 5) { //4 parametros + INSERT
				unsigned long long timestamp;
				convertirTimestamp(requestSeparada[4], &timestamp);
				errorNo = procesarInsert(requestSeparada[1], convertirKey(requestSeparada[2]), requestSeparada[3], timestamp);
			} else {
				errorNo = procesarInsert(requestSeparada[1], convertirKey(requestSeparada[2]), requestSeparada[3], obtenerHoraActual());
			}
			break;
		case CREATE:
			//log_info(logger_LFS, "Me llego un CREATE");
			errorNo = procesarCreate(requestSeparada[1], requestSeparada[2], requestSeparada[3], requestSeparada[4]);
			break;
		case DESCRIBE:
			if(longitudDeArrayDeStrings(requestSeparada) == 2){
				errorNo = procesarDescribe(requestSeparada[1], &mensaje);
			}else{
				errorNo = procesarDescribe(NULL, &mensaje);
			}
			//log_info(logger_LFS, "Me llego un DESCRIBE");
			break;
		case DROP:
			errorNo = procesarDrop(requestSeparada[1]);
			//log_info(logger_LFS, "Me llego un DROP");
			break;
		default:
			break;
	}

	char* mensajeDeError;
	switch(errorNo){
		case SUCCESS:
			mensajeDeError= strdup("");
			break;
		case TABLA_EXISTE:
			mensajeDeError = string_from_format("La tabla %s ya existe", requestSeparada[1]);
			break;
		case ERROR_CREANDO_ARCHIVO:
			mensajeDeError = string_from_format("Error al crear un archivo de la tabla %s", requestSeparada[1]);
			break;
		case TABLA_NO_EXISTE:
			mensajeDeError = string_from_format("La tabla %s no existe", requestSeparada[1]);
			break;
		case KEY_NO_EXISTE:
			mensajeDeError = string_from_format("La KEY %s no existe", requestSeparada[2]);
			break;
		case VALUE_INVALIDO:
			mensajeDeError = string_from_format("El VALUE %s supera el tamanio maximo establecido en LFS", requestSeparada[3]);
			break;
		default:
			break;
	}

	if(!string_is_empty(mensajeDeError)){
		log_error(logger_LFS, mensajeDeError);
	}

	free(mensajeDeError);
	usleep(retardo*1000);

	if (*memoria_fd != 0) {
		enviar(errorNo, mensaje, *memoria_fd);
	}else{
		if(errorNo == SUCCESS){
			if(!string_is_empty(mensaje)){
				log_info(logger_LFS, mensaje);
			}else{
				log_info(logger_LFS, "Request ejecutada correctamente");
			}
		}
	}

	liberarArrayDeChar(requestSeparada);
	free(mensaje);
}

int obtenerBloqueDisponible() {
	int index = 0;
	pthread_mutex_lock(&mutexBitmap);
	while (index < blocks && bitarray_test_bit(bitarray, index) == 1) index++;
	if(index >= blocks) {
		index = -1;
	}else{
		bitarray_set_bit(bitarray, index);
	}
	pthread_mutex_unlock(&mutexBitmap);
	return index;
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

void liberarMutexTabla(t_bloqueo* idYMutex) {
	pthread_mutex_destroy(&(idYMutex->mutex));
	free(idYMutex);
}
