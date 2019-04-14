#include "kernel.h"
#include <nuestro_lib/nuestro_lib.h>

int main(void) {

	logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	log_info(logger, "----------------INICIO DE KERNEL--------------");
	conectarAMemoria();
	return EXIT_SUCCESS;
}

void conectarAMemoria(){
	char* mensaje;
	t_log* logger = iniciar_logger();
	//log_info(logger, "Soy un log");

	//t_config* config = leer_config();
	//char* valor = config_get_string_value(config, "CLAVE");
	//log_info(logger, valor);

	//leer_consola(logger);

//	int conexion = crearConexion(
//			config_get_string_value(config, "IP"),
//			config_get_string_value(config, "PUERTO")
//	);

	//t_paquete* paquete = armar_paquete();
	//enviar_paquete(paquete, conexion);
	//eliminar_paquete(paquete);
	scanf("%s", mensaje);
	validarMensaje(mensaje);
	//enviar_mensaje(mensaje ,conexion);

//	t_list* lista;
//
//	while(1)
//			{
//				int cod_op = recibir_operacion(conexion);
//				switch(cod_op)
//				{
//				case MENSAJE:
//					recibir_mensaje(conexion);
//					break;
//				case PAQUETE:
//					lista = recibir_paquete(conexion);
//					printf("Me llegaron los siguientes valores:\n");
//					list_iterate(lista, (void*) iterator);
//					//char* msg = string_repeat('f', 3);
//					t_paquete* paquete = armar_paquete();
//					enviar_paquete(paquete, conexion);
//					eliminar_paquete(paquete);
//					//enviar_mensaje("123", cliente_fd);
//					break;
//				case -1:
//					log_error(logger, "el cliente se desconecto. Terminando servidor");
//					break;
//				default:
//					log_warning(logger, "Operacion desconocida. No quieras meter la pata");
//					break;
//				}
//		}
	//-----------------------

  log_destroy(logger);
//	config_destroy(config);		//Preguntar
//	close(conexion);

}
