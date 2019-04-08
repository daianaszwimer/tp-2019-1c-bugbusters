#include "kernel.h"


int main(void) {
	logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	log_info(logger, "----------------INICIO DE KERNEL--------------");

	conectarAMemoria();
	escucharAMemoria();

	return EXIT_SUCCESS;
}

void conectarAMemoria(){
	t_log* logger = iniciar_logger();
	log_info(logger, "Soy un log");

	t_config* config = leer_config();
	char* valor = config_get_string_value(config, "CLAVE");
	log_info(logger, valor);

	leer_consola(logger);

	int conexion = crearConexion(
			config_get_string_value(config, "IP"),
			config_get_string_value(config, "PUERTO")
	);

	t_paquete* paquete = armar_paquete();

	enviar_paquete(paquete, conexion);

	//-----------------------
	eliminar_paquete(paquete);
	log_destroy(logger);
	config_destroy(config);
	close(conexion);
}

void escucharAMemoria() {
	log_info(logger, "Conectandonos con memoria...");
	int socketMemoria = iniciar_servidor();
	log_info(logger, "Servidor listo para recibir al cliente");
	int cliente_fd = esperar_cliente(socketMemoria);
	interpretarMensajes(cliente_fd);
}

void interpretarMensajes(int cliente_fd) {

	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(cliente_fd);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(cliente_fd);
			break;
		case PAQUETE:
			lista = recibir_paquete(cliente_fd);
			printf("Me llegaron los siguientes valores:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			log_error(logger, "el cliente se desconecto. Terminando servidor");
			exit(EXIT_FAILURE);
		default:
			log_warning(logger,
					"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
}

void iterator(char* value) {
	printf("%s\n", value);
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
