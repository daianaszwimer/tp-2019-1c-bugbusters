#include "nuestro_lib.h"

/* convertirKey()
 * Parametros:
 * 	-> key ::  char*
 * 	-> key16 :: unit16*
 * Descripcion: en el caso de que se pueda, crea una key de tipo int (de 16 bits)
 * Return:
 * 	-> :: int */
int convertirKey(char* key) {
	uint64_t key64;
	uint16_t key16;
	key64 = strtol(key,NULL,10); //strol devuelve un int como resultado deconvierte un string a un int.LOs parametros son strtol(string, puntero al string, base)
	if(key64 < 65536) {
	    key16 = strtol(key,NULL,10);
	    return key16;
	}
	return NUESTRO_ERROR;
}

/* convertirTimestamp()
 * Parametros:
 * 	-> key ::  char*
 * 	-> key16 ::  unsigned long long*
 * Descripcion: crea el timestamp en tipo unidgned long long en el caso de que sea posible.
 * 				Es posible cuando el timestamp es menor al del milisegundo en que se entro a la funcion,
 * Return:
 * 	-> :: int */
void convertirTimestamp(char* timestamp, unsigned long long* timestampLong) {
	*timestampLong = strtol(timestamp,NULL,10);
}

/* obtenerEnumConsistencia()
 * Parametros
 * -> consistencia :: char*
 * Descripcion: obtiene la consistencia y devuelve el enum correspondiente
 * Return: constante consistencia
 * -> consistencia
 * */
consistencia obtenerEnumConsistencia(char* consistencia) {
	if(string_equals_ignore_case(consistencia, "sc")) {
		return SC;
	} else if(string_equals_ignore_case(consistencia, "shc")) {
		return SHC;
	} else if(string_equals_ignore_case(consistencia, "ec")) {
		return EC;
	} else {
		return CONSISTENCIA_INVALIDA;
	}
}

void iterator(char* value) {
	printf("%s\n", value);
}

/* separarRequest()
 * Parametros
 * -> text :: char*
 * Descripcion: version mejorada de string_split (de las commons) que recibe un char* y lo separa por espacios (tomando como un string entero aquello que este entre comillas)
 * Return: char**
 * */
// TODO: que funcione bien cuando hay comillas
char** separarRequest(char* text) {
	char **substrings = NULL;
	int size = 0;

	char *text_to_iterate = string_duplicate(text);

	char *next = text_to_iterate;
	char *str = text_to_iterate;
	int freeToken = 0;

	while(next[0] != '\0') {
		char* token = strtok_r(str, " ", &next);
		if(token == NULL) {
			break;
		}
		if(*token == '"'){ //Si la palabra arranca con comillas, hay que agarrar todas las palabras hasta las siguientes comillas
			token++; //Esto borra las primeras comillas del token
			char* restOfToken = strtok_r(str, "\"", &next);
			if(restOfToken != NULL){ //En el caso de que solo haya una palabra entre comillas, la siguiente vez lo que obtiene la funcion strtok es NULL, asi que solo concatenamos si es distinto de NULL
				token = string_from_format("%s %s", token, restOfToken);
				freeToken = 1; // Dado que string_from_format hace un malloc adentro, despues vamos a tener que liberar el token.
			}
			if(token[strlen(token)-1] == '"'){ //por ultimo si el ultimo caracter son comillas, lo removemos
				token[strlen(token)-1] = 0;
			}
		}

		str = NULL;
		size++;
		substrings = realloc(substrings, sizeof(char*) * size);
		substrings[size - 1] = string_duplicate(token);
		if(freeToken) {
			free(token);
			freeToken = 0;
		}
	}

	size++;
	substrings = realloc(substrings, sizeof(char*) * size);
	substrings[size - 1] = NULL;

	free(text_to_iterate);
	return substrings;
}

/*obtenerHoraActual()
 * Parametros:
 * Descripcion: Hora actual en minutos y microsegundos
 * Return:
 * 	-> :: unsigned long long */
unsigned long long obtenerHoraActual(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long long millisegundosDesdeEpoch = ((unsigned long long)tv.tv_sec) * 1000 + ((unsigned long long)tv.tv_usec) / 1000;
	return millisegundosDesdeEpoch;
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


//TODO abajo habia otra funcion que era obtener parametros, chequear diferencias y ver cual dejar.
char** obtenerParametros(char* request) { //ojo con la memoria reservada
	char** queryYParametros =string_n_split(request, 2, " ");
	return string_split(queryYParametros[1], " ");
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

/* concatenar()
 * Parametros:
 * 	-> primerString :: char*
 * 	-> ... :: n char*
 * Descripcion: concatena n strings
 * Se le debe pasar como ultimo parametro un NULL(indicando que se finalizo la entrada de parametros)
 * Se debe hacer un free de la variable retornada
 * Return: string concatenado*/
//char* concatenar(char* primerString, ...) { // los 3 puntos indican cantidad de parametros variable
//	char* stringFinal;	// String donde se guarda el resultado de la concatenacion
//	va_list parametros; // va_list es una lista que entiende los 3 puntos que se pasan como parametro
//	char* parametro;	// Este es un parametro solo
//
//	if (primerString == NULL)
//		return NULL;
//
//	stringFinal = strdup(primerString);
//	va_start(parametros, primerString);	// Inicalizo el va_list
//
//	while ((parametro = va_arg(parametros, char*)) != NULL) {	// Recorro la lista de parametros hasta encontrar un NULL
//		stringFinal = (char*) realloc(stringFinal, strlen(stringFinal) + strlen(parametro) + 1); // Alojo memoria para el string concatenado
//		strcat(stringFinal, parametro); // Concateno el parametro con stringFinal
//	}
//
//	va_end(parametros); // Libero la va_list
//	return stringFinal;
//}

/* obtenerParametros()
 * Parametros:
 *  -> request :: char*
 *  Descripcion: recibe un request (ej: SELECT Tabla 4) y devuelve los parametros ([Tabla, 4])
 *  Return:
 *   -> requestSeparada :: char**
 */

//char** obtenerParametros(char* request) {
//	char** requestSeparada;
//	requestSeparada = separarRequest(request, " ");
//	//n = longitudDeArrayDeStrings(requestSeparada);
//    memmove(requestSeparada, requestSeparada+1, strlen(requestSeparada));
//	return requestSeparada;
//}

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
				return NUESTRO_ERROR;
			}
		}

		char** parametros = separarRequest(request[1]);
		int cantidadDeParametros = longitudDeArrayDeStrings(parametros);
		for(int i=0; request[i] != NULL; i++){
			free(request[i]);
		}
		free(request);
		for(int i=0; parametros[i] != NULL; i++){
			free(parametros[i]);
		}
		free(parametros);
		//free(request);
		//free(parametros);
		if(validadCantDeParametros(cantidadDeParametros,codPalabraReservada, logger) == TRUE) {
			return EXIT_SUCCESS;
		}
		else {
			return NUESTRO_ERROR;
		}
	}
	else {
		return NUESTRO_ERROR;
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
		return NUESTRO_ERROR;
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
				if(cantidadDeParametros == PARAMETROS_INSERT || cantidadDeParametros == PARAMETROS_INSERT_TIMESTAMP) {
					retorno = EXIT_SUCCESS;
				} else {
					retorno = EXIT_FAILURE;
				}
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
				retorno = NUESTRO_ERROR;
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
		return NUESTRO_ERROR;
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
		codPalabraReservada = NUESTRO_ERROR;
	}
	return codPalabraReservada;
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void* buffer = "";

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

t_paquete* recibir(int socket)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	int recibido = 0;
	recibido = recv(socket, &paquete->palabraReservada, sizeof(int), MSG_WAITALL);

	if(recibido == 0) {
		printf("ERROR \n");
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

//cliente

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
