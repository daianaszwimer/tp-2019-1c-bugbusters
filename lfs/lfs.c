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
	create("TABLA1", "SC", 3, 5000);

	/*pthread_create(&hiloRecibirDeMemoria, NULL, (void*)recibirConexionesMemoria, NULL);

	leerDeConsola();

	pthread_join(hiloRecibirDeMemoria, NULL);*/
	
	log_destroy(logger_LFS);
	config_destroy(config);
	return 0;
}

void leerDeConsola(void) {
	//log_info(logger, "leerDeConsola");
	while (1) {
		mensaje = readline(">");
		if (!strcmp(mensaje, "\0")) {
			break;
		}
		codValidacion = validarMensaje(mensaje, KERNEL, logger_LFS);
		printf("El mensaje es: %s \n", mensaje);
		printf("Codigo de validacion: %d \n", codValidacion);
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
		printf("El codigo que recibi de Memoria es: %d \n", palabraReservada);

		//t_paquete* paquete = armar_paquete(palabraReservada, respuesta);
		enviar(palabraReservada, paqueteRecibido->request ,memoria_fd);
	}
}

/* create() [API]
 * Parametros:
 * 	-> nombreTabla :: char*
 * 	-> tipoDeConsistencia :: char*
 * 	-> numeroDeParticiones :: int
 * 	-> tiempoDeCompactacion :: int
 * Descripcion: permite la creación de una nueva tabla dentro del file system
 * Return:  */
void create(char* nombreTabla, char* tipoDeConsistencia, int numeroDeParticiones, int tiempoDeCompactacion) {
	// La carpeta metadata y la de los bloques no se crea aca, se crea apenas levanta el lfs

	char* pathTabla = concatenar(PATH, raiz, "/Tablas/", nombreTabla, NULL);

	/* Creamos el archivo Metadata */
	DIR *dir = opendir(pathTabla);

	if(!dir){
		int error = mkdir_p(pathTabla);
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


int mkdir_p(const char *path) {
	const size_t len = strlen(path);
	char _path[256];
	char *p;

	errno = 0;

	/* Copy string so its mutable */
	if (len > sizeof(_path) - 1) {
		errno = ENAMETOOLONG;
		return -1;
	}
	strcpy(_path, path);

	/* Iterate the string */
	for (p = _path + 1; *p; p++) {
		if (*p == '/') {
			/* Temporarily truncate */
			*p = '\0';

			if (mkdir(_path, S_IRWXU) != 0) {
				if (errno != EEXIST){
					return -1;
				}
			}
			*p = '/';
		}
	}
	if (mkdir(_path, S_IRWXU) != 0) {
		if (errno != EEXIST)
			return -1;
	}
	return 0;
}


//TODO: dado que crear directorio podria retornar si hubo errores o no, catchear esos errores en la creacion de cada directorio.
void inicializarLfs() {
	raiz = config_get_string_value(config, "PUNTO_MONTAJE");

	char* tablas = concatenar(PATH, raiz, "Tablas", NULL);
	char* metadata = concatenar(PATH, raiz, "Metadata", NULL);
	char* bloques = concatenar(PATH, raiz, "Bloques", NULL);

	crearDirectorio(tablas);

	crearDirectorio(metadata);

	metadata = concatenar(metadata, "/Metadata.bin", NULL);

	FILE* metadataFile = fopen(metadata, "w+");
	if(metadataFile != NULL){
		//TODO: Este es el ejemplo que aparece en el TP, ver si despues hay que modificarlo.
		char* blockSize = strdup("BLOCK_SIZE=64");
		char* blocks = strdup("BLOCKS=100");
		char* magicNumber = strdup("MAGIC_NUMBER=LISSANDRA");
		fwrite(&blockSize, strlen(blockSize), 1, metadataFile);
		fwrite(&blocks, strlen(blocks), 1, metadataFile);
		fwrite(&magicNumber, strlen(magicNumber), 1, metadataFile);\

		fclose(metadataFile);

		free(blockSize);
		free(blocks);
		free(magicNumber);
	}else{
		//TODO: no se creo el metadata correctamente
	}

	crearDirectorio(bloques);
	config = leer_config(metadata);

	/*
	 ver si la cantidad de bloques se puede obtener levantando el metadata como config y crear la cantidad de bloques que dice en dicho
	 archivo (chequear si aca es el momento de hacer esto).
	*/

	/*
	int blocks = atoi(config_get_string_value(config, "BLOCKS"));
	for(int i= 1; i<=blocks; i++){

	}*/


	free(tablas);
	free(metadata);
	free(bloques);
}
