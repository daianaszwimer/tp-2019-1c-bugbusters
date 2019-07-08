#include "Helper.h"

void interpretarRequest(cod_request palabraReservada, char* request, int* memoria_fd) {
	char** requestSeparada = separarRequest(request);
	errorNo errorNo = SUCCESS;
	char* mensaje = strdup("");
	if(memoria_fd != NULL){
		log_info(logger_LFS, "Request de la memoria %i", *memoria_fd);
	}
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
			errorNo = procesarDrop(requestSeparada[1]);
			log_info(logger_LFS, "Me llego un DROP");
			break;
		default:
			break;
	}

	char* mensajeDeError;
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
			log_error(logger_LFS, mensajeDeError);
			break;
		case KEY_NO_EXISTE:
			mensajeDeError = string_from_format("La KEY %s no existe", requestSeparada[2]);
			log_info(logger_LFS, mensajeDeError);
			break;
		default:
			break;
	}

	free(mensajeDeError);
	usleep(retardo*1000);

	if (memoria_fd != NULL) {
		enviar(errorNo, mensaje, *memoria_fd);
	}else{
		log_info(logger_LFS, mensaje);
	}

	liberarArrayDeChar(requestSeparada);
	free(mensaje);
	log_info(logger_LFS, "---------------------------------------");
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
