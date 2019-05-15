#include "lfs.h"
#include <errno.h>
#include <assert.h>

int main(void){
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/lfs/lfs.config");
	logger_LFS = log_create("lfs.log", "Lfs", 1, LOG_LEVEL_DEBUG);
	log_info(logger_LFS, "----------------INICIO DE LISSANDRA FS--------------");
	//create("TABLA1", "SC", 3, 5000);

	pthread_create(&hiloRecibirDeMemoria, NULL, (void*)recibirConexionesMemoria, NULL);

	leerDeConsola();

	pthread_join(hiloRecibirDeMemoria, NULL);


	return EXIT_SUCCESS;
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
	char* raiz = config_get_string_value(config, "PUNTO_MONTAJE");
	char* pathTabla = strdup(raiz);
	strcat(pathTabla, "tables/");
	strcat(pathTabla, nombreTabla);
	strcat(pathTabla, "/");
	if(fopen(pathTabla, "r") != NULL) {		// Verificamos si la tabla existe
		char* log = "Ya existe una tabla de nombre: ";
		strcat(log, nombreTabla);
		log_error(logger_LFS, log);
	}
	else {
		/* Creamos el archivo Metadata */
		char* pathMetadata = strdup("/home/utnso/tp-2019-1c-bugbusters/lfs");
		strcat(pathMetadata, pathTabla);

		int error = mkdir_p(pathMetadata);
		if (error == 0){
			strcat(pathMetadata, "Metadata.bin");

			FILE* metadata = fopen("Metadata.bin", "w+");
			char* consistency = "CONSISTENCY";
			fwrite(&consistency, sizeof(consistency), sizeof(char), metadata);
			fclose(metadata);

			//t_config* metadata = config_create("Metadata.config");



			/*sprintf(numeroDeParticiones, "PARTITIONS=%i", _numeroDeParticiones);
			sprintf(tiempoDeCompactacion, "COMPACTION_TIME=%i", _tiempoDeCompactacion);*/

			/*config_set_value(metadata, "CONSISTENCY", tipoDeConsistencia);
			config_set_value(metadata, "PARTITIONS", numeroDeParticiones);
			config_set_value(metadata, "COMPACTION_TIME", tiempoDeCompactacion);*/
			int x = config_save(metadata);

			/* Creamos las particiones */
			for(int i = 0; i < numeroDeParticiones; i++) {
				char* pathParticion = strdup(pathTabla);
				strcat(pathParticion, i);
				strcat(pathParticion, ".bin");
				FILE* particion = fopen(pathParticion, "w+");
				char* tamanio = "Size=0";
				char* bloques;
				sprintf(bloques,"Block=[%i]", obtenerBloqueDisponible()); // Hay que agregar un bloque por particion, no se si uno cualquiera o que
				fwrite(&tamanio, sizeof(tamanio), 1, particion);
				fwrite(&bloques, sizeof(bloques), 1, particion);
				free(pathParticion);
			}
		}

	}
	free(pathTabla);
}

int obtenerBloqueDisponible(){
	return 1;
}

int mkdir_p(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
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

