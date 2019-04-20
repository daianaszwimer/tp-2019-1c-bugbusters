#include "nuestro_lib.h"

void iterator(char* value) {
	printf("%s\n", value);
}

t_config* leer_config(char* nombreArchivo) {
	return config_create(nombreArchivo);
}
//
//void leer_consola(t_log* logger) {
//	void loggear(char* leido) {
//		log_info(logger, leido);
//	}
//
//	_leer_consola_haciendo((void*) loggear);
//}
//

t_paquete* armar_paquete(cod_request palabraReservada, char** request) {
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->palabraReservada = palabraReservada;
	paquete->tamanio = sizeof(int);
	paquete->request = request;
	return paquete;
}
////								  requests
//void _leer_consola_haciendo(void (*accion)(char*)) {
//	char* leido = readline(">");
//
//	while (strncmp(leido, "", 1) != 0) {
//		accion(leido);
//		free(leido);
//		leido = readline(">");
//	}
//
//	free(leido);
//}
//
//

int iniciar_servidor(void)
{
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(IP, PUERTO, &hints, &servinfo);
    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }
	listen(socket_servidor, SOMAXCONN);
    freeaddrinfo(servinfo);
    //log_trace(logger, "Listo para escuchar a mi cliente");
    //puts("2345678");
    puts("Ya se porque es");
    return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;
	unsigned int tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

	//log_info(logger, "Se conecto un cliente!");

	return socket_cliente;
}

char** separarString(char* mensaje) {
	return string_split(mensaje, " ");
}

int validarMensaje(char* mensaje, Componente componente) {
	char** request = string_n_split(mensaje, 2, " ");
	char** parametros = separarString(request[1]);
	int codPalabraReservada = obtenerCodigoPalabraReservada(request[0], componente);
	int cantidadDeParametros = sizeof(parametros)/sizeof(char*);
	if(validarPalabraReservada(codPalabraReservada, componente) ==0 ) {
		return EXIT_SUCCESS;
	}
	else {

		log_error(logger,"NO FUNACAGYFSDHHFLGYIFHDJJLGFJJFGHBCBH");
		return -1;
	}
}
//&& validarCantidadDeParametros(cantidadDeParametros, request[0],componente)==0

int validarCantidadDeParametros(int cantidadDeParametros, char* palabraReservada, Componente componente) {
	int retorno;
	switch(obtenerCodigoPalabraReservada(palabraReservada, componente)) {
			case SELECT:
				if(cantidadDeParametros == PARAMETROS_SELECT){
					return EXIT_SUCCESS ;
				}else{
					log_error(logger,"tiro error aca sdfcgvhbjn");
					return -1;
				}
				break;
			case INSERT:
				retorno = (cantidadDeParametros == PARAMETROS_INSERT) ? EXIT_SUCCESS : EXIT_FAILURE;
				break;
			case CREATE:
				retorno = (cantidadDeParametros == PARAMETROS_CREATE) ? EXIT_SUCCESS : EXIT_FAILURE;
				break;
			case DESCRIBE:
				retorno = (cantidadDeParametros == PARAMETROS_DESCRIBE) ? EXIT_SUCCESS : EXIT_FAILURE;
				break;
			case DROP:
				retorno = (cantidadDeParametros == PARAMETROS_DROP) ? EXIT_SUCCESS : EXIT_FAILURE;
				break;
			case JOURNAL:
				retorno = (cantidadDeParametros == PARAMETROS_JOURNAL) ? EXIT_SUCCESS : EXIT_FAILURE;
				break;
			case ADD:
				retorno = (cantidadDeParametros == PARAMETROS_ADD) ? EXIT_SUCCESS : EXIT_FAILURE;
				break;
			case RUN:
				retorno = (cantidadDeParametros == PARAMETROS_RUN) ? EXIT_SUCCESS : EXIT_FAILURE;
				break;
			case METRICS:
				retorno = (cantidadDeParametros == PARAMETROS_METRICS) ? EXIT_SUCCESS : EXIT_FAILURE;
				break;
			default:
				log_warning(logger, "Operacion desconocida. No quieras meter la pata");
				break;
	}
	return retorno;
}



int validarPalabraReservada(int codigoPalabraReservada, Componente componente){
	if(codigoPalabraReservada == -1) {
		log_error(logger, "Debe ingresar un request válido");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int obtenerCodigoPalabraReservada(char* palabraReservada, Componente componente) {
	int retorno;
	if (!strcmp(palabraReservada, "SELECT")) {
		retorno = 0;
	}
	else if (!strcmp(palabraReservada, "INSERT")) {
		retorno = 1;
	}
	else if (!strcmp(palabraReservada, "CREATE")) {
		retorno = 2;
	}
	else if (!strcmp(palabraReservada, "DESCRIBE")) {
		retorno = 3;
	}
	else if (!strcmp(palabraReservada, "DROP")) {
		retorno = 4;
	}
	else if (!strcmp(palabraReservada, "JOURNAL")) {
		retorno = (componente == LFS) ? -1 : 5;
	}
	else if (!strcmp(palabraReservada, "ADD")) {
		retorno = (componente == KERNEL) ? 6 : -1;
	}
	else if (!strcmp(palabraReservada, "RUN")) {
		retorno = (componente == KERNEL) ? 7 : -1;
	}
	else if (!strcmp(palabraReservada, "METRICS")) {
		retorno = (componente == KERNEL) ? 8 : -1;
	}
	else {
		retorno = -1;
	}
	return retorno;
}

//void enviar_mensaje(char* mensaje, int socket_cliente)
//{
//	t_paquete* paquete = malloc(sizeof(t_paquete));
//
//	paquete->codigo_operacion = MENSAJE;
//	paquete->buffer = malloc(sizeof(t_buffer));
//	paquete->buffer->size = strlen(mensaje) + 1;
//	paquete->buffer->stream = malloc(paquete->buffer->size);
//	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);
//
//	int bytes = paquete->buffer->size + 2*sizeof(int);
//
//	void* a_enviar = serializar_paquete(paquete, bytes);
//
//	send(socket_cliente, a_enviar, bytes, 0);
//
//	free(a_enviar);
//	eliminar_paquete(paquete);
//}
//
//int recibir_request(int socket_cliente)
//{
//	int cod_request;
//	int size;
//	char* buffer = "";
//	buffer = (char*) recibir_buffer(&size, socket_cliente);
//	//strcat(buffer, "\0");   // Chequear esto
//	//log_info(logger, "Me llego la request %s", buffer);
//	free(buffer);
//	return cod_request;
//
//}
//
////int recibir_operacion(int socket_cliente)
////{
////	int cod_op;
////	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) != 0)
////		return cod_op;
////	else
////	{
////		close(socket_cliente);
////		return -1;
////	}
////}
//
void* recibir_buffer(int* size, int socket_cliente)
{
	void* buffer = "";

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}
//
//void recibir_mensaje(int socket_cliente)
//{
//	int size;
//	char* buffer = "";
//	buffer = (char*) recibir_buffer(&size, socket_cliente);
//	//strcat(buffer, "\0");   // Chequear esto
//	log_info(logger, "Me llego el mensaje %s", buffer);
//	free(buffer);
//}
//
////podemos usar la lista de valores para poder hablar del for y de como recorrer la lista

t_paquete* recibir(int socket)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	int recibido = 0;
	recibido = recv(socket, &paquete->palabraReservada, sizeof(int), MSG_WAITALL);

	if(recibido == 0) {
		paquete->palabraReservada = -1;
		void* requestRecibido = malloc(sizeof(int));
		paquete->request = requestRecibido;
		return paquete;
	}

	recv(socket, &paquete->tamanio, sizeof(int), MSG_WAITALL);

	void* requestRecibido = malloc(paquete->tamanio);

	recv(socket, requestRecibido, paquete->tamanio, MSG_WAITALL);

	paquete->request = requestRecibido;

	return paquete;
}
//
//
//
////cliente
//
//
//void* serializar_paquete(t_paquete* paquete, int bytes)
//{
//	void * magic = malloc(bytes);
//	int desplazamiento = 0;
//
//	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
//	desplazamiento+= sizeof(int);
//	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
//	desplazamiento+= sizeof(int);
//	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
//	desplazamiento+= paquete->buffer->size;
//
//	return magic;
//}

int crearConexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
		printf("error");

	freeaddrinfo(server_info);

	return socket_cliente;
}
//
//
//void crear_buffer(t_paquete* paquete)
//{
//	paquete->buffer = malloc(sizeof(t_buffer));
//	paquete->buffer->size = 0;
//	paquete->buffer->stream = NULL;
//}
//
//t_paquete* crear_super_paquete(void)
//{
//	//me falta un malloc!
//	t_paquete* paquete;
//
//	//descomentar despues de arreglar
//	//paquete->codigo_operacion = PAQUETE;
//	//crear_buffer(paquete);
//	return paquete;
//}
//
//t_paquete* crear_paquete(void)
//{
//	t_paquete* paquete = malloc(sizeof(t_paquete));
//	paquete->codigo_operacion = PAQUETE;
//	crear_buffer(paquete);
//	return paquete;
//}
//
//void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
//{
//	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));
//
//	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
//	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);
//
//	paquete->buffer->size += tamanio + sizeof(int);
//}
//
void enviar(t_paquete* paquete, int socket_cliente)
{
	log_info(logger, "Dentro de enviar");
	int tamanioPaquete = 2 * sizeof(int) + paquete->tamanio; // Preguntar
	printf("Despues de tamanio: %d \n", tamanioPaquete);
	void* buffer = malloc(tamanioPaquete);
	// memcpy(destino, origen, n) = copia n cantidad de caracteres de origen en destino
	// destino es un string
	memcpy(buffer, &paquete->palabraReservada, sizeof(int));
	printf("Buffer1: %s \n", (char*) buffer);
	memcpy(buffer + sizeof(int), &paquete->tamanio, sizeof(int));
	printf("Buffer2: %s \n", (char*) buffer);
	memcpy(buffer + 2 * sizeof(int), paquete->request, paquete->tamanio);
	printf("Todo el buffer: %s \n", (char*) buffer);
	printf("Tamanio: %d \n", tamanioPaquete);
	send(socket_cliente, buffer, tamanioPaquete, MSG_WAITALL);
	free(buffer);
}


//void eliminar_paquete(t_paquete* paquete)
//{
//	free(paquete->buffer->stream);
//	free(paquete->buffer);
//	free(paquete);
//}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}
