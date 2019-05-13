#include "lfs.h"
#include <errno.h>
#include <assert.h>
#include <stdarg.h>


int main(void){
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/lfs/lfs.config");
	logger_LFS = log_create("lfs.log", "Lfs", 1, LOG_LEVEL_DEBUG);
	log_info(logger_LFS, "----------------INICIO DE LISSANDRA FS--------------");



	inicializarLfs();

	create("TABLA1", "SC", 3, 5000);
/*
	pthread_create(&hiloRecibirDeMemoria, NULL, (void*)recibirConexionMemoria, NULL);

	leerDeConsola();

	pthread_join(hiloRecibirDeMemoria, NULL);
	 */

	free(raiz);
	return 0;
}

void leerDeConsola(void){
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
	int lissandraFS_fd = iniciar_servidor(config_get_string_value(config, "PUERTO"), config_get_string_value(config, "IP"));
	log_info(logger_LFS, "Lissandra lista para recibir a Memoria");

	/* fd = file descriptor (id de Socket)
	* fd_set es Set de fd's (una coleccion)*/

	descriptoresClientes = list_create();	// Lista de descriptores de todos los clientes conectados (podemos conectar infinitos clientes)
	int numeroDeClientes = 0;				// Cantidad de clientes conectados
	int valorMaximo = 0;					// Descriptor cuyo valor es el mas grande (para pasarselo como parametro al select)
	t_paquete* paqueteRecibido;

	while(1) {

		eliminarClientesCerrados(descriptoresClientes, &numeroDeClientes);	// Se eliminan los clientes que tengan un -1 en su fd
		FD_ZERO(&descriptoresDeInteres); 									// Inicializamos descriptoresDeInteres
		FD_SET(lissandraFS_fd, &descriptoresDeInteres);						// Agregamos el descriptorServidor a la lista de interes

		for (int i=0; i< numeroDeClientes; i++) {
			FD_SET((int) list_get(descriptoresClientes,i), &descriptoresDeInteres); // Agregamos a la lista de interes, los descriptores de los clientes
		}

		valorMaximo = maximo(descriptoresClientes, lissandraFS_fd, numeroDeClientes); // Se el valor del descriptor mas grande. Si no hay ningun cliente, devuelve el fd del servidor
		select(valorMaximo + 1, &descriptoresDeInteres, NULL, NULL, NULL); 		  	  // Espera hasta que algún cliente tenga algo que decir

		for(int i=0; i<numeroDeClientes; i++) {
			if (FD_ISSET((int) list_get(descriptoresClientes,i), &descriptoresDeInteres)) {   // Se comprueba si algún cliente ya conectado mando algo
				paqueteRecibido = recibir((int) list_get(descriptoresClientes,i)); 			  // Recibo de ese cliente en particular
				int palabraReservada = paqueteRecibido->palabraReservada;
				char* request = paqueteRecibido->request;
				printf("El codigo que recibi es: %d \n", palabraReservada);
				interpretarRequest(palabraReservada,request, i);
				//eliminar_paquete(paqueteRecibido);
				printf("Del cliente nro: %d \n \n", (int) list_get(descriptoresClientes,i)); // Muestro por pantalla el fd del cliente del que recibi el mensaje
			}
		}

		if(FD_ISSET (lissandraFS_fd, &descriptoresDeInteres)) {
			int descriptorCliente = esperar_cliente(lissandraFS_fd); 					  	  // Se comprueba si algun cliente nuevo se quiere conectar
			numeroDeClientes = (int) list_add(descriptoresClientes, (int) descriptorCliente); // Agrego el fd del cliente a la lista de fd's
			numeroDeClientes++;
		}
	}
	//log_destroy(logger_MEMORIA);
	//config_destroy(config);
}

void interpretarRequest(int palabraReservada,char* request, int i) {
	switch(palabraReservada) {
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
			log_warning(logger_LFS, "Operacion desconocida. No quieras meter la pata");
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

	size_t sizeOfPathTable = snprintf(NULL, 0, "%s%s%s%s", PATH ,raiz, "/Tablas/", nombreTabla) + 1;
	char* pathTabla = (char*) malloc(sizeOfPathTable);
	sprintf(pathTabla, "%s%s%s%s", PATH, raiz, "/Tablas/", nombreTabla);


	/* Creamos el archivo Metadata */

	int error = mkdir_p(pathTabla);
	if (error == 0){

		char* pathMetadata = strdup(pathTabla);
		pathMetadata = (char*) realloc(pathMetadata, strlen(pathTabla) + sizeof(char) * 12);
		strcat(pathMetadata, "/Metadata.bin");

		FILE* metadata = fopen(pathMetadata, "w+");
		if(metadata != NULL) {
			char* consistency = strdup(tipoDeConsistencia);
			fwrite(&tipoDeConsistencia, strlen(consistency), 1, metadata);
			fclose(metadata);
		}
	//free(pathMetadata);
		free(pathTabla);
//		//free(consistency);
//
//		//t_config* metadata = config_create("Metadata.config");
//
//
//
//		/*sprintf(numeroDeParticiones, "PARTITIONS=%i", _numeroDeParticiones);
//		sprintf(tiempoDeCompactacion, "COMPACTION_TIME=%i", _tiempoDeCompactacion);*/
//
//		/*config_set_value(metadata, "CONSISTENCY", tipoDeConsistencia);
//		config_set_value(metadata, "PARTITIONS", numeroDeParticiones);
//		config_set_value(metadata, "COMPACTION_TIME", tiempoDeCompactacion);*/
//		int x = config_save(metadata);
//
//		/* Creamos las particiones */
//		for(int i = 0; i < numeroDeParticiones; i++) {
//			char* pathParticion = strdup(pathTabla);
//			strcat(pathParticion, i);
//			strcat(pathParticion, ".bin");
//			FILE* particion = fopen(pathParticion, "w+");
//			char* tamanio = "Size=0";
//			char* bloques;
//			sprintf(bloques,"Block=[%i]", obtenerBloqueDisponible()); // Hay que agregar un bloque por particion, no se si uno cualquiera o que
//			fwrite(&tamanio, sizeof(tamanio), 1, particion);
//			fwrite(&bloques, sizeof(bloques), 1, particion);
//			free(pathParticion);
//		}
	}
//
//	free(pathTabla);

}

int obtenerBloqueDisponible(){
	return 1;
}

int mkdir_p(const char *path)
{
    const size_t len = strlen(path);
    char _path[256];
    char *p;

    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof(_path)-1) {
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
                if (errno != EEXIST)
                    return -1;
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

void inicializarLfs() {
	raiz = config_get_string_value(config, "PUNTO_MONTAJE");
	char* fs_Lissandra = strdup(PATH);
	fs_Lissandra = (char*) realloc(fs_Lissandra, strlen(fs_Lissandra) + strlen(raiz) );
	strcat(fs_Lissandra, raiz);

	char* tablas = strdup(fs_Lissandra);
	tablas = (char*) realloc(tablas, strlen(fs_Lissandra) + sizeof(char) * 7);
	strcat(tablas, "/Tablas");

	char* metadata = strdup(fs_Lissandra);
	metadata = (char*) realloc(metadata, strlen(fs_Lissandra) + sizeof(char) * 9);
	strcat(metadata, "/Metadata");

	char* bloques = strdup(fs_Lissandra);
	bloques = (char*) realloc(bloques, strlen(fs_Lissandra) + sizeof(char) * 8);
	strcat(bloques, "/Bloques");

	mkdir_p(tablas);
	mkdir_p(metadata);
	mkdir_p(bloques);


	free(fs_Lissandra);
	free(tablas);
	free(metadata);
	free(bloques);
}
