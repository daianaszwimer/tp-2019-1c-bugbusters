#include "memoria.h"

int main(void)
{
	t_log* logger = log_create("memoria.log", "Memoria", 1, LOG_LEVEL_DEBUG);
	int server_fd = iniciar_servidor();
	log_info(logger, "Servidor listo para recibir al cliente");
	int cliente_fd = esperar_cliente(server_fd);

	//t_list* lista;
	t_paquete* paqueteRecibido = recibir(cliente_fd);
	int palabraReservada = paqueteRecibido->palabraReservada;
	//char* primerParametro= request[1];
	//printf("EL PRIMER PARAMETRO ES: %s \n", request[1]);
	//printf("Y este es el msj: %s \n", (char*) paqueteRecibido->request);
	char* request = (char*) paqueteRecibido->request;


	printf("El codigo que recibi es: %d \n", palabraReservada);
	switch(palabraReservada) {
		case SELECT:
			log_info(logger, "Me llego un SELECT");
			break;
		case INSERT:
			log_info(logger, "Me llego un INSERT");
			break;
		case -1:
			log_error(logger, "el cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
	}

	return EXIT_SUCCESS;
}
