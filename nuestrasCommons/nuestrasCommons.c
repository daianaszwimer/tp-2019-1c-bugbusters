#include "nuestrasCommons.h"
void iterator(char* value) {
	printf("%s\n", value);
}

t_log* iniciar_logger() {
	return log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
}

t_config* leer_config() {
	return config_create("kernel.config");
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
//t_paquete* armar_paquete() {
//	t_paquete* paquete = crear_paquete();
//
//	void _agregar(char* leido) {
//		// Estamos aprovechando que podemos acceder al paquete
//		// de la funcion exterior y podemos agregarle lineas!
//		agregar_a_paquete(paquete, leido, strlen(leido) + 1);
//	}
//
//	_leer_consola_haciendo((void*) _agregar);
//
//	return paquete;
//}
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
//int iniciar_servidor(void)
//{
//	int socket_servidor;
//
//    struct addrinfo hints, *servinfo, *p;
//
//    memset(&hints, 0, sizeof(hints));
//    hints.ai_family = AF_UNSPEC;
//    hints.ai_socktype = SOCK_STREAM;
//    hints.ai_flags = AI_PASSIVE;
//
//    getaddrinfo(IP, PUERTO, &hints, &servinfo);
//
//    for (p=servinfo; p != NULL; p = p->ai_next)
//    {
//        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
//            continue;
//
//        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
//            close(socket_servidor);
//            continue;
//        }
//        break;
//    }
//
//	listen(socket_servidor, SOMAXCONN);
//
//    freeaddrinfo(servinfo);
//
//    log_trace(logger, "Listo para escuchar a mi cliente");
//
//    return socket_servidor;
//}
//
//int esperar_cliente(int socket_servidor)
//{
//	struct sockaddr_in dir_cliente;
//	int tam_direccion = sizeof(struct sockaddr_in);
//
//	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);
//
//	log_info(logger, "Se conecto un cliente!");
//
//	return socket_cliente;
//}

int validarMensaje(char* mensaje){
	int i=0, j=0, cod_request;
	char* request;
	while(mensaje[i] != ' '){
		request[j]=mensaje[i];
		i++;
		j++;
	}
	log_info(logger,"guarde la request");
	cod_request = obtenerCodRequest(request);
	switch(cod_request){
		case SELECT:
			log_info(logger, "ME LLEGO UN SELECTTTTTTTTTTTTTTTTTTTTTTTTTTTTT!!!!!!!!!!");
			break;
		default:
			log_info(logger, "no, no funciono.");
			//log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
	}
}

int obtenerCodRequest(char * request){
	if (strcmp(request, "SELECT")){
		return 0;
	}
	if (strcmp(request, "INSERT")){
			return 1;
		}
	if (strcmp(request, "CREATE")){
			return 2;
		}
	if (strcmp(request, "DESCRIBE")){
			return 3;
		}
	if (strcmp(request, "DROP")){
			return 4;
		}
	if (strcmp(request, "JOURNAL")){
			return 5;
		}
	if (strcmp(request, "ADD")){
			return 6;
		}
	if (strcmp(request, "RUN")){
			return 7;
		}
	if (strcmp(request, "METRICS")){
			return 8;
		}
	else {
		return -1;
	}
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
//void* recibir_buffer(int* size, int socket_cliente)
//{
//	void* buffer = "";
//
//	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
//	buffer = malloc(*size);
//	recv(socket_cliente, buffer, *size, MSG_WAITALL);
//
//	return buffer;
//}
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
//t_list* recibir_paquete(int socket_cliente)
//{
//	int size;
//	int desplazamiento = 0;
//	void * buffer;
//	t_list* valores = list_create();
//	int tamanio;
//
//	buffer = recibir_buffer(&size, socket_cliente);
//	while(desplazamiento < size)
//	{
//		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
//		desplazamiento+=sizeof(int);
//		char* valor = malloc(tamanio);
//		memcpy(valor, buffer+desplazamiento, tamanio);
//		desplazamiento+=tamanio;
//		list_add(valores, valor);
//	}
//	free(buffer);
//	return valores;
//	return NULL;
//}
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
//int crearConexion(char *ip, char* puerto)
//{
//	struct addrinfo hints;
//	struct addrinfo *server_info;
//
//	memset(&hints, 0, sizeof(hints));
//	hints.ai_family = AF_UNSPEC;
//	hints.ai_socktype = SOCK_STREAM;
//	hints.ai_flags = AI_PASSIVE;
//
//	getaddrinfo(ip, puerto, &hints, &server_info);
//
//	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
//
//	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
//		printf("error");
//
//	freeaddrinfo(server_info);
//
//	return socket_cliente;
//}
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
//void enviar_paquete(t_paquete* paquete, int socket_cliente)
//{
//	int bytes = paquete->buffer->size + 2*sizeof(int);
//	void* a_enviar = serializar_paquete(paquete, bytes);
//
//	send(socket_cliente, a_enviar, bytes, 0);
//
//	free(a_enviar);
//}
//
//void eliminar_paquete(t_paquete* paquete)
//{
//	free(paquete->buffer->stream);
//	free(paquete->buffer);
//	free(paquete);
//}
//
//void liberar_conexion(int socket_cliente)
//{
//	close(socket_cliente);
//}
