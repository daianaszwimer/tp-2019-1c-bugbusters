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

	while(1){
		int memoria_fd = esperar_cliente(lissandraFS_fd);
		hiloRequest = malloc(sizeof(pthread_t));
		if(pthread_create(&hiloRequest, NULL, (void*)procesarRequest, memoria_fd)){
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
