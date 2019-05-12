#include "nuestro_lib.h"

void iterator(char* value) {
	printf("%s\n", value);
}

/* separarString()
 * Parametros:
 * 	-> mensaje ::  char*
 * Descripcion: separa un string dado un espacio y va guaradando cada palabra en un array.
 * Return: array de string
 * 	-> :: char** */
char** separarString(char* mensaje) {
	return string_split(mensaje, " ");
}

/* longitudDeArrayDeStrings()
 * Parametros:
 * 	-> array ::  char**
 * Descripcion: Cuenta la cantidad de elementos de un array.
 * Return:
 * 	-> longitud :: int */
int longitudDeArrayDeStrings(char** array){
	int longitud = 0;
	while(array[longitud] !=NULL){
		longitud++;
	}
	return longitud;
}
/* obtenerParametros()
 * Parametros:
 *  -> request :: char*
 *  Descripcion: recibe un request (ej: SELECT Tabla 4) y devuelve los parametros ([Tabla, 4])
 *  Return:
 *   -> requestSeparada :: char**
 */
char** obtenerParametros(char* request) {
	char** requestSeparada;
	requestSeparada = separarString(request);
	//n = longitudDeArrayDeStrings(requestSeparada);
    memmove(requestSeparada, requestSeparada+1, strlen(requestSeparada));
	return requestSeparada;
}

/* leer_config()
 * Parametros:
 * 	-> nombreArchivo ::  char*
 * Descripcion: CREA el archivo de confirguracion asignandole el nombre que se le envia
 * 				por parametro  la funcion.
 * Return:
 * 	-> t_config :: t_config* */
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

////								  requests
//void _leer_consola_haciendo(void (accion)(char)) {
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

int iniciar_servidor(char* puerto, char* ip)
{
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(ip, puerto, &hints, &servinfo);
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
    return socket_servidor;
}


/* esperar_cliente()
 * Parametros:
 * 	-> int ::  socket_servidor
 * Descripcion: guarda en socket_cliente el resultado de la conexion que recibe (lo hace a traves de la funcion
 * 				accept)
 * Return:
 * 	-> socket_cliente :: int */
int esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;
	unsigned int tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

	//log_info(logger, "Se conecto un cliente!");

	return socket_cliente;
}


/* validarMensaje()
 * Parametros:
 * 	-> mensaje ::  char*
 * 	-> coponenete :: Componente
 * Descripcion: Verifica que toda la request ingresada sea correcta, es decir, verifica que:
 * 				- que la palabra reservada exista y la comprenda el componente a quien se le envio la request
 * 				- los parametros sean los correctos, en cuanto  su cantidad.
 * Return: success or failure
 * 	-> exit :: int */
int validarMensaje(char* mensaje, Componente componente, t_log* logger) {
	char** request = string_n_split(mensaje, 2, " ");

	int codPalabraReservada = obtenerCodigoPalabraReservada(request[0], componente);
	if(validarPalabraReservada(codPalabraReservada, componente, logger)== TRUE){
		if(request[1]==NULL){
			if(cantDeParametrosEsCorrecta(0,codPalabraReservada)== TRUE){
				free(request);
				return EXIT_SUCCESS;
			}else{
				free(request);
				log_info(logger,"No se ha ingresado ningun parametro para la request, y esta request necesita parametros ");
				return QUERY_ERROR;
			}
		}

		char** parametros = separarString(request[1]);
		int cantidadDeParametros = longitudDeArrayDeStrings(parametros);
		//free(request);
		//free(parametros);
		if( validadCantDeParametros(cantidadDeParametros,codPalabraReservada, logger)== TRUE) {
			return EXIT_SUCCESS;
		}
		else {
			return QUERY_ERROR;
		}
	}
	else {
		return QUERY_ERROR;
	}
}


/* validadCantDeParametros()
 * Parametros:
 * 	-> cantidadDeParametros ::  int
 * 	-> codPalabraReservada :: int
 * Descripcion: evalua que se haya ingresado la cantidad correcta de parametros que requiere la request realizada.
 * 				 En el caso de no serlo, se informa de ello.
 * Return: success or failure
 * 	-> exit :: int */
int validadCantDeParametros(int cantidadDeParametros, int codPalabraReservada, t_log* logger){
	int resultadoCantParametros = cantDeParametrosEsCorrecta(cantidadDeParametros, codPalabraReservada);
	if(resultadoCantParametros == EXIT_FAILURE){
		log_info(logger,"No se ha ingresado la cantidad correcta de paraemtros");
		return QUERY_ERROR;
	}else{
		return EXIT_SUCCESS;
	}
}


/* cantDeParametrosEsCorrecta()
 * Parametros:
 * 	-> cantidadDeParametros ::  int
 * 	-> codPalabraReservada :: int
 * Descripcion: verifica que la cantidad de parametros ingresados en la request sea igual a la
 * 				cantidad correcta de parametros que requiere la request realizada.
 * Return: success or failure
 * 	-> exit :: int */
int cantDeParametrosEsCorrecta(int cantidadDeParametros, int codPalabraReservada) {
	int retorno;
	switch(codPalabraReservada) {
			case SELECT:
				retorno = (cantidadDeParametros == PARAMETROS_SELECT)? EXIT_SUCCESS : EXIT_FAILURE;
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
				retorno = QUERY_ERROR;
				break;
	}
	return retorno;
}


/* validarPalabraReservada()
 * Parametros:
 * 	-> codigoPalabraReservada ::  int 
 * 	-> coponenete :: Componente
 * Descripcion: evalua que se haya ingresado una request que entienda el componenete.
 * 				 En el caso de no serlo, se informa de ello.
 * Return: success or failure
 * 	-> exit :: int */
int validarPalabraReservada(int codigoPalabraReservada, Componente componente, t_log* logger){
	if(codigoPalabraReservada == -1) {
		log_info(logger, "Debe ingresar un request válido");
		return QUERY_ERROR;
	}
	return EXIT_SUCCESS;
}


/* obtenerCodigoPalabraReservada()
 * Parametros:
 * 	-> palabraReservada ::  char* 
 * 	-> componente :: Componente
 * Descripcion: se obtiene el codigo de la palabra reservada de la request.
 * 				Lo hace verificando si la request realizada a un componente se encuentre
 * 				dentro de las requests que entiende dicho componente.
 * Return: 
 * 	-> codPalabraReservada :: int */
int obtenerCodigoPalabraReservada(char* palabraReservada, Componente componente) {
	int codPalabraReservada;
	if (!strcmp(palabraReservada, "SELECT")) {
		codPalabraReservada = 0;
	}
	else if (!strcmp(palabraReservada, "INSERT")) {
		codPalabraReservada = 1;
	}
	else if (!strcmp(palabraReservada, "CREATE")) {
		codPalabraReservada = 2;
	}
	else if (!strcmp(palabraReservada, "DESCRIBE")) {
		codPalabraReservada = 3;
	}
	else if (!strcmp(palabraReservada, "DROP")) {
		codPalabraReservada = 4;
	}
	else if (!strcmp(palabraReservada, "JOURNAL")) {
		codPalabraReservada = (componente == LFS) ? -1 : 5;
	}
	else if (!strcmp(palabraReservada, "ADD")) {
		codPalabraReservada = (componente == KERNEL) ? 6 : -1;
	}
	else if (!strcmp(palabraReservada, "RUN")) {
		codPalabraReservada = (componente == KERNEL) ? 7 : -1;
	}
	else if (!strcmp(palabraReservada, "METRICS")) {
		codPalabraReservada = (componente == KERNEL) ? 8 : -1;
	}
	else {
		codPalabraReservada = QUERY_ERROR;
	}
	return codPalabraReservada;
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

int crearConexion(char* ip, char* puerto)
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

/* enviar()
 * Parametros:
 * 	-> t_paquete* ::  paquete
 * 	-> int :: socket_cliente
 * Descripcion: una vez serializado el paquete, hace send del mismo a traves del socket_cliente
 * 				y finalmente se libera el paquete que se envio.
 * Return:
 * 	-> :: void  */
void enviar(cod_request palabraReservada, char* mensaje, int socket_cliente)
{
	//armamos el paquete
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->palabraReservada = palabraReservada;
	paquete->tamanio = strlen(mensaje)+1;
	paquete->request = malloc(paquete->tamanio);
	memcpy(paquete->request, mensaje, paquete->tamanio);
	int tamanioPaquete = 2 * sizeof(int) + paquete->tamanio;
	//serializamos
	void* paqueteAEnviar = serializar_paquete(paquete, tamanioPaquete);
	//enviamos
	send(socket_cliente, paqueteAEnviar, tamanioPaquete, 0);
	free(paqueteAEnviar);
	free(paquete->request);
	free(paquete);
}


/* serializar_paquete()
 * Parametros:
 * 	-> t_paquete* ::  paquete
 * 	-> int :: tamanioPaquete
 * Descripcion: prepara el paquete que luego se enviara por socket
 * Return:
 * 	-> buffer :: void*  */
void* serializar_paquete(t_paquete* paquete, int tamanioPaquete)
{
	void * buffer = malloc(tamanioPaquete);
	int desplazamiento = 0;

	// memcpy(destino, origen, n) = copia n cantidad de caracteres de origen en destino
	// destino es un string
	memcpy(buffer + desplazamiento, &paquete->palabraReservada, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buffer + desplazamiento, &paquete->tamanio, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buffer + desplazamiento, paquete->request, paquete->tamanio);
	return buffer;
}


/*eliminar_paquete()
 * Parametros:
 * 	-> t_paquete* ::  paquete
 * Descripcion: CONSIDERACIONES: el paquete es un puntero a ua estructura t_paquete y, a
 * 				su ve paquete tiene un string, char*, llamado request
 * 				DESCRIPCION: elimino primero la request del paquete y luego dicho paquete
 * Return:
 * 	-> :: void */
void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->request);
	free(paquete);
}

/*liberar_conexion()
 * Parametros:
 * 	-> int ::  socket_cliente
 * Descripcion: cierra la conexion que creo
 * Return:
 * 	-> :: void */
void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}

/* Funciones de Multiplexacion */

/* eliminarClientesCerrados()
 * Parametros:
 * 	-> t_list* :: descriptores
 * 	-> int* :: numeroDeClientes
 * Descripcion: elimina de la lista de descriptores, los clientes cerrados (que tengan un -1 en su fd)
 * Return:
 * 	-> :: void  */
void eliminarClientesCerrados(t_list* descriptores, int* numeroDeClientes) {
	for(int i = 0; i < *numeroDeClientes; i++) {
		if((int) list_get(descriptores,i) == -1) {
			int valorRemovido = (int) list_remove(descriptores, i);
			*numeroDeClientes -= 1;
		}
	}
}

/* maximo()
 * Parametros:
 * 	-> t_list* :: descriptores
 * 	-> int :: descriptorServidor
 * 	-> int :: numeroDeClientes
 * Descripcion: devuelve el valor maximo entre todos los fd's de la lista de descriptores y el fd del servidor
 * Return:
 * 	-> valorMaximo :: int  */
int maximo(t_list* descriptores, int descriptorServidor, int numeroDeClientes) {
	int max = descriptorServidor;
	for(int i = 0; i < numeroDeClientes; i++) {
		if((int) list_get(descriptores,i) > max) {
			max = (int) list_get(descriptores,i);
		}
	}
	return max;
}
