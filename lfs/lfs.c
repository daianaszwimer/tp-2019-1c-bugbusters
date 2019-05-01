#include "lfs.h"

int main(void){
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/lfs/lfs.config");
	logger_LFS = log_create("lfs.log", "Lfs", 1, LOG_LEVEL_DEBUG);
	log_info(logger_LFS, "----------------INICIO DE LISSANDRA FS--------------");

	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);

	recibirConexionMemoria();

	pthread_join(hiloLeerDeConsola, NULL);


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

void recibirConexionMemoria() {
	int lissandraFS_fd = iniciar_servidor(config_get_string_value(config, "PUERTO"), config_get_string_value(config, "IP"));
	log_info(logger_LFS, "Lissandra lista para recibir a Memoria");

	while(1) {
		int memoria_fd = esperar_cliente(lissandraFS_fd);
		t_paquete* paqueteRecibido = recibir(memoria_fd);
		int palabraReservada = paqueteRecibido->palabraReservada;
		printf("El codigo que recibi de Memoria es: %d \n", palabraReservada);

		char* respuesta = "soy una respuesta de lfss";
		t_paquete* paquete = armar_paquete(palabraReservada, respuesta);
		enviar(paquete, memoria_fd);
	}
}
