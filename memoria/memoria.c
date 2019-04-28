#include "memoria.h"

int main(void) {

	logger = log_create("memoria.log", "Memoria", 1, LOG_LEVEL_DEBUG);
	log_info(logger, "Servidor listo para recibir al cliente");

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
		log_info(logger, mensaje);
		free(mensaje);
	}
}

void escucharMultiplesClientes(){
	/* fd = file descriptor (id de Socket)
	 * fd_set es Set de fd's (una coleccion)*/
	int descriptorServidor = iniciar_servidor();
	t_list* descriptoresClientes = list_create();	// Lista de descriptores de todos los clientes conectados (podemos conectar infinitos clientes)
	fd_set descriptoresDeInteres;					// Coleccion de descriptores de interes para select
	int numeroDeClientes = 0;						// Cantidad de clientes conectados
	int valorMaximo = 0;							// Descriptor cuyo valor es el mas grande (para pasarselo como parametro al select)

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
				t_paquete* paqueteRecibido = recibir((int) list_get(descriptoresClientes,i)); // Recibo de ese cliente en particular
				int palabraReservada = paqueteRecibido->palabraReservada;
				printf("El codigo que recibi es: %d \n", palabraReservada);

				switch(palabraReservada) {
					case SELECT:
						log_info(logger, "Me llego un SELECT");
						break;
					case INSERT:
						log_info(logger, "Me llego un INSERT");
						break;
					case CREATE:
						log_info(logger, "Me llego un CREATE");
						break;
					case DESCRIBE:
						log_info(logger, "Me llego un DESCRIBE");
						break;
					case DROP:
						log_info(logger, "Me llego un DROP");
						break;
					case JOURNAL:
						log_info(logger, "Me llego un JOURNAL");
						break;
					case -1:
						log_error(logger, "el cliente se desconecto. Terminando servidor");
						int valorAnterior = (int) list_replace(descriptoresClientes, i, -1); // Si el cliente se desconecta le pongo un -1 en su fd
						break;
					default:
						log_warning(logger, "Operacion desconocida. No quieras meter la pata");
						break;
				}

				printf("Del cliente nro: %d \n \n", (int) list_get(descriptoresClientes,i)); // Muestro por pantalla el fd del cliente del que recibi el mensaje
			}
		}

		if(FD_ISSET (descriptorServidor, &descriptoresDeInteres)) {
			int descriptorCliente = esperar_cliente(descriptorServidor); 					  // Se comprueba si algun cliente nuevo se quiere conectar
			numeroDeClientes = (int) list_add(descriptoresClientes, (int) descriptorCliente); // Agrego el fd del cliente a la lista de fd's
			numeroDeClientes++;
		}
	}
}
