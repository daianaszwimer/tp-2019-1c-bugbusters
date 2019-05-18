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

	if(pthread_create(&hiloRecibirDeMemoria, NULL, (void*)recibirConexionesMemoria, NULL)){

	}

	leerDeConsola();

	pthread_join(hiloRecibirDeMemoria, NULL);
	

	free(pathRaiz);
	log_destroy(logger_LFS);
	config_destroy(config);
	return 0;
}

void leerDeConsola(void) {
	//log_info(logger, "leerDeConsola");
	while (1) {
		mensaje = readline(">");
		validarRequest(mensaje);
	}
	free(mensaje);
}

void validarRequest(char* mensaje){
	int codValidacion;
	char** request = string_n_split(mensaje, 2, " ");
	codValidacion = validarMensaje(mensaje, LFS, logger_LFS);
	cod_request palabraReservada = obtenerCodigoPalabraReservada(request[0],LFS);
	switch(codValidacion){
		case EXIT_SUCCESS:
			interpretarRequest(palabraReservada, mensaje, CONSOLE);
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
			//Create()
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

	char* pathTabla = concatenar(PATH, pathRaiz, "/Tablas/", nombreTabla, NULL);

	/* Validamos si la tabla existe */
	DIR *dir = opendir(pathTabla);

	if(!dir){
		/* Creamos la carpeta de la tabla */
		int error = crearDirectorio(pathTabla);
		if (error == 0) {
			char* metadataPath = concatenar(pathTabla, "/Metadata.bin", NULL);
			/* Creamos el archivo Metadata */
			FILE* metadata = fopen(metadataPath, "w+");
			if (metadata != NULL) {
				fclose(metadata);
				t_config *metadataConfig = config_create(metadataPath);
				config_set_value(metadataConfig, "CONSISTENCY", tipoDeConsistencia);
				config_set_value(metadataConfig, "PARTITIONS", string_itoa(tipoDeConsistencia));
				config_set_value(metadataConfig, "COMPACTION_TIME", string_itoa(tiempoDeCompactacion));
				config_save(metadataConfig);
				free(metadataPath);

				/* Creamos las particiones */
				for(int i = 0; i < numeroDeParticiones; i++) {
					char* pathParticion = concatenar(pathTabla, "/", string_itoa(i), ".bin", NULL);
					FILE* fileParticion = fopen(pathParticion, "w+");
					if(fileParticion != NULL){
						fclose(fileParticion);
						t_config *configParticion = config_create(pathParticion);

						config_set_value(configParticion, "SIZE", "0");
						config_set_value(configParticion, "BLOCKS", string_itoa(obtenerBloqueDisponible()));
						config_save(configParticion);
						config_destroy(configParticion);
					} else {
						//TODO archivo particion i no creada
					}
					free(pathParticion);
				}
			} else {
				// TODO archivo metadata no creado
			}

		} else {
			//TODO error al crear tabla
		}
	} else {
		//TODO existe tabla
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

void inicializarLfs() {
	pathRaiz = config_get_string_value(config, "PUNTO_MONTAJE");

	char* pathTablas = concatenar(PATH, pathRaiz, "Tablas", NULL);
	char* pathMetadata = concatenar(PATH, pathRaiz, "Metadata", NULL);
	char* pathBloques = concatenar(PATH, pathRaiz, "Bloques", NULL);

	crearDirectorio(pathTablas);

	crearDirectorio(pathMetadata);

	pathMetadata = concatenar(pathMetadata, "/Metadata.bin", NULL);

	FILE* file = fopen(pathMetadata, "w+");
	if(file != NULL){
		fclose(file);
	}
	t_config *configMetadata = config_create(pathMetadata);

	config_set_value(configMetadata, "BLOCK_SIZE", "64");
	config_set_value(configMetadata, "BLOCKS", "10");
	config_set_value(configMetadata, "MAGIC_NUMBER", "LISSANDRA");

	config_save(configMetadata);

	crearDirectorio(pathBloques);

	int blocks = config_get_int_value(configMetadata, "BLOCKS");
	for(int i = 1; i <= blocks; i++){
		FILE* block = fopen(concatenar(pathBloques, "/", string_itoa(i), ".bin", NULL), "w+");
		if(block != NULL){
			fclose(block);
		}
	}

	config_destroy(configMetadata);
	free(pathTablas);
	free(pathMetadata);
	free(pathBloques);
}
