#include "lfs.h"
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdlib.h>

int main(void) {
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/lfs/lfs.config");
	logger_LFS = log_create("lfs.log", "Lfs", 1, LOG_LEVEL_DEBUG);
	log_info(logger_LFS, "----------------INICIO DE LISSANDRA FS--------------");
	
	inicializarLfs();
	//create("TABLA1", "SC", 3, 5000);

	if(pthread_create(&hiloRecibirDeMemoria, NULL, (void*)recibirConexionesMemoria, NULL)){

	}

	leerDeConsola();

	pthread_join(hiloRecibirDeMemoria, NULL);
	
	log_destroy(logger_LFS);
	config_destroy(config);
	return 0;
}

void leerDeConsola(void) {
	//log_info(logger, "leerDeConsola");
	while (1) {
		mensaje = readline(">");
		validarRequest(mensaje);
		free(mensaje);
	}
}

void validarRequest(char* mensaje){
	int codValidacion;
	char** request = string_n_split(mensaje, 2, " ");
	codValidacion = validarMensaje(mensaje, LFS, logger_LFS);
	cod_request palabraReservada = obtenerCodigoPalabraReservada(request[0],LFS);
	switch(codValidacion){
		case EXIT_SUCCESS:
			interpretarRequest(palabraReservada, mensaje,CONSOLE);
			break;
		case NUESTRO_ERROR:
			//es la q hay q hacerla generica
			log_error(logger_LFS, "La request no es valida");
			break;
		default:
			break;
	}
}

void recibirConexionesMemoria() {
	int lissandraFS_fd = iniciar_servidor(config_get_string_value(config, "PUERTO"), config_get_string_value(config, "IP"));
	log_info(logger_LFS, "Lissandra lista para recibir Memorias");
	pthread_t hiloRequest;

	while(1){
		int memoria_fd = esperar_cliente(lissandraFS_fd);
		hiloRequest = malloc(sizeof(pthread_t));
		if(pthread_create(&hiloRequest, NULL, (void*)procesarRequest, (int) memoria_fd)){
			pthread_detach(hiloRequest);
		} else {
			printf("Error al iniciar el hilo de la memoria con fd = %i", memoria_fd);
		}
	}
}


void procesarRequest(int memoria_fd){
	while(1){
		t_paquete* paqueteRecibido = recibir(memoria_fd);
		cod_request palabraReservada = paqueteRecibido->palabraReservada;
		//printf("El codigo que recibi de Memoria es: %d \n", palabraReservada);
		printf("De la memoria nro: %d \n", memoria_fd);
		interpretarRequest(palabraReservada, paqueteRecibido->request, HIMSELF);
		//t_paquete* paquete = armar_paquete(palabraReservada, respuesta);
		enviar(palabraReservada, paqueteRecibido->request ,memoria_fd);
	}
}

void interpretarRequest(cod_request palabraReservada, char* request, t_caller caller) {

	switch(palabraReservada) {
		case SELECT:
			log_info(logger_LFS, "Me llego un SELECT");
			break;
		case INSERT:
			log_info(logger_LFS, "Me llego un INSERT");
			break;
		case CREATE:
			log_info(logger_LFS, "Me llego un CREATE");
			break;
		case DESCRIBE:
			log_info(logger_LFS, "Me llego un DESCRIBE");
			break;
		case DROP:
			log_info(logger_LFS, "Me llego un DROP");
			break;
		case NUESTRO_ERROR:
			if(caller == HIMSELF){
				log_error(logger_LFS, "El cliente se desconecto");
				break;
			}
			else{
				break;
			}
		default:
			log_warning(logger_LFS, "Operacion desconocida. No quieras meter la pata");
			break;
	}
	puts("\n");
}

/* create() [API]
 * Parametros:
 * 	-> nombreTabla :: char*
 * 	-> tipoDeConsistencia :: char*
 * 	-> numeroDeParticiones :: int
 * 	-> tiempoDeCompactacion :: int
 * Descripcion: permite la creación de una nueva tabla dentro del file system
 * Return:  */
void Create(char* nombreTabla, char* tipoDeConsistencia, int numeroDeParticiones, int tiempoDeCompactacion) {
	// La carpeta metadata y la de los bloques no se crea aca, se crea apenas levanta el lfs

	char* pathTabla = concatenar(PATH, raiz, "/Tablas/", nombreTabla, NULL);

	/* Creamos el archivo Metadata */
	DIR *dir = opendir(pathTabla);

	if(!dir){
		int error = crearDirectorio(pathTabla);
		if (error == 0) {

			char* pathMetadata = concatenar(pathTabla, "/Metadata.bin", NULL);

			FILE* metadata = fopen(pathMetadata, "w+");
			if (metadata != NULL) {
				//char* _tipoDeConsistencia = concatenar("CONSISTENCY=", tipoDeConsistencia, );
				//TODO: Aca hay que escribir CONSISTENCY=tipoDeConsistencia, lo mismo con los otros 2.
				fwrite(&tipoDeConsistencia, strlen(tipoDeConsistencia), 1, metadata);
				fwrite(&numeroDeParticiones, sizeof(numeroDeParticiones), 1, metadata);
				fwrite(&tiempoDeCompactacion, sizeof(tiempoDeCompactacion), 1, metadata);
				fclose(metadata);
			}
			free(pathMetadata);

			char* _i = strdup("");
			/* Creamos las particiones */
			for(int i = 0; i < numeroDeParticiones; i++) {
				sprintf(_i, "%d", i); // Convierto i de int a string
				char* pathParticion = concatenar(pathTabla, "/", _i, ".bin", NULL);
				FILE* particion = fopen(pathParticion, "w+");
				char* tamanio = strdup("Size=0");
				char* bloques = strdup("");
				sprintf(bloques,"Block=[%d]", obtenerBloqueDisponible()); // Hay que agregar un bloque por particion, no se si uno cualquiera o que
				fwrite(&tamanio, sizeof(tamanio), 1, particion);
				fwrite(&bloques, sizeof(bloques), 1, particion);
				free(pathParticion);
				free(tamanio);
				free(bloques);
			}
			free(_i);
		}
	}
	free(pathTabla);

}

int obtenerBloqueDisponible() {
	return 1;
}

int crearDirectorio(char* path){
	char** directorios = string_split(path,"/");
	int i = 0;
	char* _path = strdup("/");
	while(directorios[i] != NULL){
		_path = concatenar(_path, directorios[i],"/", NULL);
		mkdir(_path, S_IRWXU);
		i++;
	}
	return 0;
}

//TODO: dado que crear directorio podria retornar si hubo errores o no, catchear esos errores en la creacion de cada directorio.
void inicializarLfs() {
	raiz = config_get_string_value(config, "PUNTO_MONTAJE");

	char* tablasPath = concatenar(PATH, raiz, "Tablas", NULL);
	char* metadataPath = concatenar(PATH, raiz, "Metadata", NULL);
	char* bloquesPath = concatenar(PATH, raiz, "Bloques", NULL);

	crearDirectorio(tablasPath);

	crearDirectorio(metadataPath);

	metadataPath = concatenar(metadataPath, "/Metadata.bin", NULL);

	FILE* file = fopen(metadataPath, "w+");
	if(file != NULL){
		fclose(file);
	}
	t_config *metadataConfig = config_create(metadataPath);

	config_set_value(metadataConfig, "BLOCK_SIZE", "64");
	config_set_value(metadataConfig, "BLOCKS", "10");
	config_set_value(metadataConfig, "MAGIC_NUMBER", "LISSANDRA");

	config_save(metadataConfig);

	crearDirectorio(bloquesPath);


	/*
	 ver si la cantidad de bloques se puede obtener levantando el metadata como config y crear la cantidad de bloques que dice en dicho
	 archivo (chequear si aca es el momento de hacer esto).
	*/

	int blocks = config_get_int_value(metadataConfig, "BLOCKS");
	for(int i = 1; i <= blocks; i++){
		FILE* block = fopen(concatenar(bloquesPath,"/" ,string_itoa(i), ".bin" , NULL),"w+");
		if(block != NULL){
			fclose(block);
		}
	}

	free(raiz);
	free(tablasPath);
	free(metadataPath);
	free(bloquesPath);
}
