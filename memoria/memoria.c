#include "memoria.h"

t_log* iniciar_logger();
t_config* leer_config();
void leer_consola(t_log*);
t_paquete* armar_paquete();
void _leer_consola_haciendo(void (*accion)(char*));

int main(void)
{
	void iterator(char* value) {
		printf("%s\n", value);
	}

	logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	int server_fd = iniciar_servidor(); //ver shared libraries para no tener quilombos con eclipse
	log_info(logger, "Servidor listo para recibir al cliente");
	int cliente_fd = esperar_cliente(server_fd);

	t_list* lista;
	while(1)
		{
			int cod_op = recibir_operacion(cliente_fd);
			switch(cod_op)
			{
			case MENSAJE:
				recibir_mensaje(cliente_fd);
				break;
			case PAQUETE:
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


t_log* iniciar_logger() {
	return log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
}

t_config* leer_config() {
	return config_create("kernel.config");
}

void leer_consola(t_log* logger) {
	void loggear(char* leido) {
		log_info(logger, leido);
	}

	_leer_consola_haciendo((void*) loggear);
}

t_paquete* armar_paquete() {
	t_paquete* paquete = crear_paquete();

	void _agregar(char* leido) {
		// Estamos aprovechando que podemos acceder al paquete
		// de la funcion exterior y podemos agregarle lineas!
		agregar_a_paquete(paquete, leido, strlen(leido) + 1);
	}

	_leer_consola_haciendo((void*) _agregar);

	return paquete;
}
//								  requests
void _leer_consola_haciendo(void (*accion)(char*)) {
	char* leido = readline(">");

	while (strncmp(leido, "", 1) != 0) {
		accion(leido);
		free(leido);
		leido = readline(">");
	}

	free(leido);
}

