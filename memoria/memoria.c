#include "memoria.h"
#include <nuestro_lib/nuestro_lib.h>

int main(void)
{

	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	int server_fd = iniciar_servidor(); //ver shared libraries para no tener quilombos con eclipse
	log_info(logger, "Servidor listo para recibir al cliente");
	int cliente_fd = esperar_cliente(server_fd);

	t_list* lista;
	while(1) {
		int cod_op = recibir_operacion(cliente_fd);
		switch(cod_op)
		{
			int cod_op = recibir_operacion(cliente_fd);
			switch(cod_op)
			{
			case SELECT:
				recibir_mensaje(cliente_fd);
				break;
			case INSERT:
				lista = recibir_paquete(cliente_fd);
				printf("Me llegaron los siguientes valores:\n");
				list_iterate(lista, (void*) iterator);
				//char* msg = string_repeat('f', 3);
				t_paquete* paquete = armar_paquete();
				enviar_paquete(paquete, cliente_fd);
				eliminar_paquete(paquete);
				//enviar_mensaje("123", cliente_fd);
				break;
			case -1:
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				return EXIT_FAILURE;
			default:
				log_warning(logger, "Operacion desconocida. No quieras meter la pata");
				break;
			}
	}

	return EXIT_SUCCESS;
}
