#include "memoria.h"

int main(void) {
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/memoria/memoria.config");
	logger_MEMORIA = log_create("memoria.log", "Memoria", 1, LOG_LEVEL_DEBUG);

	//conectar con file system
	conectarConFileSystem();

	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);
	pthread_create(&hiloEscucharMultiplesClientes, NULL, (void*)escucharMultiplesClientes, NULL);

	pthread_join(hiloLeerDeConsola, NULL);
	pthread_join(hiloEscucharMultiplesClientes, NULL);

	return 0;
}

void leerDeConsola(void){
	//log_info(logger, "LeerDeConsola");
	while (1) {
		mensaje = readline(">");
		if (!strcmp(mensaje, "\0")) {
			break;
		}
		log_info(logger_MEMORIA, mensaje);
		free(mensaje);
	}
}

void escucharMultiplesClientes(){
	/* fd = file descriptor (id de Socket)
	 * fd_set es Set de fd's (una coleccion)*/

	//conectar con kernel
	int descriptorServidor = iniciar_servidor(config_get_string_value(config, "PUERTO"), config_get_string_value(config, "IP"));
	log_info(logger_MEMORIA, "Memoria lista para recibir al kernel");


	t_list* descriptoresClientes = list_create();	// Lista de descriptores de todos los clientes conectados (podemos conectar infinitos clientes)
	fd_set descriptoresDeInteres;					// Coleccion de descriptores de interes para select
	int numeroDeClientes = 0;						// Cantidad de clientes conectados
	int valorMaximo = 0;							// Descriptor cuyo valor es el mas grande (para pasarselo como parametro al select)
	t_paquete* paqueteRecibido;

	while(1) {

		eliminarClientesCerrados(descriptoresClientes, &numeroDeClientes);	// Se eliminan los clientes que tengan un -1 en su fd
		FD_ZERO(&descriptoresDeInteres); 									// Inicializamos descriptoresDeInteres
		FD_SET(descriptorServidor, &descriptoresDeInteres);					// Agregamos el descriptorServidor a la lista de interes

		for (int i=0; i< numeroDeClientes; i++) {
			FD_SET((int) list_get(descriptoresClientes,i), &descriptoresDeInteres); // Agregamos a la lista de interes, los descriptores de los clientes
		}

		valorMaximo = maximo(descriptoresClientes, descriptorServidor, numeroDeClientes); // Se el valor del descriptor mas grande. Si no hay ningun cliente, devuelve el fd del servidor
		select(valorMaximo + 1, &descriptoresDeInteres, NULL, NULL, NULL); 				  // Espera hasta que algún cliente tenga algo que decir

		for(int i=0; i<numeroDeClientes; i++) {
			if (FD_ISSET((int) list_get(descriptoresClientes,i), &descriptoresDeInteres)) {   // Se comprueba si algún cliente ya conectado mando algo
				paqueteRecibido = recibir((int) list_get(descriptoresClientes,i)); // Recibo de ese cliente en particular
				int palabraReservada = paqueteRecibido->palabraReservada;
				printf("El codigo que recibi es: %d \n", palabraReservada);

				switch(palabraReservada) {
					case SELECT:
						log_info(logger_MEMORIA, "Me llego un SELECT");
						break;
					case INSERT:
						log_info(logger_MEMORIA, "Me llego un INSERT");
						break;
					case CREATE:
						log_info(logger_MEMORIA, "Me llego un CREATE");
						break;
					case DESCRIBE:
						log_info(logger_MEMORIA, "Me llego un DESCRIBE");
						break;
					case DROP:
						log_info(logger_MEMORIA, "Me llego un DROP");
						break;
					case JOURNAL:
						log_info(logger_MEMORIA, "Me llego un JOURNAL");
						break;
					case -1:
						log_error(logger_MEMORIA, "el cliente se desconecto. Terminando servidor");
						int valorAnterior = (int) list_replace(descriptoresClientes, i, -1); // Si el cliente se desconecta le pongo un -1 en su fd
						break;
					default:
						log_warning(logger_MEMORIA, "Operacion desconocida. No quieras meter la pata");
						break;
				}
				//eliminar_paquete(paqueteRecibido);
				printf("Del cliente nro: %d \n \n", (int) list_get(descriptoresClientes,i)); // Muestro por pantalla el fd del cliente del que recibi el mensaje
			}
		}

		if(FD_ISSET (descriptorServidor, &descriptoresDeInteres)) {
			int descriptorCliente = esperar_cliente(descriptorServidor); 					  // Se comprueba si algun cliente nuevo se quiere conectar
			numeroDeClientes = (int) list_add(descriptoresClientes, (int) descriptorCliente); // Agrego el fd del cliente a la lista de fd's
			numeroDeClientes++;
		}
	}
	log_destroy(logger_MEMORIA);
	config_destroy(config);
}

void conectarConFileSystem() {
	int conexionLfs = crearConexion(
			config_get_string_value(config, "IP_LFS"),
			config_get_string_value(config, "PUERTO_LFS"));
	liberar_conexion(conexionLfs);

}

