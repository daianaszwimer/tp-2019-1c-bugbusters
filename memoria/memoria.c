#include "memoria.h"

int main(void) {
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/memoria/memoria.config");
	logger_MEMORIA = log_create("memoria.log", "Memoria", 1, LOG_LEVEL_DEBUG);
	datoEstaEnCache = FALSE;
	//conectar con file system
	conectarAFileSystem();

	//	SEMAFOROS
	sem_init(&semLeerDeConsola, 0, 1);
	sem_init(&semEnviarMensajeAFileSystem, 0, 0);

	// 	HILOS
	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);
	pthread_create(&hiloEscucharMultiplesClientes, NULL, (void*)escucharMultiplesClientes, NULL);
	pthread_create(&hiloEnviarMensajeAFileSystem, NULL, (void*)enviarMensajeAFileSystem, NULL);


	pthread_join(hiloLeerDeConsola, NULL);
	pthread_join(hiloEscucharMultiplesClientes, NULL);
	pthread_join(hiloEnviarMensajeAFileSystem, NULL);

	liberar_conexion(conexionLfs);
	log_destroy(logger_MEMORIA);
	config_destroy(config);

	//momentaneamente, asi cerramos todas las conexiones
	FD_ZERO(&descriptoresDeInteres);
	return 0;
}

void leerDeConsola(void){
	//log_info(logger, "leerDeConsola");
	while (1) {
		sem_wait(&semLeerDeConsola);
		mensaje = readline(">");
		if (!strcmp(mensaje, "\0")) {
			sem_post(&semEnviarMensajeAFileSystem);
			break;
		}
		codValidacion = validarMensaje(mensaje, KERNEL, logger_MEMORIA);
		sem_post(&semEnviarMensajeAFileSystem);
	}
}

void conectarAFileSystem() {
	//while
	conexionLfs = crearConexion(
			config_get_string_value(config, "IP_LFS"),
			config_get_string_value(config, "PUERTO_LFS"));
}

void escucharMultiplesClientes() {
	int descriptorServidor = iniciar_servidor(config_get_string_value(config, "PUERTO"), config_get_string_value(config, "IP"));
	log_info(logger_MEMORIA, "Memoria lista para recibir al kernel");

	/* fd = file descriptor (id de Socket)
	 * fd_set es Set de fd's (una coleccion)*/

	descriptoresClientes = list_create();	// Lista de descriptores de todos los clientes conectados (podemos conectar infinitos clientes)
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
				char* request = paqueteRecibido->request;
				printf("El codigo que recibi es: %d \n", palabraReservada);
				interpretarRequest(palabraReservada,request, i);
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
}

void interpretarRequest(int palabraReservada,char* request, int i) {
	switch(palabraReservada) {
		case SELECT:
			log_info(logger_MEMORIA, "Me llego un SELECT");
			procesarSelect(palabraReservada, request);
			break;
		case INSERT:
			log_info(logger_MEMORIA, "Me llego un INSERT");
			procesarInsert(palabraReservada, request);
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
	log_info(logger_MEMORIA, "(MENSAJE DE KERNEL)");
}

/*procesarInsert()
 * Parametros:
 * 	-> char* ::  request
 * 	-> cod_request :: palabraReservada
 * Descripcion:
 * Return:
 * 	-> :: void */
void enviarMensajeAFileSystem(void){
	int codRequest; // es la palabra reservada (ej: SELECT)
	char** request;
	t_paquete* paquete;
	t_paquete* paqueteRecibido;

	while (1) {
		sem_wait(&semEnviarMensajeAFileSystem);
		if (!strcmp(mensaje, "\0")) {
			break;
		}
		printf("El mensaje es: %s \n", mensaje);
		printf("Codigo de validacion: %d \n", codValidacion);
		if (codValidacion != EXIT_FAILURE && codValidacion != QUERY_ERROR) {
			request = separarString(mensaje);
			codRequest = obtenerCodigoPalabraReservada(request[0], KERNEL);
			// El paquete tiene el cod_request y UN request completo
			paquete = armar_paquete(codRequest, mensaje);
			printf("Voy a enviar este cod: %d \n", paquete->palabraReservada);
			log_info(logger_MEMORIA, "Antes de enviar mensaje");
			enviar(paquete, conexionLfs);
			free(paquete);
			log_info(logger_MEMORIA, "despues de enviar mensaje");
		}
		free(mensaje);
		//config_destroy(config);
		sem_post(&semLeerDeConsola);
		paqueteRecibido = recibir(conexionLfs);
		int palabraReservada = paqueteRecibido->palabraReservada;
		log_info(logger_MEMORIA, "Me respuesta, del SELECT, de LFS");
		printf("El codigo que recibi de LFS es: %s \n", (char*) paqueteRecibido->request);

	}
}

void procesarSelect(char* request,cod_request palabraReservada) {
	if(datoEstaEnCache == TRUE) {
		//lo busco entre mis datos
		//le respondo a kernel
	} else {
		//mando request a lfs
		//y recibo la rta y se la mando a kernel y espero la rta de lfs
		t_paquete* paquete = armar_paquete(palabraReservada, request);
		enviar(paquete, conexionLfs);

	}
}

/*procesarInsert()
 * Parametros:
 * 	-> char* ::  request
 * 	-> cod_request :: palabraReservada
 * Descripcion: se va a fijar si existe el segmento de la tabla ,que se quiere hacer insert,
 * 				en la memoria principal.
 * 				Si Existe dicho segmento, busca la key (y de encontrarla,
 * 				actualiza su valor insertando el timestap actual).Y si no encuentra, solicita pag
 * 				y la crea. Pero de no haber pag suficientes, se hace journaling.
 * 				Si no se encuentra el segmento,solicita un segment para crearlo y lo hace.Y, en
 * Return:
 * 	-> :: void */
void procesarInsert(char* request,cod_request palabraReservada) {

}

