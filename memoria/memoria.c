#include "memoria.h"
#include <stdlib.h>
#include <errno.h>

int main(void) {

	//--------------------------------INICIO DE MEMORIA ---------------------------------------------------------------
	config = leer_config("/home/utnso/tp-2019-1c-bugbusters/memoria/memoria.config");
	logger_MEMORIA = log_create("memoria.log", "Memoria", 1, LOG_LEVEL_DEBUG);
	retardoGossiping = config_get_int_value(config, "RETARDO_GOSSIPING");
	retardoJournal = config_get_int_value(config, "RETARDO_JOURNAL");
	retardoFS = config_get_int_value(config, "RETARDO_FS");
	retardoMemPrincipal =config_get_int_value(config, "RETARDO_MEM");
	memoriasLevantadas = list_create();
	memoriasSeeds = list_create();

	ipFS = strdup(config_get_string_value(config, "IP_FS"));
	puertoFS = strdup(config_get_string_value(config, "PUERTO_FS"));
	pthread_mutex_init(&semMMemoriasLevantadas, NULL);
	pthread_mutex_init(&semMConexionLFS, NULL);
	char** puertosSeeds = config_get_array_value(config, "PUERTO_SEEDS");
	char** ipsSeeds = config_get_array_value(config, "IP_SEEDS");

	puertoMio = strdup(config_get_string_value(config, "PUERTO"));
	ipMia = strdup(config_get_string_value(config, "IP"));
	numerosMio = strdup(config_get_string_value(config, "MEMORY_NUMBER"));
	int i;
	for(i = 0; (ipsSeeds[i] != NULL && puertosSeeds[i] != NULL); i++) {
		config_memoria* memoriaSeed = (config_memoria*) malloc(sizeof(config_memoria));
		memoriaSeed->ip = strdup(ipsSeeds[i]);
		memoriaSeed->puerto = strdup(puertosSeeds[i]);
		memoriaSeed->numero = strdup(""); // no tenemos los numeros de las seeds
		// no hace falta semaforo para la lista de las seeds porque solo le escribe aca
		// y se en gossiping
		list_add(memoriasSeeds, memoriaSeed);
	}
	if (i == 0) {
		log_info(logger_MEMORIA, "No tengo seeds :(");
	}
	liberarArrayDeChar(puertosSeeds);
	liberarArrayDeChar(ipsSeeds);

	//--------------------------------CONEXION CON LFS ---------------------------------------------------------------

	conectarAFileSystem();

	//--------------------------------RESERVAR MEMORIA ---------------------------------------------------------------
	inicializacionDeMemoria();
	log_info(logger_MEMORIA,"INICIO DE MEMORIA");

	//--------------------------------SEMAFOROS-HILOS ----------------------------------------------------------------
	//	SEMAFOROS
	sem_init(&semEnviarMensajeAFileSystem, 0, 0);
	pthread_mutex_init(&semMBitarray, NULL);
	pthread_mutex_init(&semMTablaSegmentos, NULL);
	pthread_mutex_init(&semMMemoriasLevantadas, NULL);
	pthread_mutex_init(&semMConfig, NULL);
	pthread_mutex_init(&semMGossiping, NULL);
	pthread_mutex_init(&semMSleepJournal, NULL);
	pthread_mutex_init(&semMMem, NULL);
	pthread_mutex_init(&semMFS, NULL);
	pthread_mutex_init(&semMJOURNAL, NULL);
	pthread_mutex_init(&semMCONSOLA, NULL);
	pthread_mutex_init(&semMKERNEL, NULL);

	// 	HILOS
	pthread_create(&hiloEscucharMultiplesClientes, NULL, (void*)escucharMultiplesClientes, NULL);
	pthread_create(&hiloHacerGossiping, NULL, (void*)hacerGossiping, NULL);
	pthread_create(&hiloHacerJournal, NULL, (void*) hacerJournal, NULL);
	pthread_create(&hiloCambioEnConfig, NULL, (void*)escucharCambiosEnConfig, NULL);
	pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL);


	pthread_join(hiloCambioEnConfig, NULL);
	pthread_join(hiloHacerGossiping, NULL);
	pthread_join(hiloHacerJournal, NULL);
	pthread_join(hiloLeerDeConsola, NULL);
	log_info(logger_MEMORIA, "Hilo de consola finalizado");
	pthread_join(hiloEscucharMultiplesClientes, NULL);
	log_info(logger_MEMORIA, "Hilo recibir kernels finalizado");



	//-------------------------------- -PARTE FINAL DE MEMORIA---------------------------------------------------------
	liberarEstructurasMemoria();
	liberarMemoria();
	return EXIT_SUCCESS;
}


void inicializacionDeMemoria(){
	//-------------------------------Reserva de memoria-------------------------------------------------------
	memoria = malloc(config_get_int_value(config, "TAM_MEM"));
	marcosTotales = floor(config_get_int_value(config, "TAM_MEM")/(int)(sizeof(uint16_t)+sizeof(unsigned long long)+maxValue));


	//-------------------------------Creacion de structs-------------------------------------------------------
	bitarrayString = string_repeat('0', marcosTotales);
	bitarray= bitarray_create_with_mode(bitarrayString,marcosTotales,LSB_FIRST);
	tablaDeSegmentos = (t_tablaDeSegmentos*)malloc(sizeof(t_tablaDeSegmentos));
	tablaDeSegmentos->segmentos =list_create();
	listaSemSegmentos=list_create();
}

void formatearMemoriasLevantadas(char** puertos, char** ips, char** numeros) {
	// mando la data de mi config
	string_append_with_format(puertos, "%s", puertoMio);
	string_append_with_format(ips, "%s", ipMia);
	string_append_with_format(numeros, "%s", numerosMio);
	int i = 0;
	pthread_mutex_lock(&semMMemoriasLevantadas);
	config_memoria* memoriaAFormatear = (config_memoria*) list_get(memoriasLevantadas, i);
	while(memoriaAFormatear != NULL) {
		string_append_with_format(puertos, ",%s", memoriaAFormatear->puerto);
		string_append_with_format(ips, ",%s", memoriaAFormatear->ip);
		if (memoriaAFormatear->numero != NULL) {

			string_append_with_format(numeros, ",%s", memoriaAFormatear->numero);
		} else {
			string_append_with_format(numeros, ",%s", "");

		}
		i++;
		memoriaAFormatear = (config_memoria*) list_get(memoriasLevantadas, i);
	}
	pthread_mutex_unlock(&semMMemoriasLevantadas);
}

void eliminarMemoria(char* puerto, char* ip) {
	log_warning(logger_MEMORIA, "La memoria %s no está levantada", puerto);
	int esMemoriaAEliminar(config_memoria* memoriaEnLista) {
		return string_equals_ignore_case(memoriaEnLista->ip, ip) &&
				string_equals_ignore_case(memoriaEnLista->puerto, puerto);
	}
	list_remove_and_destroy_by_condition(memoriasLevantadas,(void*)esMemoriaAEliminar, (void*)liberarConfigMemoria);
}

void liberarConfigMemoria(config_memoria* configALiberar) {
	if (configALiberar != NULL) {
		free(configALiberar->ip);
		free(configALiberar->numero);
		free(configALiberar->puerto);
		configALiberar->ip = NULL;
		configALiberar->puerto = NULL;
		configALiberar->numero = NULL;
	}
	free(configALiberar);
	configALiberar = NULL;
}

/* escucharCambiosEnConfig()
 * Parametros:
 * 	-> void
 * Descripcion: es un hilo que escucha cambios en el config y actualiza las variables correspodientes.
 * Return:
 * 	-> :: void  */
void escucharCambiosEnConfig(void) {
	char buffer[BUF_LEN];
	file_descriptor = inotify_init();
	if (file_descriptor < 0) {
		log_error(logger_MEMORIA, "iNotify no se pudo inicializar correctamente");
	}

	watch_descriptor = inotify_add_watch(file_descriptor, "/home/utnso/tp-2019-1c-bugbusters/memoria/memoria.config", IN_MODIFY);
	while(1) {
		int length = read(file_descriptor, buffer, BUF_LEN);
		log_warning(logger_MEMORIA, "Cambió el archivo de config");
		pthread_mutex_lock(&semMConfig);
		config_destroy(config);
		config = leer_config("/home/utnso/tp-2019-1c-bugbusters/memoria/memoria.config");
		pthread_mutex_unlock(&semMConfig);
		if (length < 0) {
			log_error(logger_MEMORIA, "ERROR en iNotify");
		} else {
			if(config_has_property(config,"RETARDO_GOSSIPING") && config_has_property(config,"RETARDO_JOURNAL") && config_has_property(config,"RETARDO_FS") && config_has_property(config,"RETARDO_MEM")){
				pthread_mutex_lock(&semMGossiping);
				retardoGossiping = config_get_int_value(config, "RETARDO_GOSSIPING");
				pthread_mutex_unlock(&semMGossiping);
				pthread_mutex_lock(&semMSleepJournal);
				retardoJournal = config_get_int_value(config, "RETARDO_JOURNAL");
				pthread_mutex_unlock(&semMSleepJournal);
				pthread_mutex_lock(&semMFS);
				retardoFS = config_get_int_value(config, "RETARDO_FS");
				pthread_mutex_unlock(&semMFS);
				pthread_mutex_lock(&semMMem);
				retardoMemPrincipal = config_get_int_value(config, "RETARDO_MEM");
				pthread_mutex_unlock(&semMMem);
			}else{
				log_error(logger_MEMORIA, "Error en iNotify: sus cambios no han sido tomados, vuelva a cambiar el archivo");
			}
		}

		inotify_rm_watch(file_descriptor, watch_descriptor);
		close(file_descriptor);

		file_descriptor = inotify_init();
		if (file_descriptor < 0) {
			log_error(logger_MEMORIA, "iNotify no se pudo inicializar correctamente");
		}

		watch_descriptor = inotify_add_watch(file_descriptor, "/home/utnso/tp-2019-1c-bugbusters/memoria/memoria.config", IN_MODIFY);
	}
}

void hacerGossiping() {
	int retardo;
	int esSeed;
	char* puertosQueTengo;
	char* ipsQueTengo;
	char* numerosQueTengo;
	void gossip(config_memoria* mem) {
		mandarGossiping(mem, esSeed, puertosQueTengo, ipsQueTengo, numerosQueTengo);
	}
	while(1) {
		puertosQueTengo = strdup("");
		ipsQueTengo = strdup("");
		numerosQueTengo = strdup("");
		// primero le pido a mis seeds
		// hacer for que vaya del primer seed al ultimo
		// cuando pido gossiping mando lista actaul
		// copie el codigo de la funcion formatearMemoriasLevantadas para evitar deadlock
		// antes de mandar memorias me fijo si estan levantadas
		string_append_with_format(&puertosQueTengo, "%s", puertoMio);
		string_append_with_format(&ipsQueTengo, "%s", ipMia);
		string_append_with_format(&numerosQueTengo, "%s", numerosMio);
		int i = 0;
		pthread_mutex_lock(&semMMemoriasLevantadas);
		config_memoria* memoriaAFormatear = (config_memoria*) list_get(memoriasLevantadas, i);
		while(memoriaAFormatear != NULL) {
			int conexionChequeo = crearConexion(memoriaAFormatear->ip, memoriaAFormatear->puerto);
			if (conexionChequeo != COMPONENTE_CAIDO) {
				char* ip = strdup(memoriaAFormatear->ip);
				char* puerto = strdup(memoriaAFormatear->puerto);
				char* num;
				if (memoriaAFormatear->numero != NULL) {
					num = strdup(memoriaAFormatear->numero);
				} else {
					num = strdup("");
				}
				pthread_mutex_unlock(&semMMemoriasLevantadas);
				string_append_with_format(&puertosQueTengo, ",%s", puerto);
				string_append_with_format(&ipsQueTengo, ",%s", ip);
				string_append_with_format(&numerosQueTengo, ",%s", num);
				liberar_conexion(conexionChequeo);
			} else {
				eliminarMemoria(memoriaAFormatear->puerto, memoriaAFormatear->ip);
				pthread_mutex_unlock(&semMMemoriasLevantadas);
			}
			i++;
			pthread_mutex_lock(&semMMemoriasLevantadas);
			memoriaAFormatear = (config_memoria*) list_get(memoriasLevantadas, i);
		}
		pthread_mutex_unlock(&semMMemoriasLevantadas);
		esSeed = 1;
		list_iterate(memoriasSeeds, (void*)gossip);
		esSeed = 0;
		pthread_mutex_lock(&semMMemoriasLevantadas);
		// todo: no volver a mandarle a las semillas
		list_iterate(memoriasLevantadas, (void*)gossip);
		pthread_mutex_unlock(&semMMemoriasLevantadas);
		free(puertosQueTengo);
		free(ipsQueTengo);
		free(numerosQueTengo);
		pthread_mutex_lock(&semMGossiping);
		retardo = retardoGossiping;
		pthread_mutex_unlock(&semMGossiping);
		usleep(retardo*1000);
	}
}

void mandarGossiping(config_memoria* memoriaSeed, int vaASerSeed, char* puertosQueTengo, char* ipsQueTengo, char* numerosQueTengo) {
	if (!vaASerSeed) {
		// solo le mando si no es mi semilla porque en tal caso ya le mande
		int esSeed(config_memoria* seed) {
			return string_equals_ignore_case(memoriaSeed->ip, seed->ip) &&
					string_equals_ignore_case(memoriaSeed->puerto, seed->puerto);
		}
		if (list_any_satisfy(memoriasSeeds, (void*)esSeed)) {
			// OJO, solo hacerlo cuando yo pido, no cuando respondo, no llamar a la misma funcion, ADEMAS faltan mutex
			return;
		}
	}
	// si no me puedo conectar, la borro de mi lista de memorias
	int conexionTemporaneaSeed = crearConexion(memoriaSeed->ip, memoriaSeed->puerto);
	if (conexionTemporaneaSeed == COMPONENTE_CAIDO) {
		eliminarMemoria(memoriaSeed->puerto, memoriaSeed->ip);
		return;
	}

	int estadoHandshake = enviarHandshake(MEMORIA, conexionTemporaneaSeed);
	if (estadoHandshake == COMPONENTE_CAIDO) {
		liberar_conexion(conexionTemporaneaSeed);
		eliminarMemoria(memoriaSeed->puerto, memoriaSeed->ip);
		return;
	}

	int estadoOperacion = enviarTipoOperacion(GOSSIPING, conexionTemporaneaSeed);
	if (estadoOperacion == COMPONENTE_CAIDO) {
		liberar_conexion(conexionTemporaneaSeed);
		eliminarMemoria(memoriaSeed->puerto, memoriaSeed->ip);
		return;
	}

	int estadoEnviar = enviarGossiping(puertosQueTengo, ipsQueTengo, numerosQueTengo, 0, conexionTemporaneaSeed);
	if (estadoEnviar == COMPONENTE_CAIDO) {
		liberar_conexion(conexionTemporaneaSeed);
		eliminarMemoria(memoriaSeed->puerto, memoriaSeed->ip);
		return;
	}
	log_warning(logger_MEMORIA, "Tengo %s %s %s", puertosQueTengo, ipsQueTengo, numerosQueTengo);
	log_warning(logger_MEMORIA, "Le voy a mandar a la memoria %s %s", memoriaSeed->ip, memoriaSeed->puerto);
	liberar_conexion(conexionTemporaneaSeed);
}

void agregarMemorias(t_gossiping* gossipingRecibido) {
	if (string_equals_ignore_case(gossipingRecibido->ips, "")) {
		return;
	}
	int i;
	char** ips = string_split(gossipingRecibido->ips, ",");
	char** puertos = string_split(gossipingRecibido->puertos, ",");
	char** numeros = string_split(gossipingRecibido->numeros, ",");
	for(i = 0; ips[i] != NULL; i++) {
		config_memoria* memoriaNueva = (config_memoria*) malloc(sizeof(config_memoria));
		memoriaNueva->ip = strdup(ips[i]);
		memoriaNueva->puerto = strdup(puertos[i]);
		// si no tengo el numero de memoria no me sirve porque el kernel lo necesita
		if (numeros[i] == NULL || string_equals_ignore_case(numeros[i], "")) {
			memoriaNueva->numero = strdup("");
			liberarConfigMemoria(memoriaNueva);
		} else {
			memoriaNueva->numero = strdup(numeros[i]);

			int existeUnaIgual(config_memoria* memoriaAgregada) {
				return string_equals_ignore_case(memoriaAgregada->ip, memoriaNueva->ip) &&
						string_equals_ignore_case(memoriaAgregada->puerto, memoriaNueva->puerto);
			}

			// no me autoguardo en la lista de memorias
			if (string_equals_ignore_case(memoriaNueva->ip, ipMia) && string_equals_ignore_case(memoriaNueva->puerto, puertoMio)) {
				liberarConfigMemoria(memoriaNueva);
			} else {
				pthread_mutex_lock(&semMMemoriasLevantadas);
				if (!list_any_satisfy(memoriasLevantadas, (void*)existeUnaIgual)) {
					// agrego memoria si no existe en mi lista de memorias
					log_info(logger_MEMORIA, "Me llegó una nueva memoria en el gossiping, num: %s %s", memoriaNueva->ip, memoriaNueva->puerto);
					list_add(memoriasLevantadas, memoriaNueva);
					pthread_mutex_unlock(&semMMemoriasLevantadas);
				} else {
					pthread_mutex_unlock(&semMMemoriasLevantadas);
					liberarConfigMemoria(memoriaNueva);
				}
			}
		}
		memoriaNueva = NULL;
	}
	liberarArrayDeChar(ips);
	liberarArrayDeChar(puertos);
	liberarArrayDeChar(numeros);
}


/* obtenerIndiceMarcoDisponible()
 * Parametros:
 * 	->  ::  void
 * Descripcion:Obtiene, en caso de que exista, un marco disponible.
 * 				En caso contrario, devuelve error.
 * Return:
 * 	-> error||indice :: int
 * VALGRIND:: SI */
int obtenerIndiceMarcoDisponible() {
	int index = 0;
	pthread_mutex_lock(&semMBitarray);
	while(index < marcosTotales && bitarray_test_bit(bitarray, index)) index++;
	pthread_mutex_unlock(&semMBitarray);
	if(index >= marcosTotales) {
		index = LRU;
	}
	return index;
}

/* leerDeConsola()
 * Parametros:
 * 	->  ::  void
 * Descripcion: Lee la request de consola.
 * 				Valida el caso de que s ehaya ingresado SALIDA (por lo que setea el flagTerminaHiloMultiplesClientes en 1
 * Return:
 * 	-> codPalabraReservada :: void
 * VALGRIND:: SI */
void leerDeConsola(void){
	char* mensaje;
	log_info(logger_MEMORIA, "Vamos a leer de consola");
	while (1) {
		mensaje = readline(">");
		if (!strcmp(mensaje, "\0")) {
			free(mensaje);
			continue;
		}
		if (string_equals_ignore_case(mensaje, "EXIT")) {
			pthread_cancel(hiloEscucharMultiplesClientes);
			pthread_cancel(hiloCambioEnConfig);
			pthread_cancel(hiloHacerGossiping);
			pthread_cancel(hiloHacerJournal);
			free(mensaje);
			break;
		}
		validarRequest(mensaje);

	}
}

/* validarRequest()
 * Parametros:
 * 	->  mensaje :: char*
 * Descripcion: Cachea si el codigo de validacion encontrado.
 * 				En caso de ser un codigo valido, se proede a interpretar la request y se devuelve EXIT_SUCCESS.
 * 				En caso de error se deuvuelve NUESTRO_ERROR y se logea el mismo.
 * 				En caso de de ser salida, se devuelve SALIDA.
 *
 * Return:
 * 	-> resultadoVlidacion :: int
 * VALGRIND:: NO */
void validarRequest(char* mensaje){
	char** request = string_n_split(mensaje, 2, " ");
	char** requestSeparada= separarRequest(mensaje);
	char* mensajeDeError;
	int mensajeValido = validarMensaje(mensaje, MEMORIA, &mensajeDeError);
	int palabraReservada = obtenerCodigoPalabraReservada(request[0],MEMORIA);
	if(mensajeValido == SUCCESS){
		if(palabraReservada == INSERT && validarValue(mensaje,requestSeparada[3],maxValue,logger_MEMORIA) == NUESTRO_ERROR){
			log_warning(logger_MEMORIA,"Ingrese un valor menor que:  %d", maxValue);
		}else{
			interpretarRequest(palabraReservada, mensaje, CONSOLE,-1);
		}
	}else{
		log_error(logger_MEMORIA, mensajeDeError);
	}
	free(mensaje);
	mensaje=NULL;
	liberarArrayDeChar(request);
	request =NULL;
	liberarArrayDeChar(requestSeparada);
	requestSeparada=NULL;
}

/* conectarAFileSystem()
 * Parametros:
 * 	->  :: void
 * Descripcion: Se crea la conexion con lissandraFileSystem y se realiza el handshake con la misma
 * 				Se recibe de lfs el tam max del value
 * Return:
 * 	-> :: void
 * VALGRIND:: SI */
void conectarAFileSystem() {
	pthread_mutex_lock(&semMConexionLFS);
	conexionLfs = crearConexion(
			ipFS,
			puertoFS);
	pthread_mutex_unlock(&semMConexionLFS);
	int rta = enviarHandshake(MEMORIA, conexionLfs);
	if (rta == COMPONENTE_CAIDO) {
		log_error(logger_MEMORIA, "No está levantado el FS");
		liberar_conexion(conexionLfs);
		conexionLfs = COMPONENTE_CAIDO;
		return;
	}

	t_handshake_rta* handshake_rta = recibirRtaHandshake(conexionLfs, &rta);
	if (handshake_rta->rta == CONEXION_INVALIDA) {
		log_error(logger_MEMORIA, "Este tipo de conexión no es válida");
		pthread_mutex_lock(&semMConexionLFS);
		liberar_conexion(conexionLfs);
		conexionLfs = COMPONENTE_CAIDO;
		pthread_mutex_unlock(&semMConexionLFS);
		return;
	}
	pthread_mutex_lock(&semMConexionLFS);
	handshakeLFS = recibirValueLFS(conexionLfs);
	pthread_mutex_unlock(&semMConexionLFS);
	maxValue= handshakeLFS->tamanioValue;
	log_info(logger_MEMORIA, "SE CONECTO CON LFS");
	log_info(logger_MEMORIA, "Recibi de LFS TAMAÑO_VALUE: %d", handshakeLFS->tamanioValue);
	free(handshakeLFS);
}


/* escucharMultiplesClientes()
 * VALGRIND:: SI */
void escucharMultiplesClientes() {
	int descriptorServidor = iniciar_servidor(puertoMio, ipMia);
	log_info(logger_MEMORIA, "Memoria lista para recibir al kernel");

	fd_set descriptoresDeInteres;
	fd_set setCopia;

	t_paquete* paqueteRecibido;

	FD_ZERO(&descriptoresDeInteres);
	FD_SET(descriptorServidor, &descriptoresDeInteres);
	int descriptorMayor = descriptorServidor;
	while(1) {
		setCopia = descriptoresDeInteres;
		int estadoSelect = select(descriptorMayor + 1, &setCopia, NULL, NULL, NULL);

		if (estadoSelect == -1) {
			log_info(logger_MEMORIA, "Select falló porque %s", strerror(errno));
		}

		int numDescriptor = 0;

		while (numDescriptor <= descriptorMayor) {
			if (FD_ISSET(numDescriptor, &setCopia)) {
				if (numDescriptor == descriptorServidor) {
					int descriptorCliente = esperar_cliente(descriptorServidor);
					if (descriptorCliente == -1) {
						numDescriptor++;
						log_error(logger_MEMORIA, "Hubo un error conectandose");
						continue;
					}
					FD_SET(descriptorCliente, &descriptoresDeInteres);
					if (descriptorCliente > descriptorMayor) {
						descriptorMayor	 = descriptorCliente;
					}
				} else {
					// handshake se puede sacar de aca???
					int codigoOperacion = 0;
					t_handshake* handshake = recibirHandshake(numDescriptor, &codigoOperacion);
					if (codigoOperacion == COMPONENTE_CAIDO) {
						// si fallo el recibir o no es una memoria o kernel
						close(numDescriptor);
						log_info(logger_MEMORIA, "Desconectando al socket %d", numDescriptor);
						FD_CLR(numDescriptor, &descriptoresDeInteres);
						numDescriptor++;
						free(handshake);
						continue;
					}
					if (handshake->tipoComponente == KERNEL || handshake->tipoComponente == MEMORIA) {
						// hacer cambios en gossiping tmb
						codigoOperacion = enviarRtaHandshake(CONEXION_EXITOSA, numDescriptor);
					} else {
						enviarRtaHandshake(CONEXION_INVALIDA, numDescriptor);
						close(numDescriptor);
						log_info(logger_MEMORIA, "Desconectando al socket %d", numDescriptor);
						FD_CLR(numDescriptor, &descriptoresDeInteres);
						numDescriptor++;
						free(handshake);
						continue;
					}
					if (codigoOperacion == COMPONENTE_CAIDO) {
						// si fallo el recibir o no es una memoria o kernel
						close(numDescriptor);
						log_info(logger_MEMORIA, "Desconectando al socket %d", numDescriptor);
						FD_CLR(numDescriptor, &descriptoresDeInteres);
						numDescriptor++;
						free(handshake);
						continue;
					}
					int rtaOperacion;
					t_operacion* operacionARealizar = recibirOperacion(numDescriptor, &rtaOperacion);
					if (rtaOperacion == COMPONENTE_CAIDO) {
						close(numDescriptor);
						log_info(logger_MEMORIA, "Desconectando al socket %d", numDescriptor);
						FD_CLR(numDescriptor, &descriptoresDeInteres);
						numDescriptor++;
						free(operacionARealizar);
						continue;
					}
					if (operacionARealizar->tipo_rol == REQUEST) {

						paqueteRecibido = recibir(numDescriptor); // Recibo de ese cliente en particular
						codigoOperacion = paqueteRecibido->palabraReservada;
						char* request = paqueteRecibido->request;
						log_info(logger_MEMORIA, "Del fd num: %d", numDescriptor);
						if (codigoOperacion == COMPONENTE_CAIDO) {
							close(numDescriptor);
							FD_CLR(numDescriptor, &descriptoresDeInteres);
							log_info(logger_MEMORIA, "Desconectando al socket %d", numDescriptor);
							eliminar_paquete(paqueteRecibido);
							paqueteRecibido=NULL;
							numDescriptor++;
							continue;
						}
						log_info(logger_MEMORIA, "El codigo que recibi es: %d", codigoOperacion);
						interpretarRequest(codigoOperacion,request, ANOTHER_COMPONENT, numDescriptor);
						eliminar_paquete(paqueteRecibido);
						paqueteRecibido=NULL;
					} else if (operacionARealizar->tipo_rol == GOSSIPING) {
						t_gossiping* gossipingRecibido = recibirGossiping(numDescriptor, &codigoOperacion);
						agregarMemorias(gossipingRecibido);
						if (codigoOperacion == COMPONENTE_CAIDO) {
							// llamo a eliminar memoria
							close(numDescriptor);
							FD_CLR(numDescriptor, &descriptoresDeInteres);
							log_info(logger_MEMORIA, "Desconectando al socket %d", numDescriptor);
							liberarHandshakeMemoria(gossipingRecibido);
							numDescriptor++;
							continue;
						}
						if (gossipingRecibido->esDeKernel) {
							char* puertosQueTengo = strdup("");
							char* ipsQueTengo = strdup("");
							char* numerosQueTengo = strdup("");
							formatearMemoriasLevantadas(&puertosQueTengo, &ipsQueTengo, &numerosQueTengo);
							//log_warning(logger_MEMORIA, "formatee %s %s %s", puertosQueTengo, ipsQueTengo, numerosQueTengo);
							enviarGossiping(puertosQueTengo, ipsQueTengo, numerosQueTengo, 0, numDescriptor);

							free(puertosQueTengo);
							free(ipsQueTengo);
							free(numerosQueTengo);
						}
						//enviarGossiping(puertosQueTengo, ipsQueTengo, numerosQueTengo, numDescriptor);
						liberarHandshakeMemoria(gossipingRecibido);
					}
					free(operacionARealizar);
					free(handshake);
				}
			}
			numDescriptor++;
		}
	}
}


/* interpretarRequest()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 * 	-> char* :: request
 * 	-> t_caller :: caller
 * 	-> int :: i (socket_kernel)
 * Descripcion: El coponente de memoria interpreta la reques que le llego desde cualquier caller (kernel o consola)
 * 				y ,segun la palabra reservada,delega al correspondiente procesar
 * Return:
 * 	-> :: void
 * VALGRIND:: EN PROCESO */
void interpretarRequest(int palabraReservada,char* request,t_caller caller, int indiceKernel) {

		consistencia consistenciaMemoria;
		if(caller== ANOTHER_COMPONENT){
			consistenciaMemoria = palabraReservada;
		}else{
			consistenciaMemoria = EC;
		}
		char** requestSeparada = string_n_split(request, 2, " ");
		int codRequest = obtenerCodigoPalabraReservada(requestSeparada[0],MEMORIA);

		log_info(logger_MEMORIA,"entre a interpretarr request");
		switch(codRequest) {

			case SELECT:
				log_debug(logger_MEMORIA, "Me llego un SELECT");
				if(caller== ANOTHER_COMPONENT){
					pthread_mutex_lock(&semMKERNEL);
				}else{
					pthread_mutex_lock(&semMCONSOLA);
				}
				procesarSelect(codRequest, request,consistenciaMemoria, caller, indiceKernel);
				if(caller== ANOTHER_COMPONENT){
					pthread_mutex_unlock(&semMKERNEL);
				}else{
					pthread_mutex_unlock(&semMCONSOLA);
				}
				break;
			case INSERT:
				log_debug(logger_MEMORIA, "Me llego un INSERT");
				if(caller== ANOTHER_COMPONENT){
					pthread_mutex_lock(&semMKERNEL);
				}else{
					pthread_mutex_lock(&semMCONSOLA);
				}
				procesarInsert(codRequest, request,consistenciaMemoria, caller, indiceKernel);
				if(caller== ANOTHER_COMPONENT){
					pthread_mutex_unlock(&semMKERNEL);
				}else{
					pthread_mutex_unlock(&semMCONSOLA);
				}
				break;
			case CREATE:
				log_debug(logger_MEMORIA, "Me llego un CREATE");
				if(caller== ANOTHER_COMPONENT){
					pthread_mutex_lock(&semMKERNEL);
				}else{
					pthread_mutex_lock(&semMCONSOLA);
				}
				procesarCreate(codRequest, request,consistenciaMemoria, caller, indiceKernel);
				if(caller== ANOTHER_COMPONENT){
					pthread_mutex_unlock(&semMKERNEL);
				}else{
					pthread_mutex_unlock(&semMCONSOLA);
				}
				break;
			case DESCRIBE:
				log_debug(logger_MEMORIA, "Me llego un DESCRIBE");
				if(caller== ANOTHER_COMPONENT){
					pthread_mutex_lock(&semMKERNEL);
				}else{
					pthread_mutex_lock(&semMCONSOLA);
				}
				procesarDescribe(codRequest, request,caller,indiceKernel);
				if(caller== ANOTHER_COMPONENT){
					pthread_mutex_unlock(&semMKERNEL);
				}else{
					pthread_mutex_unlock(&semMCONSOLA);
				}
				break;
			case DROP:
				log_debug(logger_MEMORIA, "Me llego un DROP");
				if(caller== ANOTHER_COMPONENT){
					pthread_mutex_lock(&semMKERNEL);
				}else{
					pthread_mutex_lock(&semMCONSOLA);
				}
				procesarDrop(codRequest, request ,consistenciaMemoria, caller, indiceKernel);
				if(caller== ANOTHER_COMPONENT){
					pthread_mutex_unlock(&semMKERNEL);
				}else{
					pthread_mutex_unlock(&semMCONSOLA);
				}
				break;
			case JOURNAL:
				log_debug(logger_MEMORIA, "Me llego un JOURNAL");
				procesarJournal(codRequest, request, caller, indiceKernel,MY_REQUEST);
				break;
			default:
				log_warning(logger_MEMORIA, "No has ingresado una request valida");
				break;
	}
		liberarArrayDeChar(requestSeparada);
		requestSeparada=NULL;

}

/*intercambiarConFileSystem()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 * 	-> char* ::  request
 * Descripcion: Permite enviar la request (recibida por parametro) a LFS y espera la respuesta de ella.
 * Return:
 * 	-> paqueteRecibido:: char*
 * VALGRIND:: NO */
int intercambiarConFileSystem(cod_request palabraReservada, char* request,t_paquete** paqueteRecibido, t_caller caller, int indiceKernel){
	pthread_mutex_lock(&semMConexionLFS);
	int conexionAux = conexionLfs;
	pthread_mutex_unlock(&semMConexionLFS);
	if (conexionAux == COMPONENTE_CAIDO) {
		pthread_mutex_lock(&semMConexionLFS);
		conexionLfs = crearConexion(
				ipFS,
				puertoFS);
		pthread_mutex_unlock(&semMConexionLFS);
		t_handshake_lfs* handshakeLFS2 = recibirValueLFS(conexionLfs);
		free(handshakeLFS2);
		pthread_mutex_lock(&semMConexionLFS);
		conexionAux = conexionLfs;
		pthread_mutex_unlock(&semMConexionLFS);
		if (conexionAux == COMPONENTE_CAIDO) {
			return FAILURE;
		}
		int resultadoLFS = enviar(palabraReservada, request, conexionAux);
		if(resultadoLFS == COMPONENTE_CAIDO) {
			liberar_conexion(conexionAux);
			pthread_mutex_lock(&semMConexionLFS);
			conexionLfs = COMPONENTE_CAIDO;
			pthread_mutex_unlock(&semMConexionLFS);
			return FAILURE;
		} else {
			log_info(logger_MEMORIA, "SE CONECTO CON LFS");
			pthread_mutex_lock(&semMFS);
			int retardoFileSystem=retardoFS;
			pthread_mutex_unlock(&semMFS);
			usleep(retardoFileSystem*1000);
			*paqueteRecibido = recibir(conexionAux);
			return SUCCESS;
		}
	} else {
		int resultadoLFS = enviar(palabraReservada, request, conexionAux);
		if(resultadoLFS == COMPONENTE_CAIDO) {
			liberar_conexion(conexionAux);
			pthread_mutex_lock(&semMConexionLFS);
			conexionLfs = COMPONENTE_CAIDO;
			pthread_mutex_unlock(&semMConexionLFS);
			return FAILURE;
		} else {
			pthread_mutex_lock(&semMFS);
			int retardoFileSystem=retardoFS;
			pthread_mutex_unlock(&semMFS);
			usleep(retardoFileSystem*1000);
			*paqueteRecibido = recibir(conexionAux);
			if ((*paqueteRecibido)->palabraReservada == COMPONENTE_CAIDO) {
				pthread_mutex_lock(&semMConexionLFS);
				conexionLfs = COMPONENTE_CAIDO;
				pthread_mutex_unlock(&semMConexionLFS);
			}
			return SUCCESS;
		}
	}
}

/*procesarSelect()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 * 	-> char* ::  request
 * 	-> t_caller :: caller
 * 	-> int :: i (socket_kernel)
 * Descripcion: En caso de que exista, permite obtener el valor de la key consultada de una tabla.
 * 				SC || SHC : siempre consulta la existencia de la tabla y la key consultada a LFS. Y,en caso de que alli
 * 							no exista, se informa ello.
 * 							En caso de que exista, guardamos el resultado devuelto por LFS en cache (que nos sirve para
 * 							el insert).
 * 				EC : -Revisa si la tabla y value se encuentran en memoria.E n caso de encontrarlo en cache, devuelve el
 * 							value asosciado.
 * 							En caso de no existir la key o value, realiza procedimiento de SC||SHC
 * Return:
 * 	-> :: void
 * VALGRIND:: NO */
void procesarSelect(cod_request palabraReservada, char* request,consistencia consistenciaMemoria,t_caller caller, int indiceKernel) {

	t_paquete* valorDeLFS;
	t_elemTablaDePaginas* elementoEncontrado;
	char* pathSegmento=strdup("");

	int resultadoCache;
	int resultadoLFS;
	int rtaGuardarEnMemoria;

	if(consistenciaMemoria == EC || caller == CONSOLE){		// en caso de no existir el segmento o la tabla en MEMORIA, se lo solicta a LFS
		t_paquete* valorEncontrado;
		resultadoCache= estaEnMemoria(palabraReservada, request,&valorEncontrado,&elementoEncontrado,&pathSegmento);
		if(resultadoCache == EXIT_SUCCESS ) {
			log_info(logger_MEMORIA, "LO ENCONTRE EN CACHEE!");
			actualizarTimestamp(elementoEncontrado->marco);
			unlockSemSegmento(pathSegmento);
			enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, valorEncontrado,caller, indiceKernel);
		} else {// en caso de no existir el segmento o la tabla en MEMORIA, se lo solicta a LFS
			log_info(logger_MEMORIA,"ME LO TIENE QUE DECIR LFS");
			resultadoLFS = intercambiarConFileSystem(palabraReservada,request,&valorDeLFS, caller, indiceKernel);

			if(resultadoLFS == -1 || valorDeLFS->palabraReservada == COMPONENTE_CAIDO){
				valorDeLFS = malloc(sizeof(t_paquete));
				valorDeLFS->request=strdup("FALLO CONEXION LFS");
				valorDeLFS->tamanio=(sizeof(valorDeLFS->request));
				//rtaGuardarEnMemoria =guardarRespuestaDeLFSaMemoria(valorDeLFS, resultadoCache);
				enviarAlDestinatarioCorrecto(COMPONENTE_CAIDO, COMPONENTE_CAIDO,request, valorDeLFS,caller,indiceKernel);
			}else{
				if(rtaGuardarEnMemoria == MEMORIA_FULL){
					valorDeLFS->request=strdup("MEMORIA FULL.Debe realizarse JOURNAL");
					valorDeLFS->tamanio=(sizeof(valorDeLFS->request));
					rtaGuardarEnMemoria =guardarRespuestaDeLFSaMemoria(valorDeLFS, resultadoCache);
					enviarAlDestinatarioCorrecto(palabraReservada,MEMORIA_FULL,request, valorDeLFS,caller,indiceKernel);
				}else{
					rtaGuardarEnMemoria =guardarRespuestaDeLFSaMemoria(valorDeLFS, resultadoCache);
					enviarAlDestinatarioCorrecto(palabraReservada, valorDeLFS->palabraReservada,request, valorDeLFS,caller,indiceKernel);
				}
			}
		}

	}else if(consistenciaMemoria==SC || consistenciaMemoria == SHC){
		log_info(logger_MEMORIA,"ME LO TIENE QUE DECIR LFS");
		int resultadoLFS = intercambiarConFileSystem(palabraReservada,request,&valorDeLFS, caller, indiceKernel);
		if(resultadoLFS == FAILURE || valorDeLFS->palabraReservada == COMPONENTE_CAIDO){
			valorDeLFS = malloc(sizeof(t_paquete));
			valorDeLFS->request=strdup("FALLO CONEXION LFS");
			valorDeLFS->tamanio=(sizeof(valorDeLFS->request));
			enviarAlDestinatarioCorrecto(palabraReservada, valorDeLFS->palabraReservada,request, valorDeLFS,caller,indiceKernel);
		}else{
			enviarAlDestinatarioCorrecto(palabraReservada, valorDeLFS->palabraReservada,request, valorDeLFS,caller,indiceKernel);
		}

	}else{
		log_info(logger_MEMORIA, "NO se le ha asignado un tipo de consistencia a la memoria, por lo que no puede responder la consulta: ", request);

	}
	free(pathSegmento);
	pathSegmento=NULL;
}

/*estaEnMemoria()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 * 	-> char* :: request
 * 	-> t_paquete** :: valorEncontrado- se modifica por referencia
 * 	-> t_elemTablaDePaginas** :: elemento (que contiene el puntero a la pag econtrada)- se modifica por referecia.
 * Descripcion: Revisa si la tabla y key solicitada se encuentran que cache.
 * 				En caso de que sea cierto, guarda valorEncontrado y elementoEncontrado; devolviendo operacion exitosa.
 * 				En caso de que no exista la tabla||key devuelve el error correspondiente.
 * Return:
 * 	-> int :: resultado de la operacion
 * 	VALGRIND :NO */
int estaEnMemoria(cod_request palabraReservada, char* request,t_paquete** valorEncontrado,t_elemTablaDePaginas** elementoEncontrado,char** pathSegmento){
	t_elemTablaDePaginas* elementoDePagEnCache;
	t_paquete* paqueteAuxiliar;
	pthread_mutex_lock(&semMMem);
	int retardoMem=retardoMemPrincipal;
	pthread_mutex_unlock(&semMMem);
	char** parametros = separarRequest(request);
	char* segmentoABuscar=strdup(parametros[1]);
	uint16_t keyABuscar= convertirKey(parametros[2]);


	int hallarSegmento(t_segmento* segmento){
		return string_equals_ignore_case(segmento->path, segmentoABuscar);
	}

	pthread_mutex_lock(&semMTablaSegmentos);
	t_segmento* segmentoEnCache = list_find(tablaDeSegmentos->segmentos,(void*)hallarSegmento);

	if(segmentoEnCache!=NULL){
		*pathSegmento=strdup(segmentoEnCache->path);
		pthread_mutex_unlock(&semMTablaSegmentos);
	}else{
		pthread_mutex_unlock(&semMTablaSegmentos);
	}
	usleep(retardoMem*1000);

	if(segmentoEnCache!= NULL){
		lockSemSegmento(segmentoEnCache->path);

		int encontrarElemTablaDePag(t_elemTablaDePaginas* elemDePagina){
			return (elemDePagina->marco->key == keyABuscar);
		}

		elementoDePagEnCache= list_find(segmentoEnCache->tablaDePagina,(void*)encontrarElemTablaDePag);
		if(elementoDePagEnCache != NULL && palabraReservada == SELECT){
			paqueteAuxiliar=malloc(sizeof(t_paquete));
			paqueteAuxiliar->palabraReservada=SUCCESS;
			char* requestAEnviar= strdup("");
			string_append_with_format(&requestAEnviar,"%s%s%s%s%c%s%c",segmentoABuscar," ",parametros[2]," ",'"',elementoDePagEnCache->marco->value,'"');
			paqueteAuxiliar->request = strdup(requestAEnviar);
			paqueteAuxiliar->tamanio=sizeof(maxValue);

			free(requestAEnviar);
			requestAEnviar=NULL;
			*elementoEncontrado=elementoDePagEnCache;
			*valorEncontrado = paqueteAuxiliar;
			free(segmentoABuscar);
			segmentoABuscar=NULL;
			liberarArrayDeChar(parametros);
			parametros=NULL;
			return EXIT_SUCCESS;
		}else if (elementoDePagEnCache != NULL){ // es el mismo caso que el anterior, solo q para el insert (ya q no necesita armar ese paquete
			*elementoEncontrado=elementoDePagEnCache;
			free(segmentoABuscar);
			segmentoABuscar=NULL;
			liberarArrayDeChar(parametros);
			parametros=NULL;
			return EXIT_SUCCESS;
		}else{
			unlockSemSegmento(segmentoEnCache->path);
			free(segmentoABuscar);
			segmentoABuscar=NULL;
			liberarArrayDeChar(parametros);
			parametros=NULL;
			return KEYINEXISTENTE;
		}
	}else{
		free(segmentoABuscar);
		segmentoABuscar=NULL;
		liberarArrayDeChar(parametros);
		parametros=NULL;
		return SEGMENTOINEXISTENTE;
	}

}


/*encontrarSegmento()
 * Parametros:
 * 	-> char* ::segmentoABuscar
 * Descripcion: busca entre los segmentos existentes en cache y, en caso de existir el segmentoSolicitado,
 * 				devuelve un puntero al mismo.
 * Return:
 * 	-> t_tablaDePaginas* :: segmentoEncontrado
 * VALGRIND:: SI */
t_segmento* encontrarSegmento(char* segmentoABuscar){
	pthread_mutex_lock(&semMMem);
	int retardoMem=retardoMemPrincipal;
	pthread_mutex_unlock(&semMMem);

	int encontrarTabla(t_segmento* segmento){
			return string_equals_ignore_case(segmento->path, segmentoABuscar);
	}
	pthread_mutex_lock(&semMTablaSegmentos);
	t_segmento* segmentoEncontrado= list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);
	pthread_mutex_unlock(&semMTablaSegmentos);


	usleep(retardoMem*1000);

	return segmentoEncontrado;
}

void lockSemSegmento(char* pathSegmento){
	int semCoincideCon(t_semSegmento* semSegmento){
		return string_equals_ignore_case(pathSegmento,semSegmento->path);
	}
	pthread_mutex_lock(&semMListSemSegmentos);
	t_semSegmento* semSegmentoEncontrado =list_find(listaSemSegmentos,(void*)semCoincideCon);
	if(semSegmentoEncontrado!=NULL){
		pthread_mutex_unlock(&semMListSemSegmentos);
		pthread_mutex_lock(semSegmentoEncontrado->sem);
	}else{
		pthread_mutex_unlock(&semMListSemSegmentos);
	}
}

void unlockSemSegmento(char* pathSegmento){
	int semCoincideCon(t_semSegmento* semSegmento){
		return string_equals_ignore_case(pathSegmento,semSegmento->path);
	}
	pthread_mutex_lock(&semMListSemSegmentos);
	t_semSegmento* semSegmentoEncontrado =list_find(listaSemSegmentos,(void*)semCoincideCon);
	if(semSegmentoEncontrado!=NULL){
		pthread_mutex_unlock(&semMListSemSegmentos);
		pthread_mutex_unlock(semSegmentoEncontrado->sem);
	}else{
		pthread_mutex_unlock(&semMListSemSegmentos);
	}
}

/* enviarAlDestinatarioCorrecto()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 * 	-> int :: codResultado(de la request solictada)
 * 	-> char* ::  request
 * 	-> t_paquete* :: valorAEnviar
 * 	-> t_caller :: caller
 * 	-> int :: i (socketKernel)
 * Descripcion: En caso de que la request haya llegado desde kernel, le envia la respuesta a el.
 * 				En caso de que haya llegado por consola, muestra el resultado de la request por la misma.
 * Return:
 * 	-> :: void
 * VALGRIND:: NO*/
 void enviarAlDestinatarioCorrecto(int palabraReservada,int codResultado,char* request,t_paquete* valorAEnviar,t_caller caller, int indiceKernel){
	 char *errorDefault= strdup("");
	 int status;
	 switch(caller){
	 	 case(ANOTHER_COMPONENT):
	 		log_info(logger_MEMORIA, valorAEnviar->request);
			status = enviar(codResultado, valorAEnviar->request, indiceKernel);
			eliminar_paquete(valorAEnviar);
			valorAEnviar=NULL;
			if (status == COMPONENTE_CAIDO) {
				log_error(logger_MEMORIA, "Se cayó Kernel y no le puedo mandar la rta");
			}
			break;
	 	 case(CONSOLE):
	 		mostrarResultadoPorConsola(palabraReservada, codResultado,request, valorAEnviar);
	 	 	eliminar_paquete(valorAEnviar);
	 	 	valorAEnviar=NULL;
	 	 	break;
	 	 default:
	 		string_append_with_format(&errorDefault, " No se ha encontrado a quien devolver la request realizada %s",request);
	 		log_info(logger_MEMORIA,errorDefault);
	 		break;
	}
	free(errorDefault);
	errorDefault=NULL;
}

/* mostrarResultadoPorConsola()
  * Parametros:
  * 	-> cod_request :: palabraReservada
  * 	-> int :: codResultado(de la request solictada)
  * 	-> char* ::  request
  * 	-> t_paquete* :: valorAEnviar
  * Descripcion: Muestra por consola el mensaje a la request hecha, ya se un resultado exitoso o erroneo.
  * Return:
  * 	-> :: void
  * VALGRIND:: SI */
 void mostrarResultadoPorConsola(int palabraReservada, int codResultado,char* request,t_paquete* valorAEnviar){
	 char* respuesta= strdup("");
	 char* error=strdup("");
	 char** requestSeparada=separarRequest(request);
	 char* valorEncontrado;

	 switch(palabraReservada){

	 	 case(SELECT):	{
	 		char** valorAEnviarSeparado=separarRequest(valorAEnviar->request);

	 		if(codResultado == SUCCESS){
		 		valorEncontrado = strdup(valorAEnviarSeparado[2]);
				string_append_with_format(&respuesta, "%s%s%s%s","La respuesta a la request: ",request," es: ", valorEncontrado);
				log_info(logger_MEMORIA,respuesta);
	 		}else{
	 			switch(codResultado){
					case(KEY_NO_EXISTE):
						string_append_with_format(&error, "%s%s%s","La respuesta a: ",request," no es valida, KEY INEXISTENTE");
						log_info(logger_MEMORIA,error);
						break;
					case(TABLA_NO_EXISTE):
						string_append_with_format(&error, "%s%s%s","La respuesta a: ",request," no es valida, TABLA INEXISTENTE");
						log_info(logger_MEMORIA,error);
						break;
					default:
						log_info(logger_MEMORIA,"No se ha podido encontrar respuesta a la request",request);
						break;
	 			}
			}
			free(respuesta);
			respuesta = NULL;
			free(error);
			error = NULL;
			liberarArrayDeChar(valorAEnviarSeparado);
			valorAEnviarSeparado = NULL;
			liberarArrayDeChar(requestSeparada);
			requestSeparada = NULL;
			break;
	 	 }

		case(INSERT):	{
			char** valorAEnviarSeparado = separarRequest(valorAEnviar->request);
			if(codResultado == SUCCESS){
				string_append_with_format(&respuesta, "%s%s%s","La request: ",request," se ha realizado con exito");
				log_info(logger_MEMORIA,respuesta);
	 			free(respuesta);
	 			respuesta=NULL;
	 			free(error);
	 			error=NULL;
	 			liberarArrayDeChar(valorAEnviarSeparado);
	 			valorAEnviarSeparado=NULL;
	 			liberarArrayDeChar(requestSeparada);
	 			requestSeparada=NULL;
			}else if(codResultado == MEMORIA_FULL){
				string_append_with_format(&respuesta, "%s%s%s","La request: ",request," NO ha podido realizarse.La memoria se encuentra FULL");
				log_info(logger_MEMORIA,respuesta);
				free(respuesta);
				respuesta=NULL;
				free(error);
				error=NULL;
				liberarArrayDeChar(valorAEnviarSeparado);
				valorAEnviarSeparado=NULL;
				liberarArrayDeChar(requestSeparada);
				requestSeparada=NULL;
			}else{
				string_append_with_format(&error, "%s%s%s","La request: ",request," no ha podido realizarse, TABLA INEXISTENTE");
				log_info(logger_MEMORIA,error);
				free(respuesta);
				respuesta=NULL;
				free(error);
				error=NULL;
				liberarArrayDeChar(valorAEnviarSeparado);
				valorAEnviarSeparado=NULL;
				liberarArrayDeChar(requestSeparada);
				requestSeparada=NULL;
			}
			break;
		}

		case(CREATE):
			if(codResultado == SUCCESS || codResultado == TABLA_EXISTE){
				string_append_with_format(&respuesta, "%s%s%s","La request: ",request," se ha realizado con exito");
				log_info(logger_MEMORIA,respuesta);
	 			free(respuesta);
	 			respuesta=NULL;
	 			free(error);
	 			error=NULL;
	 			liberarArrayDeChar(requestSeparada);
	 			requestSeparada=NULL;

			}else{
				string_append_with_format(&error, "%s%s%s","La request: ",request," no ha podido realizarse");
				log_info(logger_MEMORIA,error);
	 			free(respuesta);
	 			respuesta=NULL;
	 			free(error);
	 			error=NULL;
	 			liberarArrayDeChar(requestSeparada);
	 			requestSeparada=NULL;
			}
			break;

	 	case(DESCRIBE):
	 		if(codResultado == SUCCESS){
				string_append_with_format(&error, "%s%s%s","La request: ",request," se ha realizado con exito");
				log_info(logger_MEMORIA,error);
	 		}else{
				string_append_with_format(&error, "%s%s%s","La request: ",request," no ha podido realizarse");
				log_info(logger_MEMORIA,error);
	 		}
			free(respuesta);
			respuesta=NULL;
			free(error);
			error=NULL;
			liberarArrayDeChar(requestSeparada);
			requestSeparada=NULL;
			break;

	 	 case(DROP):
			if(codResultado == SUCCESS){
				string_append_with_format(&error, "%s%s%s","La request: ",request," se ha realizado con exito");
				log_info(logger_MEMORIA,error);
			}else{
				string_append_with_format(&error, "%s%s%s","La request: ",request," no ha podido realizarse");
				log_info(logger_MEMORIA,error);
			}
			free(respuesta);
			respuesta=NULL;
			free(error);
			error=NULL;
			liberarArrayDeChar(requestSeparada);
			requestSeparada=NULL;
	 	 	break;

	 	 case(JOURNAL):
			if(codResultado == TRUE){
				string_append_with_format(&error, "%s%s%s","La request: ",request,valorAEnviar->request);
				log_info(logger_MEMORIA,error);
			}else if (codResultado ==JOURNALTIME){
				string_append_with_format(&error, "%s%s%s","La request: ",request,valorAEnviar->request);
				log_info(logger_MEMORIA,error);
			}else{
				string_append_with_format(&error, "%s%s%s","La request: ",request,valorAEnviar->request);
				log_info(logger_MEMORIA,error);
			}
			free(respuesta);
			respuesta=NULL;
			free(error);
			error=NULL;
			liberarArrayDeChar(requestSeparada);
			requestSeparada=NULL;
	 	 	break;

	 	 case(COMPONENTE_CAIDO):
	 		log_error(logger_MEMORIA,"ERROR: El LFS se encuentra desconectado");
	 		string_append_with_format(&error, "%s%s%s","La request ",request," no a podido realizarse");
	 		log_info(logger_MEMORIA,error);
	 		break;

	 	 default:
			log_info(logger_MEMORIA,"MEMORIA NO LO SABE RESOLVER AUN, PERO TE INVITO A QUE LO HAGAS VOS :)");
 			free(respuesta);
 			respuesta=NULL;
 			free(error);
 			error=NULL;
 			liberarArrayDeChar(requestSeparada);
 			requestSeparada=NULL;
 			break;
		}
}

 /* guardarRespuestaDeLFSaCACHE()
   * Parametros:
   * 	-> t_paquete* :: nuevoPaquete
   * 	-> t_erroresCache :: tipoError
   * Descripcion: Guarda el resultado devuelto por LFS a cache.
   * 			-En el caso de de que ya existia la key, se actualiza la misma
   * 			-En el caso de KEYIEXISTENTE= se debe solicitar PAGINA LIBRE:
   * 				*si la hay,se guarda una nueva pag con los valores obtenido en el
   * 					segmento(Que este si existia en cache).
   * 				*Si NO hay espacio libre => ALGORITMO DE REEMPLAZO (Y si MEMORIA == FULL) =>JOURNALING
   * 			-En el caso de SEGMENTOINEXISTENTE: se debe agregar un nuevo segmento y solictar PAGINA LIBRE
   * 				(siguiendo la logica de keyinexistente)
   * Return:
   * 	-> :: void
   * VALGRIND:: NO*/
 int guardarRespuestaDeLFSaMemoria(t_paquete* nuevoPaquete,t_erroresMemoria tipoError){
	 int memoriaSuficiente=SUCCESS;
 	if(nuevoPaquete->palabraReservada == SUCCESS){
 		log_info(logger_MEMORIA,"ENTRO A DONDE NO DEBIA");
		pthread_mutex_lock(&semMMem);
 		int retardoMem=retardoMemPrincipal;
		pthread_mutex_unlock(&semMMem);
 		char** requestSeparada= separarRequest(nuevoPaquete->request);
 		char* tabla= strdup(requestSeparada[0]);
 		uint16_t nuevaKey= convertirKey(requestSeparada[1]);
 		char* nuevoValor= strdup(requestSeparada[2]);
 		unsigned long long nuevoTimestamp;
 		convertirTimestamp(requestSeparada[3],&nuevoTimestamp);//no chequeo, viene de LFS
 		t_marco* pagLibre = NULL;
 		int index= obtenerPaginaDisponible(&pagLibre);
 		if(index == LRU){
 			t_elemTablaDePaginas* elementoAInsertar=correrAlgoritmoLRU();
 			if (elementoAInsertar!=NULL){
 				if (tipoError == KEYINEXISTENTE) {
 					t_segmento* tablaBuscada;
 					usleep(retardoMem*1000);

 					modificarElem(&elementoAInsertar,nuevoTimestamp,nuevaKey,nuevoValor,SINMODIFICAR);

 					tablaBuscada = encontrarSegmento(tabla);
 					if(tablaBuscada!=NULL){
 						lockSemSegmento(tabla);
 						list_add(tablaBuscada->tablaDePagina,elementoAInsertar);//crearElementoEnTablaDePagina(elementoAInsertar->numeroDePag,elementoAInsertar->marco, nuevaKey,nuevoValor, nuevoTimestamp,SINMODIFICAR));
 						unlockSemSegmento(tabla);
 					}else{
 						//TODO nose si hace falta
 					}

 					memoriaSuficiente= SUCCESS;
 				} else if (tipoError == SEGMENTOINEXISTENTE) {
 					t_segmento* nuevoSegmento = (t_segmento*)malloc(sizeof(t_segmento));

 					usleep(retardoMem*1000);

 					modificarElem(&elementoAInsertar,nuevoTimestamp,nuevaKey,nuevoValor,SINMODIFICAR);

 					crearSegmento(nuevoSegmento, tabla);
 					lockSemSegmento(tabla);
 					list_add(nuevoSegmento->tablaDePagina,elementoAInsertar);
 					unlockSemSegmento(tabla);
 					pthread_mutex_lock(&semMTablaSegmentos);
 					list_add(tablaDeSegmentos->segmentos, nuevoSegmento);
 					pthread_mutex_unlock(&semMTablaSegmentos);

 					memoriaSuficiente= SUCCESS;

 				}
 			} else {

					memoriaSuficiente= MEMORIA_FULL;
 			}
 		}else{
 			if(tipoError== KEYINEXISTENTE){
 				t_segmento* tablaBuscada= encontrarSegmento(tabla);

 				usleep(retardoMem*1000);

				if(tablaBuscada!=NULL){
					lockSemSegmento(tabla);
					list_add(tablaBuscada->tablaDePagina,crearElementoEnTablaDePagina(index,pagLibre,nuevaKey, nuevoValor,nuevoTimestamp,SINMODIFICAR));
					unlockSemSegmento(tabla);
				}else{
					//TODO nose si hace falta
				}

				memoriaSuficiente = SUCCESS;

 			}else if(tipoError == SEGMENTOINEXISTENTE){
 				t_segmento* nuevoSegmento = (t_segmento*)malloc(sizeof(t_segmento));
 				crearSegmento(nuevoSegmento,tabla);

				usleep(retardoMem*1000);

				lockSemSegmento(tabla);
 				list_add(nuevoSegmento->tablaDePagina,crearElementoEnTablaDePagina(index,pagLibre,nuevaKey,nuevoValor,nuevoTimestamp,SINMODIFICAR));
				unlockSemSegmento(tabla);
 				pthread_mutex_lock(&semMTablaSegmentos);
 				list_add(tablaDeSegmentos->segmentos,nuevoSegmento);
 				pthread_mutex_unlock(&semMTablaSegmentos);

				memoriaSuficiente=  SUCCESS;

 			}
 		}
 	liberarArrayDeChar(requestSeparada);
 	requestSeparada=NULL;
 	free(tabla);
 	tabla=NULL;
 	free(nuevoValor);
 	nuevoValor=NULL;
 	}

 	return memoriaSuficiente;
 }

 void modificarElem(t_elemTablaDePaginas** elementoAInsertar,unsigned long long nuevoTimestamp, uint16_t nuevaKey,char* nuevoValor,t_flagModificado flag){
	 (*elementoAInsertar)->modificado=flag;
	 (*elementoAInsertar)->marco->key=nuevaKey;
	 (*elementoAInsertar)->marco->timestamp=obtenerHoraActual();
	 strcpy((*elementoAInsertar)->marco->value,nuevoValor);
	 pthread_mutex_lock(&semMBitarray);
	 bitarray_set_bit(bitarray,(*elementoAInsertar)->numeroDePag);
	 pthread_mutex_unlock(&semMBitarray);

 }

/* procesarInsert()
 * Parametros:
 * 	-> cod_request :: palabraReservada
 *	-> char* :: request
 *	-> t_caller :: caller
 *	-> int :: i (socket kernel)
 * Descripcion: EC:
 * 					1)se va a fijar si EXISTE SEGMENTO Y EXISTE KEY en memoria.
 	 	 	 	 	 si es correcto , se ACTUALIZA la pag (valor y timestamp)
 * 					2)si no encuentra NO EXISTE KEY Pero si EXISTE SEGMENTO.Solicitar PAGINA LIBRE:
 * 					*si la hay,se guarda una nueva pag con los valores obtenido en el
 * 						segmento existente.
 * 					*Si NO hay espacio libre => ALGORITMO DE REEMPLAZO (Y si MEMORIA == FULL) =>JOURNALING
 * 					3)Si NO EXISTE SEGMENTO,solicita un segmento y solictar PAGINA LIBRE(siguiendo la logica de keyinexistente)
 *
 *				SC || SHC:
 *					1)Le pregunta  LFS si existe la tabla en la que se desea insertar.
 *					2)Si no existe, se notifica error al destinatario correspondiente
 *					3)Si existe en lisandra=>se inserta alli.
 * Return:
 * 	-> :: void
 * 	VALGRIND :: NO*/
void procesarInsert(cod_request palabraReservada, char* request,consistencia consistenciaMemoria, t_caller caller, int indiceKernel) {
		t_elemTablaDePaginas* elementoEncontrado= NULL;
		char** requestSeparada = separarRequest(request);
		char* pathSegmento=strdup("");
		if(caller == ANOTHER_COMPONENT && validarValue(request,requestSeparada[3],maxValue,logger_MEMORIA) == NUESTRO_ERROR){
			log_warning(logger_MEMORIA,"Ingrese un valor menor que:  %d", maxValue);
			t_paquete* insertALFS=(t_paquete*)malloc(sizeof(t_paquete));
			insertALFS->palabraReservada=palabraReservada;
			insertALFS->request=strdup("Request invalida, se esta superando el maximo");
			insertALFS->tamanio=sizeof(insertALFS->request);
			enviarAlDestinatarioCorrecto(palabraReservada,VALUE_INVALIDO,request,insertALFS,caller,indiceKernel);
			free(pathSegmento);
			pathSegmento=NULL;
		}else{
			if(consistenciaMemoria == EC || caller == CONSOLE){
				int resultadoCache= estaEnMemoria(palabraReservada, request,NULL,&elementoEncontrado,&pathSegmento);
				insertar(resultadoCache,palabraReservada,request,elementoEncontrado,caller,indiceKernel,pathSegmento);
				free(pathSegmento);
				pathSegmento=NULL;

			}else if(consistenciaMemoria == SC || consistenciaMemoria == SHC){
				free(pathSegmento);
				pathSegmento=NULL;
				t_paquete* insertALFS;
				string_append_with_format("%s%s%llu",request," ",obtenerHoraActual());
				int resultadoLFS =  intercambiarConFileSystem(palabraReservada,request, &insertALFS, caller, indiceKernel);

				if(resultadoLFS == -1 || insertALFS->palabraReservada == COMPONENTE_CAIDO){
					insertALFS = malloc(sizeof(t_paquete));
					insertALFS->request=strdup("FALLO CONEXION LFS");
					insertALFS->tamanio=(sizeof(insertALFS->request));
					enviarAlDestinatarioCorrecto(COMPONENTE_CAIDO, COMPONENTE_CAIDO,request, insertALFS,caller,indiceKernel);
				}else{
					if(insertALFS->palabraReservada== EXIT_SUCCESS){
						enviarAlDestinatarioCorrecto(palabraReservada,SUCCESS,request,insertALFS,caller,indiceKernel);
					}else{
						enviarAlDestinatarioCorrecto(palabraReservada,insertALFS->palabraReservada,request,insertALFS,caller,indiceKernel);
					}
				}

			}else{
				free(pathSegmento);
				pathSegmento=NULL;
				log_info(logger_MEMORIA, "NO se le ha asignado un tipo de consistencia a la memoria, por lo que no puede responder la consulta: ", request);
			}
		}
		liberarArrayDeChar(requestSeparada);
		requestSeparada=NULL;
}

/* insertar()
 * Parametros:
 * 	-> int :: resultadoCache
 * 	-> cod_request :: palabraReservada
 *	-> char* :: request
 *	-> t_elemTablaDePaginas* :: elementoEncontrado
 *	-> t_caller :: caller
 *	-> int :: i (socket kernel)
 * Descripcion: Dependiendo del resultado arrojado por estaEnCache, inserta en memoria.
 * 				Si EXISTE SEGMENTO Y EXISTE VALUR => ACTUALIZA
 * 				Si EXISTE SEGMENTO Y NO EXISTE KEY => solicita pagina libre (+algoritmo de reemplazo+jounaling en caso de ser necesario)
 * 				SI NO EXISTE LE SEGMENTO => se solicita agregar un nuevo segmento y se siguen los pasos de keyinexistente
 * Return:
 * 	-> :: void
 * 	VALGRIND :: NO*/
void insertar(int resultadoCache,cod_request palabraReservada,char* request,t_elemTablaDePaginas* elementoEncontrado,t_caller caller, int indiceKernel,char* pathSegmento){
	t_paquete* paqueteAEnviar;

	char** requestSeparada = separarRequest(request);
	char* nuevaTabla = strdup(requestSeparada[1]);
	char* nuevaKeyChar = strdup(requestSeparada[2]);
	int nuevaKey = convertirKey(nuevaKeyChar);
	char* nuevoValor = strdup(requestSeparada[3]);
	unsigned long long nuevoTimestamp;

	pthread_mutex_lock(&semMMem);
	int retardoMem=retardoMemPrincipal;
	pthread_mutex_unlock(&semMMem);

	if(requestSeparada[4]!=NULL){
		convertirTimestamp(requestSeparada[4],&nuevoTimestamp);
	}else{
		nuevoTimestamp= obtenerHoraActual();
	}


	if(resultadoCache == EXIT_SUCCESS){ // es decir que EXISTE SEGMENTO y EXISTE KEY
		log_info(logger_MEMORIA, "LO ENCONTRE EN CACHEE!");
		actualizarElementoEnTablaDePagina(elementoEncontrado,nuevoValor);
		unlockSemSegmento(pathSegmento);
		paqueteAEnviar= armarPaqueteDeRtaAEnviar(request);
		enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, paqueteAEnviar,caller, indiceKernel);


	}else{
		t_marco* pagLibre =NULL;
		int index =obtenerPaginaDisponible(&pagLibre);
		if(index == LRU){
			log_info(logger_MEMORIA,"Debe ejecutars eel algoritmo de reemplazo");
			t_elemTablaDePaginas* elementoAInsertar=correrAlgoritmoLRU();
			if (elementoAInsertar!=NULL){
				if (resultadoCache == KEYINEXISTENTE) {
					t_segmento* segmentoBuscado;

					segmentoBuscado = encontrarSegmento(nuevaTabla);

					usleep(retardoMem*1000);
 					modificarElem(&elementoAInsertar,nuevoTimestamp,nuevaKey,nuevoValor,MODIFICADO);

					lockSemSegmento(nuevaTabla);
					list_add(segmentoBuscado->tablaDePagina,crearElementoEnTablaDePagina(elementoAInsertar->numeroDePag,elementoAInsertar->marco, nuevaKey,nuevoValor, nuevoTimestamp,MODIFICADO));
					unlockSemSegmento(nuevaTabla);

					paqueteAEnviar= armarPaqueteDeRtaAEnviar(request);
					enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, paqueteAEnviar,caller,indiceKernel);


				} else if (resultadoCache == SEGMENTOINEXISTENTE) {
					//-------------------------------------------
					t_semSegmento* semSegmento = (t_semSegmento*) malloc(sizeof(t_segmento));
					semSegmento->path = strdup(nuevaTabla);
					pthread_mutex_t* mutex= (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
					pthread_mutex_init(mutex, NULL);
					semSegmento->sem = mutex;

					pthread_mutex_lock(&semMListSemSegmentos);
					list_add(listaSemSegmentos, semSegmento);
					pthread_mutex_unlock(&semMListSemSegmentos);

					//-------------------------------------------
					t_segmento* nuevoSegmento = (t_segmento*)malloc(sizeof(t_segmento));
					crearSegmento(nuevoSegmento, nuevaTabla);

					lockSemSegmento(nuevaTabla);
					list_add(nuevoSegmento->tablaDePagina,crearElementoEnTablaDePagina(elementoAInsertar->numeroDePag,elementoAInsertar->marco, nuevaKey,nuevoValor, nuevoTimestamp,MODIFICADO));
					unlockSemSegmento(nuevaTabla);

					pthread_mutex_lock(&semMTablaSegmentos);
					list_add(tablaDeSegmentos->segmentos, nuevoSegmento);
					pthread_mutex_unlock(&semMTablaSegmentos);

					usleep(retardoMem*1000);

					paqueteAEnviar= armarPaqueteDeRtaAEnviar(request);
					enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, paqueteAEnviar,caller,indiceKernel);

					free(nuevaTabla);
					nuevaTabla=NULL;
				}
			} else {
				paqueteAEnviar= armarPaqueteDeRtaAEnviar(request);
				enviarAlDestinatarioCorrecto(palabraReservada, MEMORIA_FULL,request, paqueteAEnviar,caller,indiceKernel);

			}
		}else{
			if(resultadoCache == KEYINEXISTENTE){
				usleep(retardoMem*1000);

				t_segmento* segmentoDestino = encontrarSegmento(nuevaTabla);

				lockSemSegmento(nuevaTabla);
				list_add(segmentoDestino->tablaDePagina,crearElementoEnTablaDePagina(index,pagLibre,nuevaKey,nuevoValor, nuevoTimestamp,MODIFICADO));
				unlockSemSegmento(nuevaTabla);

				paqueteAEnviar= armarPaqueteDeRtaAEnviar(request);
				enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, paqueteAEnviar,caller,indiceKernel);

			}else if(resultadoCache == SEGMENTOINEXISTENTE){

				usleep(retardoMem*1000);

				t_segmento* nuevoSegmento = (t_segmento*)malloc(sizeof(t_segmento));
				crearSegmento(nuevoSegmento,nuevaTabla);

				lockSemSegmento(nuevaTabla);
				list_add(nuevoSegmento->tablaDePagina,crearElementoEnTablaDePagina(index,pagLibre,nuevaKey,nuevoValor,nuevoTimestamp,MODIFICADO));
				unlockSemSegmento(nuevaTabla);

				pthread_mutex_lock(&semMTablaSegmentos);
				list_add(tablaDeSegmentos->segmentos,nuevoSegmento);
				pthread_mutex_unlock(&semMTablaSegmentos);

				paqueteAEnviar= armarPaqueteDeRtaAEnviar(request);
				enviarAlDestinatarioCorrecto(palabraReservada, SUCCESS,request, paqueteAEnviar,caller,indiceKernel);

			}
		}
	}
	free(nuevaKeyChar);
	nuevaKeyChar=NULL;
	free(nuevaTabla);
	nuevaTabla=NULL;
	free(nuevoValor);
	nuevoValor=NULL;
	liberarArrayDeChar(requestSeparada);
	requestSeparada=NULL;
}

/* armarPaqueteDeRtaAEnviar()
 * Parametros:
 *	-> char* :: request
 * Descripcion: Cuando el resultado del insert fue EXITOSO,
 * 				arma un paquete para enviarse (a quien solicito un insert)
 * Return:
 * 	-> paqueteAEnviar:: t_paquete*
 * 	VALGRIND :: EN PROCESO */
t_paquete* armarPaqueteDeRtaAEnviar(char* request){
	t_paquete* paqueteAEnviar= malloc(sizeof(t_paquete));

	paqueteAEnviar->palabraReservada= SUCCESS;
	char** requestRespuesta= string_n_split(request,2," ");
	paqueteAEnviar->request=strdup(requestRespuesta[1]);
	paqueteAEnviar->tamanio=strlen(requestRespuesta[1]);

	liberarArrayDeChar(requestRespuesta);
	requestRespuesta=NULL;
	return paqueteAEnviar;
}

/* actualizarTimestamp()
 * Parametros:
 *	-> t_marco* :: marco
 * Descripcion: actualiza el timestamp correspondiente al marco dado
 * Return:
 * 	-> void ::
 * 	VALGRIND :: SI*/
void actualizarTimestamp(t_marco* marco){
	marco->timestamp= obtenerHoraActual();
}


/* actualizarPagina()
 * Parametros:
 *	-> t_marco* :: pagina
 *	-> char* :: nuevoValor
 * Descripcion: actualiza la pagina, insertando la hora actual, y actualiza el puntero
 * 				al marco, insertando el nuevo valor.
 * Return:
 * 	-> void ::
 * 	VALGRIND :: SI*/
void actualizarPagina (t_marco* marco, char* nuevoValue){
	marco->timestamp =  obtenerHoraActual();
	//strcpy(marco->value,'/0');
	strcpy(marco->value,nuevoValue);
}

/* actualizarElementoEnTablaDePagina()
 * Parametros:
 *	-> t_elemTablaDePaginas* :: elemento
 *	-> char* :: nuevoValor
 * Descripcion: actualiza un elemento de la tabla de paginas(de un segmento), indicando
 * 				que este ha sido actualizado(falg activado).
 * Return:
 * 	-> void ::
 * 	VALGRIND :: SI*/
void actualizarElementoEnTablaDePagina(t_elemTablaDePaginas* elemento, char* nuevoValor){
	pthread_mutex_lock(&semMMem);
	int retardoMem=retardoMemPrincipal;
	pthread_mutex_unlock(&semMMem);
	usleep(retardoMem*1000);
	actualizarPagina(elemento->marco,nuevoValor);
	elemento->modificado = MODIFICADO;
}

/* crearPagina()
 * Parametros:
 *	-> t_marco* :: pagina
 *	-> uint16_t :: nuevaKey
 *	-> char* :: nuevoValor
 *	-> unsigned long long :: timesTamp
 * Descripcion: crea una nueva pagina, utilizando como marco, un puntero que fue previamente
 * 				hallado.
 * Return:
 * 	-> t_marco* :: pagina
 * 	VALGRIND :: SI*/
t_marco* crearMarcoDePagina(t_marco* pagina,uint16_t nuevaKey, char* nuevoValue, unsigned long long timesTamp){
	pagina->key= nuevaKey;
	memcpy(pagina->value,nuevoValue,strlen(nuevoValue) + 1);
	pagina->timestamp= timesTamp;
	return pagina;
}

/* crearElementoEnTablaDePagina()
 * Parametros:
 * 	-> int :: id
 *	-> t_marco* :: pagina
 *	-> uint16_t :: nuevaKey
 *	-> char* :: nuevoValor
 *	-> unsigned long long :: timesTamp
 *	-> t_flagModificado :: flag
 * Descripcion: crea una nuevo elemento (de la tabla de paginas).Le asigna al mismo
 * 				un nuevo id(por lo cual, primero incrementa la variable global), le setea el
 * 				flag modificado en 0 y, crea una pagina.
 * Return:
 * 	-> t_elemTablaDePaginas* :: elementoDeTablaDePagina
 * 	VALGRIND :: SI*/
t_elemTablaDePaginas* crearElementoEnTablaDePagina(int id,t_marco* pagLibre, uint16_t nuevaKey, char* nuevoValue, unsigned long long timesTamp,t_flagModificado flag){
	t_elemTablaDePaginas* nuevoElemento= (t_elemTablaDePaginas*)malloc(sizeof(t_elemTablaDePaginas));
	pthread_mutex_lock(&semMBitarray);
	bitarray_set_bit(bitarray, id);
	pthread_mutex_unlock(&semMBitarray);
	nuevoElemento->numeroDePag = id;
	nuevoElemento->marco = crearMarcoDePagina(pagLibre,nuevaKey,nuevoValue,timesTamp);
	nuevoElemento->modificado = flag;

	return nuevoElemento;
}

/* crearSegmento()
 * Parametros:
 *	-> char* :: pathNuevoSegmento
 * Descripcion: crea un nuevo segmento con el path que fue solicitado.
 * Return:
 * 	-> t_segmento* :: nuevoSegmento
 * 	VALGRIND :: SI*/
void crearSegmento(t_segmento* nuevoSegmento,char* pathNuevoSegmento){
	nuevoSegmento->path=strdup(pathNuevoSegmento);
	nuevoSegmento->tablaDePagina=list_create();

	t_semSegmento* semSegmento = (t_semSegmento*) malloc(sizeof(t_segmento));
	semSegmento->path = strdup(pathNuevoSegmento);
	pthread_mutex_t* mutex=(pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, NULL);
	semSegmento->sem = mutex;

	pthread_mutex_lock(&semMListSemSegmentos);
	list_add(listaSemSegmentos, semSegmento);
	pthread_mutex_unlock(&semMListSemSegmentos);

}

/* procesarCreate()
 * Parametros:
 *	-> codRequest :: codRequest
 *	-> char* :: request
 *	-> consistencia :: consistencia
 *	-> t_caller :: caller
 *	-> int :: socketKernel
 * Descripcion: SC || SHC -> envia el create  LFS.
 * 				EC -> crea el segmento en la memoria, sin consultarle a LFS.
 * Return:
 * 	-> void ::
 * 	VALGRIND :: NO*/
void procesarCreate(cod_request codRequest, char* request ,consistencia consistencia, t_caller caller, int indiceKernel){
	t_paquete* valorDeLFS = (t_paquete*) malloc(sizeof(t_paquete));
	int resultadoLFS = intercambiarConFileSystem(codRequest,request, &valorDeLFS, caller, indiceKernel);

	if(resultadoLFS == -1 || valorDeLFS->palabraReservada == COMPONENTE_CAIDO){
		valorDeLFS = malloc(sizeof(t_paquete));
		valorDeLFS->request=strdup("FALLO CONEXION LFS");
		valorDeLFS->tamanio=(sizeof(valorDeLFS->request));
		enviarAlDestinatarioCorrecto(COMPONENTE_CAIDO, COMPONENTE_CAIDO,request, valorDeLFS,caller,indiceKernel);
	}else{
		if(consistencia == EC || caller == CONSOLE){
			create(codRequest, request);
		}
		enviarAlDestinatarioCorrecto(codRequest,SUCCESS,request, valorDeLFS, caller,indiceKernel);
	}
}

/* create()
 * Parametros:
 *	-> codRequest :: codRequest
 *	-> char* :: request
 * Descripcion: Verifica que el segmento si existe en la memoria,y , en caso
 * 				de que no sea asi, se crea un nuevo segmento.
 * 				En caso contrario, no lo crea.
 * Return:
 * 	-> void ::
 * 	VALGRIND :: EN PROCESO */
void create(cod_request codRequest,char* request){
	t_erroresMemoria rtaCache = existeSegmentoEnMemoria(codRequest,request);
	pthread_mutex_lock(&semMMem);
	int retardoMem=retardoMemPrincipal;
	pthread_mutex_unlock(&semMMem);


	if(rtaCache == SEGMENTOINEXISTENTE){

		usleep(retardoMem*1000);

		char** requestSeparada = separarRequest(request);
		t_segmento* nuevoSegmento = (t_segmento*)malloc(sizeof(t_segmento));

		crearSegmento(nuevoSegmento,requestSeparada[1]);
		pthread_mutex_lock(&semMTablaSegmentos);
		list_add(tablaDeSegmentos->segmentos,nuevoSegmento);
		pthread_mutex_unlock(&semMTablaSegmentos);

		liberarArrayDeChar(requestSeparada);
		requestSeparada=NULL;
	}
}

/* existeSegmentoEnMemoria()
 * Parametros:
 *	-> codRequest :: codRequest
 *	-> char* :: request
 * Descripcion: Solamente verifica si existe o no un segmento en memoria.
 * Return:
 * 	-> void ::
 * 	VALGRIND :: EN PROCESO */
t_erroresMemoria existeSegmentoEnMemoria(cod_request palabraReservada, char* request){
	char** parametros = separarRequest(request);
	char* segmentoABuscar=strdup(parametros[1]);
	int encontrarTabla(t_segmento* segmento){
		return string_equals_ignore_case(segmento->path, segmentoABuscar);
	}
	pthread_mutex_lock(&semMTablaSegmentos);
	t_segmento* segmentosEnCache = list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);
	pthread_mutex_unlock(&semMTablaSegmentos);

	if(segmentosEnCache!= NULL){
		liberarArrayDeChar(parametros);
		parametros=NULL;
		free(segmentoABuscar);
		segmentoABuscar =NULL;
		log_info(logger_MEMORIA,"YA EXISTE EL SEGMENTO");
		return SEGMENTOEXISTENTE;
	}else{
		liberarArrayDeChar(parametros);
		parametros=NULL;
		free(segmentoABuscar);
		segmentoABuscar =NULL;
		log_info(logger_MEMORIA,"NO EXISTE EL SEGMENTO");
		return SEGMENTOINEXISTENTE;
	}
}


/* obtenerPaginaDisponible()
 * Parametros:
 *	-> t_marco** :: pagLibre
 * Descripcion: Obtiene un puntero a una pagina libre, ocurriendo esto cuando existe un marco libre
 *				donde pueda guardarse la misma.
 * Return:
 * 	-> int :: resultado de la obtencion de una paina libre
 * 	VALGRIND :: SI*/
int obtenerPaginaDisponible(t_marco** pagLibre){
	int index= obtenerIndiceMarcoDisponible();
	if(index == LRU){
		return LRU;
	}else{
		*pagLibre = memoria + (sizeof(unsigned long long)+sizeof(uint16_t)+maxValue)*index;
		return index;
	}

}



void removerSem(char* pathARemover){
	int segmentoEsIgual(t_semSegmento* semSegmento){
		if(string_equals_ignore_case(semSegmento->path, pathARemover)){
				return TRUE;
			}else{
				return FALSE;
			}
	}
	pthread_mutex_lock(&semMListSemSegmentos);
	list_remove_and_destroy_by_condition(listaSemSegmentos,(void*)segmentoEsIgual,(void*)liberarSemSegmento);
	pthread_mutex_unlock(&semMListSemSegmentos);
}

void liberarSemSegmento(t_semSegmento* semSegmento){
	pthread_mutex_destroy(semSegmento->sem);
	free(semSegmento->path);
	semSegmento->path=NULL;
	free(semSegmento->sem);
	semSegmento->sem=NULL;
	free(semSegmento);
	semSegmento=NULL;
}


/* liberarEstructurasMemoria()
 * Parametros:
 *	-> t_tablaDeSEgmentos :: tablaDeSegmento
 * Descripcion: Libera el marco de cada pagina, libera cada pagina de una tablaDePagina, libera la tablaDePagina
 * 				de un segmento, libera lista de segmentos de una tablaDeSegmento, libera lista de tablaDeSegmentos
 * Return:
 * 	-> :: void
 * 	VALGRIND :: NO */
void liberarEstructurasMemoria(){
	pthread_mutex_lock(&semMTablaSegmentos);
	list_destroy_and_destroy_elements(tablaDeSegmentos->segmentos, (void*) eliminarUnSegmento);
	free(tablaDeSegmentos);
	pthread_mutex_unlock(&semMTablaSegmentos);
	list_destroy_and_destroy_elements(memoriasLevantadas, (void*)liberarConfigMemoria);
	list_destroy_and_destroy_elements(memoriasSeeds, (void*)liberarConfigMemoria);
	list_destroy_and_destroy_elements(listaSemSegmentos,(void*)liberarSemSegmento);

}
/* eliminarSegmento()
 * Parametros:
 *	-> t_segmento* :: segmento
 * Descripcion: Libera a el marco de la pagina, libera cada pagina de una tabla
 * Return:
 * 	-> :: void
 * 	VALGRIND :: NO */
void eliminarUnSegmento(t_segmento* segmento){

	removerSem(segmento->path);
	free(segmento->path);
	segmento->path=NULL;
	list_destroy_and_destroy_elements(segmento->tablaDePagina, (void*) eliminarElemTablaPagina);
	free(segmento);
}


void eliminarElemTablaPagina(t_elemTablaDePaginas* pagina){
	eliminarMarco(pagina,pagina->marco);
	free(pagina);
	pagina=NULL;
}

/* liberarMemoria()
 * Parametros:
 *	-> :: void
 * Descripcion: Libera los punteros reservados en inicializarMemoria()
 * Return:
 * 	-> :: void
 * 	VALGRIND :: NO */
void liberarMemoria(){
	pthread_mutex_destroy(&semMBitarray);
	pthread_mutex_destroy(&semMTablaSegmentos);
	pthread_mutex_destroy(&semMMemoriasLevantadas);
	pthread_mutex_destroy(&semMConfig);
	pthread_mutex_destroy(&semMGossiping);
	pthread_mutex_destroy(&semMSleepJournal);
	pthread_mutex_destroy(&semMFS);
	pthread_mutex_destroy(&semMMem);
	pthread_mutex_destroy(&semMJOURNAL);
	pthread_mutex_destroy(&semMCONSOLA);
	pthread_mutex_destroy(&semMKERNEL);
	pthread_mutex_destroy(&semMConexionLFS);

	inotify_rm_watch(file_descriptor, watch_descriptor);	//iNotify
	close(file_descriptor);

	log_info(logger_MEMORIA, "Finaliza MEMORIA");
	bitarray_destroy(bitarray);
	free(bitarrayString);
	bitarrayString=NULL;
	free(memoria);
	liberar_conexion(conexionLfs);
	log_destroy(logger_MEMORIA);
	config_destroy(config);
	free(ipMia);
	free(puertoMio);
	free(numerosMio);
}


/* eliminarMarco()
 * Parametros:
 *	-> t_marco** :: pagLibre
 * Descripcion: Obtiene un puntero a una pagina libre, ocurriendo esto cuando existe un marco libre
 *				donde pueda guardarse la misma.
 * Return:
 * 	-> void ::
 * 	VALGRIND :: SI*/
void eliminarMarco(t_elemTablaDePaginas* elem,t_marco* marcoAEliminar){
	pthread_mutex_lock(&semMBitarray);
	bitarray_clean_bit(bitarray, elem->numeroDePag);
	pthread_mutex_unlock(&semMBitarray);
	memset(marcoAEliminar->value, 0, maxValue);
}

/* procesarDescribe()
 * Parametros:
 *	-> cod_request :: codRequest
 *	-> char* :: request
 *	-> t_caller :: caller
 *	-> int :: i//socket kernel
 * Descripcion: Actua como un pasamanos para pasarle a info del describe, mandada por lfs
 * 				a kernel.
 * Return:
 * 	-> void ::
 * 	VALGRIND :: NO*/
void procesarDescribe(cod_request codRequest, char* request,t_caller caller,int indiceKernel){
	t_paquete* describeLFS;
	int resultadoLFS = intercambiarConFileSystem(codRequest,request, &describeLFS, caller, indiceKernel);
	if (resultadoLFS == FAILURE || describeLFS->palabraReservada == COMPONENTE_CAIDO) {
		describeLFS = malloc(sizeof(t_paquete));
		codRequest = FAILURE;
		describeLFS->request = strdup("FALLO CONEXION CON LFS");
	}
	enviarAlDestinatarioCorrecto(codRequest,describeLFS->palabraReservada,request,describeLFS,caller,indiceKernel);
}


/* procesarDrop()
 * Parametros:
 *	-> cod_request :: codRequest
 *	-> char* :: request
 *	-> consistencia :: consistencias
 *	-> t_caller :: caller
 *	-> int :: i (socket kernel)
 * Descripcion: Busca la tabla en memoria principal, si la encuentra libera la memoria. Siempre le avisa a LFS
 * Return:
 * 	-> void ::
 * 	VALGRIND :: NO*/
void procesarDrop(cod_request codRequest, char* request ,consistencia consistencia, t_caller caller, int indiceKernel) {
	t_paquete* valorDeLFS;
	char** requestSeparada = separarRequest(request);
	char* segmentoABuscar=strdup(requestSeparada[1]);
	pthread_mutex_lock(&semMMem);
	int retardoMem=retardoMemPrincipal;
	pthread_mutex_unlock(&semMMem);
	int resultadoLFS = intercambiarConFileSystem(codRequest,request, &valorDeLFS, caller, indiceKernel);
	if (resultadoLFS == FAILURE || valorDeLFS->palabraReservada == COMPONENTE_CAIDO) {
		valorDeLFS = malloc(sizeof(t_paquete));
		codRequest = FAILURE;
		valorDeLFS->palabraReservada = FAILURE;
		valorDeLFS->request = strdup("FALLO CONEXION CON LFS");
	} else {
		if(consistencia == EC || caller == CONSOLE){
			int encontrarTabla(t_segmento* segmento){
				return string_equals_ignore_case(segmento->path, segmentoABuscar);
			}
			pthread_mutex_lock(&semMTablaSegmentos);
			t_segmento* segmentosEnCache= list_find(tablaDeSegmentos->segmentos,(void*)encontrarTabla);
			pthread_mutex_unlock(&semMTablaSegmentos);

			usleep(retardoMem*1000);

			if(segmentosEnCache!= NULL){

				pthread_mutex_lock(&semMTablaSegmentos);
				list_remove_and_destroy_by_condition(tablaDeSegmentos->segmentos,(void*)encontrarTabla,(void*)eliminarUnSegmento);
				pthread_mutex_unlock(&semMTablaSegmentos);

			}else{
				log_info(logger_MEMORIA,"La %s ya no existia en MEMORIA",segmentoABuscar);
			}
		}
	}

	enviarAlDestinatarioCorrecto(codRequest,valorDeLFS->palabraReservada,request, valorDeLFS, caller,indiceKernel);

	liberarArrayDeChar(requestSeparada);
	requestSeparada=NULL;
	free(segmentoABuscar);
	segmentoABuscar=NULL;
}

/* desvincularVictimaDeSuSegmento()
 *	-> t_elemTablaDePaginas* :: elemVictima
 * Descripcion: Le quita al segmento la pagina indicada
 * Return:
 * 	-> void ::
 * 	VALGRIND :: NO*/
int desvincularVictimaDeSuSegmento(t_elemTablaDePaginas* elemVictima){
	int desvinculacion=NUESTRO_ERROR;
	int i=0,j=0;

	t_segmento* seg;
	pthread_mutex_lock(&semMTablaSegmentos);

	while(list_get(tablaDeSegmentos->segmentos,i)!=NULL){
		seg=list_get(tablaDeSegmentos->segmentos,i);
		pthread_mutex_unlock(&semMTablaSegmentos);

		lockSemSegmento(seg->path);
		while(list_get(seg->tablaDePagina,j)!=NULL){
			t_elemTablaDePaginas* elem =list_get(seg->tablaDePagina,j);
			if(elem->numeroDePag == elemVictima->numeroDePag){

				int indice= encontrarIndice(elemVictima,seg);
				list_remove(seg->tablaDePagina,indice);
				desvinculacion=SUCCESS;
			}
			j++;
		}
		unlockSemSegmento(seg->path);
		pthread_mutex_lock(&semMTablaSegmentos);
		i++;
		j=0;
	}
	pthread_mutex_unlock(&semMTablaSegmentos);

	return desvinculacion;
}


int encontrarIndice(t_elemTablaDePaginas* elemVictima,t_segmento* segmento){
	int indice=0;
	while(list_get(segmento->tablaDePagina,indice)!=NULL){
		t_elemTablaDePaginas* elem=list_get(segmento->tablaDePagina,indice);
		if(elem->numeroDePag == elemVictima->numeroDePag){
			//indice = elem->numeroDePag;
			return indice;
		}
		indice++;
	}
	return indice;
}
/* menorTimestamp()
 *	-> t_elemTablaDePaginas* :: primerElem
 *	-> t_elemTablaDePaginas* :: segundoElem
 * Descripcion: Indica si es cierto que el primer elemento contiene un timestamp menor que el segundo.
 * Return:
 * 	-> int :: bool
 * 	VALGRIND :: SI*/
 int menorTimestamp(t_elemTablaDePaginas* primerElem,t_elemTablaDePaginas* segundoElem) {
	return primerElem->marco->timestamp <= segundoElem->marco->timestamp;
}

/* correrAlgoritmoLRU()
 *	-> int :: rta
 * Descripcion: Revisa si existe un pag sin modificar y , de hacerlo, la elige como victima para darsela
 * 				a la nueva request ingresada.
 * Return:
 * 	-> int :: bool-rta del algoritmo LRU
 * 	VALGRIND :: NO*/
 t_elemTablaDePaginas* correrAlgoritmoLRU() {
	int i=0, j= 0;
	pthread_mutex_lock(&semMMem);
	int retardoMem=retardoMemPrincipal;
	pthread_mutex_unlock(&semMMem);

	t_list* elemSinModificar=list_create();
	t_segmento* segmento;

	pthread_mutex_lock(&semMTablaSegmentos);
	while(list_get(tablaDeSegmentos->segmentos,j)!=NULL){
		segmento=list_get(tablaDeSegmentos->segmentos,j);
		pthread_mutex_unlock(&semMTablaSegmentos);
		lockSemSegmento(segmento->path);
		while (list_get(segmento->tablaDePagina, i) != NULL) {
			t_elemTablaDePaginas* elemenetoTablaDePag =list_get(segmento->tablaDePagina, i);
			if (elemenetoTablaDePag->modificado == SINMODIFICAR) {

				usleep(retardoMem*1000);
				list_add(elemSinModificar, elemenetoTablaDePag);
			}
			i++;
		}
		unlockSemSegmento(segmento->path);
		j++;
		i=0;
		pthread_mutex_lock(&semMTablaSegmentos);

	}
	pthread_mutex_unlock(&semMTablaSegmentos);
	t_elemTablaDePaginas* elemVictimaLRU;

	if (!list_is_empty(elemSinModificar)) {
		list_sort(elemSinModificar, (void*) menorTimestamp);
		elemVictimaLRU= list_get(elemSinModificar,0);
		int desvinculacion=desvincularVictimaDeSuSegmento(elemVictimaLRU);
		pthread_mutex_lock(&semMBitarray);
		bitarray_clean_bit(bitarray,elemVictimaLRU->numeroDePag);
		pthread_mutex_unlock(&semMBitarray);

	} else {
		elemVictimaLRU=NULL;
	}
	list_destroy(elemSinModificar);
	return elemVictimaLRU;
}



 /* procesarJournal()
  * Parametros:
  *		-> cod_request :: codRequest
  *		-> char* :: request
  *		-> consistencia :: consistencias
  *		-> t_caller :: caller
  *		-> int :: i (socket kernel)
  * Descripcion: Inserta todos los datos de Memoria en FS
  * Return:
  * 	-> void */

 //	OK	El Journal automático realizado cada X unidades de tiempo (configurado por archivo de configuración).
 //	OK	El Journal manual por medio de la API que dispone la Memoria (JOURNAL).
 //		El Journal forzoso debido a que la Memoria entró en un estado FULL y requiere nuevas páginas para asignar (realizado por pedido del Kernel)
 //	OK 	El Journal manual por medio de la API que dispone el Kernel.
 //		Bloquear: Salvo CREATE/DESCRIBE

void procesarJournal(cod_request palabraReservada, char* request, t_caller caller, int indiceKernel,t_tipoJournal tipoJournal) {


	pthread_mutex_lock(&semMKERNEL);
	pthread_mutex_lock(&semMCONSOLA);
	pthread_mutex_lock(&semMJOURNAL);


	if(tipoJournal == AUTOMATICO){
		log_info(logger_MEMORIA,"-----------COMENZO EL JOURNAL AUTOMATICO-----------------");
	}else{
		log_info(logger_MEMORIA,"-----------COMENZO EL JOURNAL-----------------");
	}
	sleep(10);

	t_list* listResultadosJournal= list_create();
	t_int* resultadoAux = malloc(sizeof(t_int*));
	int i=0,j=0;
	char* requestAEnviar;
	requestAEnviar= NULL;
	pthread_mutex_lock(&semMMem);
	int retardoMem = retardoMemPrincipal;
	pthread_mutex_unlock(&semMMem);

	pthread_mutex_lock(&semMTablaSegmentos);
	while(list_get(tablaDeSegmentos->segmentos,i)!=NULL){
		t_segmento* segmento=list_get(tablaDeSegmentos->segmentos,i);
		pthread_mutex_unlock(&semMTablaSegmentos);
		lockSemSegmento(segmento->path);
		while(list_get(segmento->tablaDePagina,j)!=NULL){
			t_elemTablaDePaginas* elemPagina = list_get(segmento->tablaDePagina,j);
			if(elemPagina->modificado == MODIFICADO){

				usleep(retardoMem*1000);
				requestAEnviar= strdup("");
				string_append_with_format(&requestAEnviar,"%s%s%s%s%d%s%c%s%c%s%llu","INSERT"," ",segmento->path," ",elemPagina->marco->key," ",'"',elemPagina->marco->value,'"'," ",elemPagina->marco->timestamp);

				t_paquete* insertJournalLFS;
				int resultadoLFS = intercambiarConFileSystem(INSERT,requestAEnviar, &insertJournalLFS, caller, indiceKernel);
				if (resultadoLFS == FAILURE || insertJournalLFS->palabraReservada == COMPONENTE_CAIDO) {
					insertJournalLFS = malloc(sizeof(t_paquete));
					insertJournalLFS->palabraReservada = FAILURE;
					insertJournalLFS->request = strdup("FALLO CONEXION CON LFS");
				}
				log_info(logger_MEMORIA,"Le enviamos a LFS: %s", requestAEnviar);

				free(requestAEnviar);
				requestAEnviar=NULL;

				if(insertJournalLFS->palabraReservada==SUCCESS ){
					resultadoAux->valor=SUCCESS;
				}else{
					resultadoAux->valor=NUESTRO_ERROR;
				}
				list_add(listResultadosJournal,resultadoAux);

				eliminar_paquete(insertJournalLFS);
			}
			j++;
		}
		unlockSemSegmento(segmento->path);
		i++;
		j=0;
		pthread_mutex_lock(&semMTablaSegmentos);

	}
	pthread_mutex_unlock(&semMTablaSegmentos);

	pthread_mutex_lock(&semMTablaSegmentos);
	list_clean_and_destroy_elements(tablaDeSegmentos->segmentos,(void*)eliminarUnSegmento);
	pthread_mutex_unlock(&semMTablaSegmentos);

	int esJournalSUCCESS(t_int* valor){
		if(valor->valor == SUCCESS){
			return TRUE;
		}else{
			return FALSE;
		}
	}
	t_paquete* resultadoJournal= (t_paquete*)malloc(sizeof(t_paquete));
	if(tipoJournal == MY_REQUEST){
		int resultadoControl = list_all_satisfy(listResultadosJournal, (void*) esJournalSUCCESS);

		if(resultadoControl == TRUE || list_is_empty(listResultadosJournal)){
			resultadoJournal->palabraReservada=JOURNAL;
			resultadoJournal->request=strdup("  Se ha realizado el JOURNAL con exito");
			resultadoJournal->tamanio=sizeof(resultadoJournal->request);
			enviarAlDestinatarioCorrecto(palabraReservada,SUCCESS,request,resultadoJournal,caller, indiceKernel);

		}else{
			resultadoJournal->palabraReservada=JOURNAL;
			resultadoJournal->request=strdup(" NO se ha realizado el JOURNAL con exito");
			resultadoJournal->tamanio=sizeof(resultadoJournal->request);
			enviarAlDestinatarioCorrecto(palabraReservada,FAILURE,request,resultadoJournal,caller, indiceKernel);
		}
	}else{
		resultadoJournal->palabraReservada=JOURNAL;
		resultadoJournal->request=strdup("  NO habia nada que impactar en LFS.JOURNAL se realizo con exito");
		resultadoJournal->tamanio=sizeof(resultadoJournal->request);
		enviarAlDestinatarioCorrecto(palabraReservada,SUCCESS,request,resultadoJournal,caller, indiceKernel);

	}
	list_destroy(listResultadosJournal);
	free(resultadoAux);
	resultadoAux=NULL;
	free(requestAEnviar);
	requestAEnviar=NULL;

	if(tipoJournal == AUTOMATICO){
		log_info(logger_MEMORIA,"-----------FIN EL JOURNAL AUTOMATICO-----------------");
	}else{
		log_info(logger_MEMORIA,"-----------FIN EL JOURNAL-----------------");
	}
	pthread_mutex_unlock(&semMKERNEL);
	pthread_mutex_unlock(&semMCONSOLA);
	pthread_mutex_unlock(&semMJOURNAL);


}

/* hacerJournal()
 * Parametros:
 * 	-> void
 * Descripcion: Realiza un journal cada x cantidad de tiempo.
 * Return:
 * 	-> :: void  */

void hacerJournal(void){
	int tiempoJournal;
	while(1){
		pthread_mutex_lock(&semMSleepJournal);
		tiempoJournal = retardoJournal;
		pthread_mutex_unlock(&semMSleepJournal);
		usleep(tiempoJournal*1000);
		procesarJournal(JOURNAL, "JOURNAL",CONSOLE,-1,AUTOMATICO);

	}
}


