#include "lfs.h"
#include <errno.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>


int main(void) {
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/lfs/lfs.config");
	logger_LFS = log_create("lfs.log", "Lfs", 1, LOG_LEVEL_DEBUG);
	log_info(logger_LFS, "----------------INICIO DE LISSANDRA FS--------------");
	char** a = separarRequest("INSERT 1 \"Hola como te va\" 12345");
	//inicializarLfs();

	//Create("TABLA1","SC",3,5000);

	//if(pthread_create(&hiloRecibirDeMemoria, NULL, (void*)recibirConexionesMemoria, NULL)){

	//}

	//leerDeConsola();

	//pthread_join(hiloRecibirDeMemoria, NULL);
	

	//free(pathRaiz);
	log_destroy(logger_LFS);
	config_destroy(config);
	return 0;
}

char** separarRequest(char* request) {
	// La idea es concatenar cada char hasta que haya un espacio,
	// ahi lo guardo en una posicion de requestSeparada
	// Si aparece una comilla activo el flag comilla
	// y concateno aunque haya espacios hasta que el flag comilla se desactive
	char** requestSeparada = NULL;
	int comilla = 0;
	int j=0;
	char* elementoConcatenado = strdup("");
	char* elemento;

	for(int i=0;request[i]!='\0';i++) {
		elemento = malloc(2*sizeof(char));
		elemento[0] = request[i];
		elemento[1] = '\0';
		if((request[i]==34) && comilla) { // 34 es el valor ascii de la comilla
			comilla = 0;
		} else if(request[i]==34) {
			comilla = 1;
		}
		if((request[i]==32) && !comilla) { // 32 es el valor ascii del espacio
			requestSeparada[j] = strdup(elementoConcatenado);
			free(elementoConcatenado);
			j++;
		} else {
			elementoConcatenado = concatenar(elementoConcatenado,elemento,NULL);
		}
		free(elemento);
	}
	free(elementoConcatenado);
	return requestSeparada;
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
		interpretarRequest(palabraReservada, paqueteRecibido->request, ANOTHER_COMPONENT);
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
			if(caller == ANOTHER_COMPONENT){
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

	char* pathTabla = concatenar(pathRaiz, "Tablas/", nombreTabla, NULL);

	//TODO PASAR NOMBRE DE TABLA A MAYUSCULA


	/* Validamos si la tabla existe */
	DIR *dir = opendir(pathTabla);
	//TODO ver si chequear si la tabla existe asi, o usando mkdir
	if(!dir){
		/* Creamos la carpeta de la tabla */
		mkdir(pathTabla, S_IRWXU);
		char* metadataPath = concatenar(pathTabla, "/Metadata.bin", NULL);

		/* Creamos el archivo Metadata */
		int metadataFileDescriptor = open(metadataPath, O_CREAT , S_IRWXU);
		close(metadataFileDescriptor);

		t_config *metadataConfig = config_create(metadataPath);
		config_set_value(metadataConfig, "CONSISTENCY", tipoDeConsistencia);
		config_set_value(metadataConfig, "PARTITIONS", string_itoa(numeroDeParticiones));
		config_set_value(metadataConfig, "COMPACTION_TIME", string_itoa(tiempoDeCompactacion));
		config_save(metadataConfig);

		/* Creamos las particiones */
		for(int i = 0; i < numeroDeParticiones; i++) {
			char* pathParticion = concatenar(pathTabla, "/", string_itoa(i), ".bin", NULL);

			int particionFileDescriptor = open(pathParticion, O_CREAT ,S_IRWXU);
			close(particionFileDescriptor);

			t_config *configParticion = config_create(pathParticion);
			config_set_value(configParticion, "SIZE", "0");
			config_set_value(configParticion, "BLOCKS", string_itoa(obtenerBloqueDisponible()));
			config_save(configParticion);
			config_destroy(configParticion);
			free(pathParticion);
		}

		config_destroy(metadataConfig);
		free(metadataPath);
	} else {
		//TODO existe tabla
	}
	free(pathTabla);
}

int obtenerBloqueDisponible() {
	return 1;
}

void inicializarLfs() {
	pathRaiz = concatenar(PATH, config_get_string_value(config, "PUNTO_MONTAJE"), NULL);
	mkdir(pathRaiz, S_IRWXU);

	char* pathTablas = concatenar(pathRaiz, "Tablas", NULL);
	char* pathMetadata = concatenar(pathRaiz, "Metadata", NULL);
	char* pathBloques = concatenar(pathRaiz, "Bloques", NULL);

	mkdir(pathTablas, S_IRWXU);

	mkdir(pathMetadata, S_IRWXU);
	pathMetadata = concatenar(pathMetadata, "/Metadata.bin", NULL);

	int metadataDescriptor = open(pathMetadata, O_CREAT ,S_IRWXU);
	close(metadataDescriptor);

	t_config *configMetadata = config_create(pathMetadata);
	config_set_value(configMetadata, "BLOCK_SIZE", "64");
	config_set_value(configMetadata, "BLOCKS", "10");
	config_set_value(configMetadata, "MAGIC_NUMBER", "LISSANDRA");
	config_save(configMetadata);

	mkdir(pathBloques, S_IRWXU);

	int blocks = config_get_int_value(configMetadata, "BLOCKS");
	for(int i = 1; i <= blocks; i++){
		int bloqueFileDescriptor = open(concatenar(pathBloques, "/", string_itoa(i), ".bin", NULL), O_CREAT ,S_IRWXU);
		close(bloqueFileDescriptor);
	}

	config_destroy(configMetadata);
	free(pathTablas);
	free(pathMetadata);
	free(pathBloques);
}

/* insert() [API]
 * Parametros:
 * 	-> nombreTabla :: char*
 * 	-> key :: uint16_t
 * 	-> value :: char*
 * 	-> timestamp :: unsigned long long
 * Descripcion: permite la creacion y/o actualizacion del valor de una key dentro de una tabla
 * Return:  */
void insert(char* nombreTabla, uint16_t key, char* value, unsigned long long timestamp) {
	char* pathTabla = concatenar(PATH, pathRaiz, "/Tablas/", nombreTabla, NULL);

	/* Validamos si la tabla existe */
	DIR *dir = opendir(pathTabla);
	if(dir) {
		char* metadataPath = concatenar(pathTabla, "/Metadata.bin", NULL);
		t_config *metadataConfig = config_create(metadataPath);
		char* tipoDeConsistencia = config_get_string_value(metadataConfig, "CONSISTENCY");
		int numeroDeParticiones = config_get_int_value(metadataConfig, "PARTITIONS");
		int tiempoDeCompactacion = config_get_int_value(metadataConfig, "COMPACTION_TIME");



	} else {
		// TODO: no existe tabla
	}
}
