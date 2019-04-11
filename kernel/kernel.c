#include "kernel.h"


int main(void) {

	logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	log_info(logger, "----------------INICIO DE KERNEL--------------");
	conectarAMemoria();
	return EXIT_SUCCESS;
}

void conectarAMemoria(){
	void iterator(char* value) {
					printf("%s\n", value);
				}
	t_log* logger = iniciar_logger();
	//log_info(logger, "Soy un log");

	t_config* config = leer_config();
	//char* valor = config_get_string_value(config, "CLAVE");
	//log_info(logger, valor);

	leer_consola(logger);

	int conexion = crearConexion(
			config_get_string_value(config, "IP"),
			config_get_string_value(config, "PUERTO")
	);

	t_paquete* paquete = armar_paquete();

	enviar_paquete(paquete, conexion);
	eliminar_paquete(paquete);

	recibir_mensaje(conexion);

	//-----------------------

	log_destroy(logger);
	config_destroy(config);		//Preguntar
	close(conexion);

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
