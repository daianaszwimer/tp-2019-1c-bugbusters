#include "lfs.h"
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdlib.h>

int main(void) {
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/lfs/lfs.config");
	logger_LFS = log_create("lfs.log", "Lfs", 1, LOG_LEVEL_DEBUG);
	log_info(logger_LFS,
			"----------------INICIO DE LISSANDRA FS--------------");

	inicializarLfs();
	create("TABLA1", "SC", 3, 5000);
	/*
	 pthread_create(&hiloRecibirDeMemoria, NULL, (void*)recibirConexionMemoria, NULL);

	 leerDeConsola();

	 pthread_join(hiloRecibirDeMemoria, NULL);
	 */

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

void recibirConexionMemoria() {
	int lissandraFS_fd = iniciar_servidor(
			config_get_string_value(config, "PUERTO"),
			config_get_string_value(config, "IP"));
	log_info(logger_LFS, "Lissandra lista para recibir a Memoria");

	/* fd = file descriptor (id de Socket)
	 * fd_set es Set de fd's (una coleccion)*/

	descriptoresClientes = list_create();// Lista de descriptores de todos los clientes conectados (podemos conectar infinitos clientes)
	int numeroDeClientes = 0;				// Cantidad de clientes conectados
	int valorMaximo = 0;// Descriptor cuyo valor es el mas grande (para pasarselo como parametro al select)
	t_paquete* paqueteRecibido;

	while (1) {

		eliminarClientesCerrados(descriptoresClientes, &numeroDeClientes);// Se eliminan los clientes que tengan un -1 en su fd
		FD_ZERO(&descriptoresDeInteres); // Inicializamos descriptoresDeInteres
		FD_SET(lissandraFS_fd, &descriptoresDeInteres);	// Agregamos el descriptorServidor a la lista de interes

		for (int i = 0; i < numeroDeClientes; i++) {
			FD_SET((int ) list_get(descriptoresClientes, i),
					&descriptoresDeInteres); // Agregamos a la lista de interes, los descriptores de los clientes
		}

		valorMaximo = maximo(descriptoresClientes, lissandraFS_fd,
				numeroDeClientes); // Se el valor del descriptor mas grande. Si no hay ningun cliente, devuelve el fd del servidor
		select(valorMaximo + 1, &descriptoresDeInteres, NULL, NULL, NULL); // Espera hasta que algún cliente tenga algo que decir

		for (int i = 0; i < numeroDeClientes; i++) {
			if (FD_ISSET((int ) list_get(descriptoresClientes, i),
					&descriptoresDeInteres)) { // Se comprueba si algún cliente ya conectado mando algo
				paqueteRecibido = recibir(
						(int) list_get(descriptoresClientes, i)); // Recibo de ese cliente en particular
				int palabraReservada = paqueteRecibido->palabraReservada;
				char* request = paqueteRecibido->request;
				printf("El codigo que recibi es: %d \n", palabraReservada);
				interpretarRequest(palabraReservada, request, i);
				//eliminar_paquete(paqueteRecibido);
				printf("Del cliente nro: %d \n \n",
						(int) list_get(descriptoresClientes, i)); // Muestro por pantalla el fd del cliente del que recibi el mensaje
			}
		}

		if (FD_ISSET(lissandraFS_fd, &descriptoresDeInteres)) {
			int descriptorCliente = esperar_cliente(lissandraFS_fd); // Se comprueba si algun cliente nuevo se quiere conectar
			numeroDeClientes = (int) list_add(descriptoresClientes,
					(int) descriptorCliente); // Agrego el fd del cliente a la lista de fd's
			numeroDeClientes++;
		}
	}
	//log_destroy(logger_MEMORIA);
	//config_destroy(config);
}

void interpretarRequest(int palabraReservada, char* request, int i) {
	switch (palabraReservada) {
	case SELECT:
		log_info(logger_LFS, "Me llego un SELECT");
		//procesarSelect(palabraReservada, request);
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
	case JOURNAL:
		log_info(logger_LFS, "Me llego un JOURNAL");
		break;
	case -1:
		log_error(logger_LFS, "el cliente se desconecto. Terminando servidor");
		int valorAnterior = (int) list_replace(descriptoresClientes, i, -1); // Si el cliente se desconecta le pongo un -1 en su fd
		break;
	default:
		log_warning(logger_LFS,
				"Operacion desconocida. No quieras meter la pata");
		break;
	}
	log_info(logger_LFS, "(MENSAJE DE MEMORIA)");
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

	metadata = concatenar(metadata, "Metadata.bin", NULL);

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
