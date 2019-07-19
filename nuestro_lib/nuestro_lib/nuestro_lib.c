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
	*timestampLong = strtoll(timestamp,NULL,10);
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

//TODO: Comparar con separarString de memoria
/* separarRequest()
 * Parametros
 * -> text :: char*
 * Descripcion: version mejorada de string_split (de las commons) que recibe un char* y lo separa por espacios (tomando como un string entero aquello que este entre comillas)
 * Return: char**
 * */
// TODO: que funcione bien cuando hay comillas
char** separarRequest(char* text) {
	char** requestSeparada= NULL;
	if(string_contains(text, "\"")){
		requestSeparada=string_split(text,"\"");

		char** primeraParte = string_split(requestSeparada[0], " ");

		int i;
		for(i = 0; primeraParte[i] != NULL; i++);

		primeraParte = realloc(primeraParte, sizeof(char*) * 5);
		primeraParte[i] = strdup(requestSeparada[1]);

		if(requestSeparada[2] != NULL){
			i++;
			primeraParte = realloc(primeraParte, sizeof(char*) * 6);
			primeraParte[i] = strdup(requestSeparada[2]);
		}

		primeraParte[i+1] = NULL;
		liberarArrayDeChar(requestSeparada);
		return primeraParte;
	}else{
		requestSeparada=string_split(text," ");
		return requestSeparada;
	}


//	char **substrings = NULL;
//	int size = 0;
//
//	char *text_to_iterate = string_duplicate(text);
//
//	char *next = text_to_iterate;
//	char *str = text_to_iterate;
//	int freeToken = 0;
//	char* token;
//
//	while(next[0] != '\0') {
//		token = strtok_r(str, " ", &next);
//		if(token == NULL) {
//			break;
//		}
//		if(*token == '"'){ //Si la palabra arranca con comillas, hay que agarrar todas las palabras hasta las siguientes comillas
//			token++; //Esto borra las primeras comillas del token
//			if(token[strlen(token)-1] == '"' && string_contains(token, " ")){
//				char* restOfToken = strtok_r(str, "\"", &next);
//				if(restOfToken != NULL){ //En el caso de que solo haya una palabra entre comillas, la siguiente vez lo que obtiene la funcion strtok es NULL, asi que solo concatenamos si es distinto de NULL
//					token = string_from_format("%s %s", token, restOfToken);
//					freeToken = 1; // Dado que string_from_format hace un malloc adentro, despues vamos a tener que liberar el token.
//				}
//			}
//
//			if(token[strlen(token)-1] == '"'){ //por ultimo si el ultimo caracter son comillas, lo removemos
//				token[strlen(token)-1] = 0;
//			}
//		}
//
//		str = NULL;
//		size++;
//		substrings = realloc(substrings, sizeof(char*) * size);
//		substrings[size - 1] = string_duplicate(token);
//		if(freeToken) {
//			free(token);
//			freeToken = 0;
//		}
//	}
//
//	size++;
//	substrings = realloc(substrings, sizeof(char*) * size);
//	substrings[size - 1] = NULL;
//
//	free(text_to_iterate);
//	return substrings;
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

        int val = 1;
        if(setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1){
        	perror("setsockopt");
        }

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            perror("bind");
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
	// enviar
	//log_info(logger, "Se conecto un cliente!");

	return socket_cliente;
}

/* liberarArrayDeChar()
 * Parametros:
 *  -> char** :: arrayDeChar
 *  Descripcion: tomar un char** y lo libera
 * Return:
 *  -> void
 */
void liberarArrayDeChar(char** arrayDeChar) {
	if(arrayDeChar!=NULL){
		for (int j = 0; arrayDeChar[j] != NULL; j++) {
			free(arrayDeChar[j]);
			arrayDeChar[j]=NULL;
		}
	}free(arrayDeChar);
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
errorNo validarMensaje(char* mensaje, Componente componente, char** mensajeError) {
	errorNo error = SUCCESS;
	char** requestDividida = string_n_split(mensaje, 2, " ");

	if(string_is_empty(mensaje) || requestDividida[0] == NULL){
		error = REQUEST_VACIA;
	}else{
		int codPalabraReservada = obtenerCodigoPalabraReservada(requestDividida[0], componente);
		if(codPalabraReservada == NUESTRO_ERROR){
			error = COD_REQUEST_INV;
		}else{
			char** parametros = NULL;
			if(requestDividida[1] != NULL){
				parametros = separarRequest(requestDividida[1]);
			}
			error = validarParametrosDelRequest(codPalabraReservada, parametros, componente);
		}
	}

	switch(error){
		case SUCCESS:
			*mensajeError = '\0';
			break;
		case REQUEST_VACIA:
			*mensajeError = "Request vacia";
			break;
		case COD_REQUEST_INV:
			*mensajeError = "Comando invalido";
			break;
		case CANT_PARAM_INV:
			*mensajeError = "Cantidad de parametros invalidos para la request ingresada";
			break;
		case KEY_NO_NUMERICA:
			*mensajeError = "Key no numerica";
			break;
		case KEY_MUY_GRANDE:
			*mensajeError = "La key es demasiado grande";
			break;
		case TIMESTAMP_NO_NUMERICO:
			*mensajeError = "Timestamp no numerico";
			break;
		case CONSISTENCIA_NO_VALIDA:
			*mensajeError = "Tipo de consistencia no valida";
			break;
		default:
			*mensajeError = '\0';
			break;

	}

	liberarArrayDeChar(requestDividida);
	return error;
}

/* validarParametrosDelRequest()
 * Parametros:
 * 	-> cantidadDeParametros ::  int
 * 	-> codPalabraReservada :: int
 * Descripcion: verifica que la cantidad de parametros ingresados en la request sea igual a la
 * 				cantidad correcta de parametros que requiere la request realizada.
 * Return: success or failure
 * 	-> exit :: int */
errorNo validarParametrosDelRequest(int codPalabraReservada, char** parametros, Componente componente) {
	errorNo error = SUCCESS;

	int cantidadDeParametros = 0;
	if(parametros != NULL){
		cantidadDeParametros = longitudDeArrayDeStrings(parametros);
	}

	switch(codPalabraReservada) {
			case SELECT:
				error = validarSelect(parametros, cantidadDeParametros);
				break;
			case INSERT:
				error = validarInsert(parametros, cantidadDeParametros, componente);
				break;
			case CREATE:
				error = validarCreate(parametros, cantidadDeParametros);
				break;
			case DESCRIBE:
				error = validarDescribe(parametros, cantidadDeParametros);
				break;
			case DROP:
				error = (cantidadDeParametros == PARAMETROS_DROP) ? SUCCESS : CANT_PARAM_INV;
				break;
			case JOURNAL:
				error = (cantidadDeParametros == PARAMETROS_JOURNAL) ? SUCCESS : CANT_PARAM_INV;
				break;
			case ADD:
				error = (cantidadDeParametros == PARAMETROS_ADD) ? SUCCESS : CANT_PARAM_INV;
				break;
			case RUN:
				error = (cantidadDeParametros == PARAMETROS_RUN) ? SUCCESS : CANT_PARAM_INV;
				break;
			case METRICS:
				error = (cantidadDeParametros == PARAMETROS_METRICS) ? SUCCESS : CANT_PARAM_INV;
				break;
			default:
				error = FAILURE;
				break;
	}
	if(parametros != NULL){
		liberarArrayDeChar(parametros);
	}
	return error;
}

errorNo validarSelect(char** parametros, int cantidadDeParametros){
	errorNo error = SUCCESS;
	if(cantidadDeParametros != PARAMETROS_SELECT){
		error = CANT_PARAM_INV;
	}else{
		char* key = parametros[1];
		if(!esNumero(key)){
			error = KEY_NO_NUMERICA;
		}
	}
	return error;
}

errorNo validarInsert(char** parametros, int cantidadDeParametros, Componente componente){
	errorNo error = SUCCESS;
	if(componente == LFS){
		if(cantidadDeParametros != PARAMETROS_INSERT && cantidadDeParametros != PARAMETROS_INSERT_TIMESTAMP){
			error = CANT_PARAM_INV;
		}
	}else{
		if(cantidadDeParametros != PARAMETROS_INSERT){
			error = CANT_PARAM_INV;
		}
	}

	if(error == SUCCESS){
		char* key = parametros[1];
		int keyConvertida = convertirKey(key);
		if(!esNumero(key)){
			error = KEY_NO_NUMERICA;
		} else if(keyConvertida == -1) {
			error = KEY_MUY_GRANDE;
		}
		char* timestamp = parametros[3];
		if(timestamp != NULL){
			if(!esNumero(timestamp)){
				error = TIMESTAMP_NO_NUMERICO;
			}
		}
	}
	return error;
}

errorNo validarCreate(char** parametros, int cantidadDeParametros){
	errorNo error = SUCCESS;
	if(cantidadDeParametros != PARAMETROS_CREATE){
		error = CANT_PARAM_INV;
	}else{
		char* consistencia = parametros[1];
		if(obtenerEnumConsistencia(consistencia) == CONSISTENCIA_INVALIDA){
			error = CONSISTENCIA_NO_VALIDA;
		}
		char* particiones = parametros[2];
		if(!esNumero(particiones)){
			error = KEY_NO_NUMERICA;
		}
		char* tiempoDeCompactacion = parametros[3];
		if(!esNumero(tiempoDeCompactacion)){
			error = TIMESTAMP_NO_NUMERICO;
		}
	}
	return error;
}

errorNo validarDescribe(char** parametros, int cantidadDeParametros){
	errorNo error = SUCCESS;
	if(cantidadDeParametros != PARAMETROS_DESCRIBE_GLOBAL && cantidadDeParametros != PARAMETROS_DESCRIBE_TABLA){
		error = CANT_PARAM_INV;
	}
	return error;
}

int esNumero(char* str){
	for (int i = 0; str[i] != '\0'; i++){
		if(!isdigit(str[i])) return 0;
	}
	return 1;
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
		codPalabraReservada = SELECT;
	}
	else if (!strcmp(palabraReservada, "INSERT")) {
		codPalabraReservada = INSERT;
	}
	else if (!strcmp(palabraReservada, "CREATE")) {
		codPalabraReservada = CREATE;
	}
	else if (!strcmp(palabraReservada, "DESCRIBE")) {
		codPalabraReservada = DESCRIBE;
	}
	else if (!strcmp(palabraReservada, "DROP")) {
		codPalabraReservada = DROP;
	}
	else if (!strcmp(palabraReservada, "JOURNAL")) {
		codPalabraReservada = (componente == LFS) ? NUESTRO_ERROR : JOURNAL;
	}
	else if (!strcmp(palabraReservada, "ADD")) {
		codPalabraReservada = (componente == KERNEL) ? ADD : NUESTRO_ERROR;
	}
	else if (!strcmp(palabraReservada, "RUN")) {
		codPalabraReservada = (componente == KERNEL) ? RUN : NUESTRO_ERROR;
	}
	else if (!strcmp(palabraReservada, "METRICS")) {
		codPalabraReservada = (componente == KERNEL) ? METRICS : NUESTRO_ERROR;
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
	if(recibido <= 0) {
		paquete->palabraReservada = COMPONENTE_CAIDO;
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

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1) {
		// printf("error");
		freeaddrinfo(server_info);
		return COMPONENTE_CAIDO;
	}

	freeaddrinfo(server_info);

	return socket_cliente;
}

/*
 *HANDSHAKES
 *
 */
void enviarValueLFS(int tamanioValue, int socket_cliente)
{
	t_handshake_lfs* handshake = malloc(sizeof(t_handshake_lfs));
	handshake->tamanioValue = tamanioValue;
	int tamanioPaquete = sizeof(int);
	void* handshakeAEnviar = serializar_handshake_lfs(handshake, tamanioPaquete);
	send(socket_cliente, handshakeAEnviar, tamanioPaquete, 0);
	free(handshakeAEnviar);
	free(handshake);
}

t_handshake_lfs* recibirValueLFS(int socket)
{
	t_handshake_lfs* handshake = malloc(sizeof(t_handshake_lfs));
	recv(socket, &handshake->tamanioValue, sizeof(int), MSG_WAITALL);
	return handshake;
}

void* serializar_handshake_lfs(t_handshake_lfs* handshake, int tamanio)
{
	void * buffer = malloc(tamanio);
	// memcpy(destino, origen, n) = copia n cantidad de caracteres de origen en destino
	// destino es un string
	memcpy(buffer, &handshake->tamanioValue, sizeof(int));
	return buffer;
}

void* serializar_handshake_operacion(t_operacion* operacion, int tamanio) {
	void * buffer = malloc(tamanio);
	// memcpy(destino, origen, n) = copia n cantidad de caracteres de origen en destino
	// destino es un string
	memcpy(buffer, &operacion->tipo_rol, sizeof(int));
	return buffer;
}

int enviarTipoOperacion(rol tipoRol, int socket_cliente) {
	t_operacion* operacion = malloc(sizeof(t_operacion));
	operacion->tipo_rol = tipoRol;
	int tamanioPaquete = sizeof(int);
	void* operacionAEnviar = serializar_handshake_operacion(operacion, tamanioPaquete);
	if (send(socket_cliente, operacionAEnviar, tamanioPaquete, 0) == -1) {
		free(operacionAEnviar);
		free(operacion);
		return COMPONENTE_CAIDO;
	}
	free(operacionAEnviar);
	free(operacion);
	return SUCCESS;
}

t_operacion* recibirOperacion(int socket, int* codigoOperacion)
{
	t_operacion* operacion = malloc(sizeof(t_operacion));
	int rta = recv(socket, &operacion->tipo_rol, sizeof(int), MSG_WAITALL);

	if (rta <= 0) {
		*codigoOperacion = COMPONENTE_CAIDO;
		return operacion;
	}
	*codigoOperacion = SUCCESS;

	return operacion;
}

void* serializar_handshake_rta(t_handshake_rta* h_rta, int tamanio) {
	void * buffer = malloc(tamanio);
	memcpy(buffer, &h_rta->rta, sizeof(int));
	return buffer;
}

int enviarRtaHandshake(rta_handshake rta, int socket_cliente) {
	t_handshake_rta* handshake = malloc(sizeof(t_handshake_rta));
	handshake->rta = rta;
	int tamanioPaquete = sizeof(int);
	void* handshakeAEnviar = serializar_handshake_rta(handshake, tamanioPaquete);
	if (send(socket_cliente, handshakeAEnviar, tamanioPaquete, 0) == -1) {
		free(handshakeAEnviar);
		free(handshake);
		return COMPONENTE_CAIDO;
	}
	free(handshakeAEnviar);
	free(handshake);
	return SUCCESS;
}

t_handshake_rta* recibirRtaHandshake(int socket, int* codigoOperacion)
{
	t_handshake_rta* rta_handshake_1 = malloc(sizeof(t_handshake_rta));
	int rta = recv(socket, &rta_handshake_1->rta, sizeof(int), MSG_WAITALL);

	if (rta <= 0) {
		*codigoOperacion = COMPONENTE_CAIDO;
		return rta_handshake_1;
	}
	*codigoOperacion = SUCCESS;

	return rta_handshake_1;
}

/* enviarGossiping()
 * Parametros:
 * 	-> char* :: puertos
 * 	-> char* :: ips
 * 	-> int :: socket_cliente
 * Descripcion: recibe la data a enviar, la serializa y la manda
 * Return:
 * 	-> :: void  */
int enviarGossiping(char* puertos, char* ips, char* numeros, int esDeKernel, int socket_cliente)
{
	t_gossiping* gossiping = malloc(sizeof(t_gossiping));
	gossiping->tamanioIps = strlen(ips) + 1;
	gossiping->ips = malloc(gossiping->tamanioIps);
	memcpy(gossiping->ips, ips, gossiping->tamanioIps);


	gossiping->tamanioPuertos = strlen(puertos) + 1;
	gossiping->puertos = malloc(gossiping->tamanioPuertos);
	memcpy(gossiping->puertos, puertos, gossiping->tamanioPuertos);


	gossiping->tamanioNumeros = strlen(numeros) + 1;
	gossiping->numeros = malloc(gossiping->tamanioNumeros);
	memcpy(gossiping->numeros, numeros, gossiping->tamanioNumeros);


	gossiping->esDeKernel = esDeKernel;

	int tamanioPaquete = 4 * sizeof(int) + gossiping->tamanioIps + gossiping->tamanioPuertos + gossiping->tamanioNumeros;
	void* gossipingAEnviar = serializar_gossiping(gossiping, tamanioPaquete);
	if (send(socket_cliente, gossipingAEnviar, tamanioPaquete, 0) == -1) {
		liberarHandshakeMemoria(gossiping);
		free(gossipingAEnviar);
		gossipingAEnviar=NULL;
		return COMPONENTE_CAIDO;
	}
	liberarHandshakeMemoria(gossiping);
	free(gossipingAEnviar);
	gossipingAEnviar=NULL;
	return SUCCESS;
}

/* recibirGossiping()
 * Parametros:
 * 	-> int :: socket
 * Descripcion: recibe el gossiping y lo deserealiza
 * Return:
 * 	-> gossiping :: t_gossiping  */
t_gossiping* recibirGossiping(int socket, int* resultado)
{
	t_gossiping* gossiping = malloc(sizeof(t_gossiping));
	int rta = recv(socket, &gossiping->tamanioIps, sizeof(int), MSG_WAITALL);
	if (rta <= 0) {
		*resultado = COMPONENTE_CAIDO;
		gossiping->ips = strdup("");
		gossiping->puertos = strdup("");
		gossiping->numeros = strdup("");
		return gossiping;
	}
	char* ipsRecibidos = malloc(gossiping->tamanioIps);
	recv(socket, ipsRecibidos, gossiping->tamanioIps, MSG_WAITALL);

	recv(socket, &gossiping->tamanioPuertos, sizeof(int), MSG_WAITALL);
	char* puertosRecibidos = malloc(gossiping->tamanioPuertos);
	recv(socket, puertosRecibidos, gossiping->tamanioPuertos, MSG_WAITALL);

	recv(socket, &gossiping->tamanioNumeros, sizeof(int), MSG_WAITALL);
	char* numerosRecibidos = malloc(gossiping->tamanioNumeros);
	recv(socket, numerosRecibidos, gossiping->tamanioNumeros, MSG_WAITALL);

	recv(socket, &gossiping->esDeKernel, sizeof(int), MSG_WAITALL);

	gossiping->puertos = puertosRecibidos;
	gossiping->ips = ipsRecibidos;
	gossiping->numeros = numerosRecibidos;

	*resultado = SUCCESS;

	return gossiping;
}

/* serializar_gossiping()
 * Parametros:
 * 	-> t_gossiping* ::  gossiping
 * 	-> int :: tamanio
 * Descripcion: serializa un t_gossiping.
 * Return:
 * 	-> buffer :: void*  */
void* serializar_gossiping(t_gossiping* gossiping, int tamanio)
{
	void * buffer = malloc(tamanio);
	int desplazamiento = 0;

	// memcpy(destino, origen, n) = copia n cantidad de caracteres de origen en destino
	// destino es un string
	memcpy(buffer + desplazamiento, &gossiping->tamanioIps, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buffer + desplazamiento, gossiping->ips, gossiping->tamanioIps);
	desplazamiento += gossiping->tamanioIps;
	memcpy(buffer + desplazamiento, &gossiping->tamanioPuertos, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buffer + desplazamiento, gossiping->puertos, gossiping->tamanioPuertos);
	desplazamiento += gossiping->tamanioPuertos;
	memcpy(buffer + desplazamiento, &gossiping->tamanioNumeros, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buffer + desplazamiento, gossiping->numeros, gossiping->tamanioNumeros);
	desplazamiento += gossiping->tamanioNumeros;
	memcpy(buffer + desplazamiento, &gossiping->esDeKernel, sizeof(int));
	return buffer;
}

/* enviarHandshakeMemoria()
 * Parametros:
 * 	-> char* :: puertos
 * 	-> char* :: ips
 * 	-> int :: socket_cliente
 * Descripcion: recibe la data a enviar, la serializa y la manda
 * Return:
 * 	-> :: void  */
int enviarHandshake(Componente tipoComponente, int socket_cliente)
{
	t_handshake* handshake = malloc(sizeof(t_handshake));
	handshake->tipoComponente = tipoComponente;
	int tamanioPaquete = sizeof(int);
	void* handshakeAEnviar = serializar_handshake(handshake, tamanioPaquete);
	if (send(socket_cliente, handshakeAEnviar, tamanioPaquete, 0) == -1) {
		free(handshakeAEnviar);
		free(handshake);
		return COMPONENTE_CAIDO;
	}
	free(handshakeAEnviar);
	free(handshake);
	return SUCCESS;
}

/* recibirGossiping()
 * Parametros:
 * 	-> int :: socket
 * Descripcion: recibe el handshake y lo deserealiza
 * Return:
 * 	-> gossiping :: t_gossiping  */
t_handshake* recibirHandshake(int socket, int* codigoOperacion)
{
	t_handshake* handshake = malloc(sizeof(t_handshake));
	int rta = recv(socket, &handshake->tipoComponente, sizeof(int), MSG_WAITALL);

	if (rta <= 0) {
		*codigoOperacion = COMPONENTE_CAIDO;
		return handshake;
	}

	*codigoOperacion = SUCCESS;
	return handshake;
}

/* serializar_gossiping()
 * Parametros:
 * 	-> t_gossiping* ::  gossiping
 * 	-> int :: tamanio
 * Descripcion: serializa un t_gossiping.
 * Return:
 * 	-> buffer :: void*  */
void* serializar_handshake(t_handshake* handshake, int tamanio)
{
	void * buffer = malloc(tamanio);
	memcpy(buffer, &handshake->tipoComponente, sizeof(int));
	return buffer;
}

/* enviar()
 * Parametros:
 * 	-> t_paquete* ::  paquete
 * 	-> int :: socket_cliente
 * Descripcion: una vez serializado el paquete, hace send del mismo a traves del socket_cliente
 * 				y finalmente se libera el paquete que se envio.
 * Return:
 * 	-> :: void  */
int enviar(int cod, char* mensaje, int socket_cliente)
{
	t_paquete* paquete = (t_paquete*) malloc(sizeof(t_paquete));
	paquete->palabraReservada = cod;

	paquete->tamanio = strlen(mensaje)+1;
	paquete->request = (char*) malloc(paquete->tamanio);
	memcpy(paquete->request, mensaje, paquete->tamanio);
	int tamanioPaquete = 2 * sizeof(int) + paquete->tamanio;
	//serializamos
	void* paqueteAEnviar = serializar_paquete(paquete, tamanioPaquete);
	//enviamos

	if (send(socket_cliente, paqueteAEnviar, tamanioPaquete, 0) == -1) {
		free(paqueteAEnviar);
		eliminar_paquete(paquete);
		return COMPONENTE_CAIDO;
	}

	free(paqueteAEnviar);
	eliminar_paquete(paquete);

	return SUCCESS;
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

/*liberarHandshakeMemoria()
 * Parametros:
 * 	-> t_handshake_memoria* ::  memoriaALiberar
 * Descripcion: libera lo que está adentro del handshake.
 * Return:
 * 	-> :: void */
void liberarHandshakeMemoria(t_gossiping* memoriaALiberar) {
	free(memoriaALiberar->ips);
	free(memoriaALiberar->numeros);
	free(memoriaALiberar->puertos);
	free(memoriaALiberar);
	memoriaALiberar = NULL;
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

/* validarValue()
 * Parametros:
 * 	-> char :: valor
 * 	-> int :: tamanio Maximo
 * Descripcion: devuelve si el value corresponde con el tamanio maximo
 * Return:
 * 	->  char* :: valor  */
int validarValue(char* request,char* value, int tamMaximo,t_log* logger) {
	if((strlen(value))<=tamMaximo){
		return EXIT_SUCCESS;
	}
	char* mensajeError= strdup("");
	string_append_with_format(&mensajeError,"%s%s%s%i","No es posible realizar la request: ",request," dado a que el value ingresado supera el tamanio maximo ",tamMaximo);
	log_warning(logger,mensajeError);
	free(mensajeError);
	return NUESTRO_ERROR;
}


