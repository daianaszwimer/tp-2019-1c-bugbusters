#include "lfs.h"

int main(void){
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/lfs/lfs.config");
	logger_LFS = log_create("lfs.log", "Lfs", 1, LOG_LEVEL_DEBUG);
	log_info(logger_LFS, "----------------INICIO DE LISSANDRA FS--------------");

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
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	while(1){
		int memoria_fd = esperar_cliente(lissandraFS_fd);
		if(memoria_fd > 0) {

			printf("se conecto con fd %d", memoria_fd);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			int threadRequest = pthread_create(&hiloRequest, &attr, (void*)procesarRequest, (int *)memoria_fd);
			if(threadRequest == 0){
				pthread_attr_destroy(&attr);
			} else {
				printf("Error al iniciar el hilo de la memoria con fd = %i", memoria_fd);
			}
		}

		//hiloRequest = malloc(sizeof(pthread_t));
	}
}


void procesarRequest(int memoria_fd){
	while(1){
		t_paquete* paqueteRecibido = recibir(memoria_fd);
		cod_request palabraReservada = paqueteRecibido->palabraReservada;
		printf("El codigo que recibi de Memoria es: %d \n", palabraReservada);
		if (palabraReservada == -1) break;
		enviar(palabraReservada, paqueteRecibido->request ,memoria_fd);
	}
}
