#include "Lissandra.h"

/*
 * Parametros del main:
 * 1. Char*: Path del config
 * */
int main(int argc, char* argv[]) {

	pathConfig = argv[1];
	inicializacionLissandraFileSystem();

	if(!pthread_create(&hiloDeInotify, NULL, (void*)escucharCambiosEnConfig, NULL)){
		log_info(logger_LFS, "Hilo de inotify creado");
	}else{
		log_error(logger_LFS, "Error al crear hilo de inotify");
	}

	if(!pthread_create(&hiloRecibirMemorias, NULL, (void*)recibirMemorias, NULL)){
		log_info(logger_LFS, "Hilo de recibir memorias creado");
	}else{
		log_error(logger_LFS, "Error al crear hilo recibir memorias");
	}

	if(!pthread_create(&hiloDumpeo, NULL, (void*)hiloDump, NULL)){
		log_info(logger_LFS, "Hilo de Dump iniciado");
	}else{
		log_error(logger_LFS, "Error al crear hilo Dumpeo");
	}

	if(!pthread_create(&hiloLeerDeConsola, NULL, (void*)leerDeConsola, NULL)){
		log_info(logger_LFS, "Hilo de consola creado");
	}else{
		log_error(logger_LFS, "Error al crear hilo de consola");
	}

	pthread_join(hiloLeerDeConsola, NULL);
	log_info(logger_LFS, "Hilo de consola finalizado");
	pthread_join(hiloRecibirMemorias, NULL);
	log_info(logger_LFS, "Hilo recibir memorias finalizado");
	pthread_join(hiloDumpeo, NULL);
	log_info(logger_LFS, "Hilo dumpeo finalizado");
	pthread_join(hiloDeInotify, NULL);
	log_info(logger_LFS, "Hilo inotify finalizado");
	liberarMemoriaLFS();
	return EXIT_SUCCESS;
}


void inicializacionLissandraFileSystem(){
	logger_LFS = log_create("LissandraFileSystem.log", "Lfs", 1, LOG_LEVEL_DEBUG);
	log_info(logger_LFS, "----------------Inicializacion de Lissandra File System--------------");

	if(pathConfig != NULL){
		config = config_create(pathConfig);
	}else{

		config = config_create("/home/utnso/tp-2019-1c-bugbusters/lfs/LissandraFileSystem.config");
	}

	if(config == NULL){
		log_error(logger_LFS, "Error al leer archivo de configuracion");
		log_info(logger_LFS, "Finalizando Lissandra File System");
		exit(EXIT_FAILURE);
	}
	if(!config_has_property(config, "PUNTO_MONTAJE")){
		log_error(logger_LFS, "Key \"PUNTO_MONTAJE\" no encontrada en config");
		log_info(logger_LFS, "Finalizando Lissandra File System");
		exit(EXIT_FAILURE);
	}
	char* puntoDeMontaje = config_get_string_value(config, "PUNTO_MONTAJE");
	retardo = config_get_int_value(config, "RETARDO");
	tiempoDump = config_get_int_value(config, "TIEMPO_DUMP");
	tamanioValue = config_get_int_value(config, "TAMAÑO_VALUE");

	pathRaiz = strdup(puntoDeMontaje);
	pathTablas = string_from_format("%sTablas", pathRaiz);
	pathMetadata = string_from_format("%sMetadata", pathRaiz);
	pathBloques = string_from_format("%sBloques", pathRaiz);
	char* pathBitmap = string_from_format("%s/Bitmap.bin", pathMetadata);
	char* pathFileMetadata = string_from_format("%s/Metadata.bin", pathMetadata);

	DIR* dirPuntoDeMontaje = opendir(pathRaiz);
	if(dirPuntoDeMontaje){
		free(dirPuntoDeMontaje);
		configMetadata = config_create(pathFileMetadata);
		blocks = config_get_int_value(configMetadata, "BLOCKS");
	}else{
		log_info(logger_LFS, "File System no detectado");
		crearFS(pathBitmap, pathFileMetadata);
	}

	free(pathFileMetadata);

	levantarFS(pathBitmap);
	free(pathBitmap);

	log_info(logger_LFS, "----------------Lissandra File System inicializado correctamente--------------");
}

void crearFS(char* pathBitmap, char* pathFileMetadata) {
	if(mkdir(pathRaiz, S_IRWXU) == -1){
		perror("Error al crear directorio raiz");
	}
	if(mkdir(pathTablas, S_IRWXU) == -1){
		perror("Error al crear directorio de tablas");
	}
	if(mkdir(pathMetadata, S_IRWXU) == -1){
		perror("Error al crear metadata del FS");
	}
	if(mkdir(pathBloques, S_IRWXU) == -1){
		perror("Error al crear directorio de bloques");
	}

	crearFSMetadata(pathBitmap, pathFileMetadata);
	crearBloques();
}

void crearFSMetadata(char* pathBitmap, char* pathFileMetadata){
	FILE* fileMetadata = fopen(pathFileMetadata, "w");
	if(fileMetadata == NULL){
		log_error(logger_LFS, "Error al crear metadata");
		log_info(logger_LFS, "Finalizando Lissandra File System");
		exit(EXIT_FAILURE);
	}
	fclose(fileMetadata);

	configMetadata = config_create(pathFileMetadata);

	char* tamanioDeBloque = readline("Ingresar tamanio de bloque: ");
	char* numeroDeBloques = readline("Ingrese numero de bloques: ");
	char* magicNumber = readline("Ingrese magic number: ");

	if(!string_equals_ignore_case(magicNumber, "xd")){
		for(int i = 0; i < 15 ; i++)free(magicNumber);
	}
	config_set_value(configMetadata, "BLOCK_SIZE", tamanioDeBloque);
	config_set_value(configMetadata, "BLOCKS", numeroDeBloques);
	config_set_value(configMetadata, "MAGIC_NUMBER", magicNumber);
	config_save(configMetadata);

	free(tamanioDeBloque);

	free(magicNumber);

	int bitmapDescriptor = open(pathBitmap, O_CREAT | O_RDWR | O_TRUNC , S_IRWXU);
	if(bitmapDescriptor == -1){
		log_error(logger_LFS, "Error al crear bitmap");
		log_info(logger_LFS, "Finalizando Lissandra File System");
		exit(EXIT_FAILURE);
	}

	blocks = atoi(numeroDeBloques);

	if(ftruncate(bitmapDescriptor, atoi(numeroDeBloques)/8)){
		perror("Error al truncar archivo de bitmap");
	}

	bitmap = mmap(NULL, blocks/8, PROT_READ | PROT_WRITE, MAP_SHARED, bitmapDescriptor, 0);
	bitarray = bitarray_create_with_mode(bitmap, atoi(numeroDeBloques)/8, LSB_FIRST);
	bitarray_destroy(bitarray);
	munmap(bitmap, blocks/8);
	close(bitmapDescriptor);
	free(numeroDeBloques);
}

void crearBloques(){
	char* fileBloque;
	for (int i = 0; i < blocks; i++) {
		fileBloque = string_from_format("%s/%d.bin", pathBloques, i);
		FILE* bloqueFile = fopen(fileBloque, "w");
		if(bloqueFile){
			fclose(bloqueFile);
		}
		free(fileBloque);
	}
}

void levantarFS(char* pathBitmap){
	memtable = (t_memtable*) malloc(sizeof(t_memtable));
	memtable->tablas = list_create();
	memorias = list_create();

	bitmapDescriptor = open(pathBitmap, O_RDWR);
	bitmap = mmap(NULL, blocks/8, PROT_READ | PROT_WRITE, MAP_SHARED, bitmapDescriptor, 0);
	bitarray = bitarray_create_with_mode(bitmap, blocks/8, LSB_FIRST);


	pthread_mutex_init(&mutexMemtable, NULL);
	pthread_mutex_init(&mutexTablasParaCompactaciones, NULL);
	pthread_mutex_init(&mutexConfig, NULL);
	pthread_mutex_init(&mutexRetardo, NULL);
	pthread_mutex_init(&mutexTiempoDump, NULL);
	pthread_mutex_init(&mutexBitmap, NULL);
	pthread_mutex_init(&mutexMemorias, NULL);

	tablasParaCompactaciones = list_create();

	DIR* tablas;
	if((tablas = opendir(pathTablas)) == NULL){
		perror("Open Tables");
	}else{
		struct dirent* tabla;

		while((tabla = readdir(tablas)) != NULL){
			if(strcmp(tabla->d_name, ".") == 0 || strcmp(tabla->d_name, "..") == 0) continue;
			char* pathTabla = string_from_format("%s/%s", pathTablas, (char*) tabla->d_name);
			if(!pthread_create(&hiloDeCompactacion, NULL, (void*) hiloCompactacion, (void*) pathTabla)){
				pthread_detach(hiloDeCompactacion);

				t_bloqueo* idYMutexPropio = malloc(sizeof(t_bloqueo));
				idYMutexPropio->id = 0; // 0 seria consola propia, sino son fds de memorias
				pthread_mutex_init(&(idYMutexPropio->mutex), NULL);

				t_hiloTabla* hiloTabla = malloc(sizeof(t_hiloTabla));
				hiloTabla->thread = &hiloDeCompactacion;
				hiloTabla->nombreTabla = strdup(tabla->d_name);
				hiloTabla->finalizarCompactacion = 0;
				hiloTabla->cosasABloquear = list_create();
				list_add(hiloTabla->cosasABloquear, idYMutexPropio);

				pthread_mutex_lock(&mutexTablasParaCompactaciones);
				list_add(tablasParaCompactaciones, hiloTabla);
				pthread_mutex_unlock(&mutexTablasParaCompactaciones);

				log_info(logger_LFS, "Hilo de compactacion de la tabla %s creado", tabla->d_name);
				pthread_detach(hiloDeCompactacion);
			}else{
				log_error(logger_LFS, "Error al crear hilo de compactacion de la tabla %s", tabla->d_name);
			}
		}
		closedir(tablas);
	}
}

void liberarMemoriaLFS(){
	void liberarRecursos(t_hiloTabla* tabla){
		void liberarRequest(t_request* request){
			free(request->parametros);
			free(request);
		}
		list_destroy_and_destroy_elements(tabla->cosasABloquear, (void*)liberarMutexTabla);
		free(tabla->nombreTabla);
		free(tabla);
	}

	void eliminarMemoria(t_int* memoria_fd) {
		free(memoria_fd);
	}

	log_info(logger_LFS, "Finalizando LFS");

	pthread_mutex_lock(&mutexTablasParaCompactaciones);
	list_destroy_and_destroy_elements(tablasParaCompactaciones, (void*) liberarRecursos);
	pthread_mutex_unlock(&mutexTablasParaCompactaciones);


	pthread_mutex_lock(&mutexMemorias);
	list_destroy_and_destroy_elements(memorias, (void*)eliminarMemoria);
	pthread_mutex_unlock(&mutexMemorias);

	free(pathTablas);
	free(pathMetadata);
	free(pathBloques);
	free(pathRaiz);
	pthread_mutex_lock(&mutexMemtable);
	list_destroy_and_destroy_elements(memtable->tablas, (void*) vaciarTabla);
	free(memtable);
	pthread_mutex_unlock(&mutexMemtable);
	munmap(bitmap, blocks/8);
	close(bitmapDescriptor);
	bitarray_destroy(bitarray);

	log_destroy(logger_LFS);

	pthread_mutex_destroy(&mutexMemtable);
	pthread_mutex_destroy(&mutexTablasParaCompactaciones);
	pthread_mutex_destroy(&mutexConfig);
	pthread_mutex_destroy(&mutexRetardo);
	pthread_mutex_destroy(&mutexTiempoDump);
	pthread_mutex_destroy(&mutexBitmap);
	pthread_mutex_destroy(&mutexMemorias);

	config_destroy(configMetadata);
	config_destroy(config);
}

void* leerDeConsola(void* arg) {
	char* mensajeDeError;
	char* mensaje;
	while (1) {
		mensaje = readline(">");
		if (!(strncmp(mensaje, "", 1) != 0)) {
			pthread_cancel(hiloRecibirMemorias);
			pthread_cancel(hiloDumpeo);
			pthread_cancel(hiloDeInotify);
			free(mensaje);
			break;
		}

		if(validarMensaje(mensaje, LFS, &mensajeDeError) == SUCCESS){
			char** request = string_n_split(mensaje, 2, " ");
			cod_request palabraReservada = obtenerCodigoPalabraReservada(request[0], LFS);
			liberarArrayDeChar(request);
			char** requestSeparada = separarRequest(mensaje);
			int numero = 0;
			interpretarRequest(palabraReservada, mensaje, &numero);
			liberarArrayDeChar(requestSeparada);
		}else{
			log_error(logger_LFS, mensajeDeError);
		}
		free(mensaje);
	}
	return NULL;
}

void* recibirMemorias(void* arg) {
	char* puerto = config_get_string_value(config, "PUERTO");
	char* ip = config_get_string_value(config, "IP");
	int lissandraFS_fd = iniciar_servidor(puerto, ip);
	log_info(logger_LFS, "Lissandra lista para recibir Memorias");
	pthread_t hiloRequest;

	while (1) {
		int memoria_fd = esperar_cliente(lissandraFS_fd);
		if(memoria_fd > 0) {
			if(!pthread_create(&hiloRequest, NULL, (void*) conectarConMemoria, (void*) memoria_fd)) {

				t_int* fd = malloc(sizeof(t_int));
				fd->valor = memoria_fd;

				pthread_mutex_lock(&mutexMemorias);
				list_add(memorias, fd);
				pthread_mutex_unlock(&mutexMemorias);

				void agregarMemoria(t_hiloTabla* hiloTabla) {
					t_bloqueo* idYMutex = malloc(sizeof(t_bloqueo));
					idYMutex->id = memoria_fd;
					pthread_mutex_init(&(idYMutex->mutex), NULL);
					list_add(hiloTabla->cosasABloquear, idYMutex);
				}

				char* mensaje = string_from_format("Se conecto la memoria %d", memoria_fd);
				enviarHandshakeLFS(tamanioValue, memoria_fd);
				pthread_mutex_lock(&mutexTablasParaCompactaciones);
				list_iterate(tablasParaCompactaciones, (void*)agregarMemoria);
				pthread_mutex_unlock(&mutexTablasParaCompactaciones);

				log_debug(logger_LFS, mensaje);
				pthread_detach(hiloRequest);
				free(mensaje);

			} else {
				char* error = string_from_format("Error al iniciar el hilo de la memoria %d", memoria_fd);
				log_error(logger_LFS, error);
				free(error);
			}
		}
	}
	return NULL;
}

void* conectarConMemoria(void* arg) {
	int memoria_fd = (int)arg;
	while (1) {
		t_paquete* paqueteRecibido = recibir(memoria_fd);
		int palabraReservada = paqueteRecibido->palabraReservada;
		//si viene -1 es porque se desconecto la memoria
		if (palabraReservada == COMPONENTE_CAIDO){
			void eliminarMemoria(t_hiloTabla* hiloTabla){
				int encontrarMemoria(t_bloqueo* idYMutex){
					return idYMutex->id == memoria_fd;
				}
				list_remove_and_destroy_by_condition(hiloTabla->cosasABloquear, (void*)encontrarMemoria ,(void*)liberarMutexTabla);
			}

			pthread_mutex_lock(&mutexTablasParaCompactaciones);
			list_iterate(tablasParaCompactaciones, (void*)eliminarMemoria);
			pthread_mutex_unlock(&mutexTablasParaCompactaciones);

			eliminar_paquete(paqueteRecibido);
			log_debug(logger_LFS, "Se desconecto la memoria %i", memoria_fd);
			close(memoria_fd);
			break;
		}
		log_info(logger_LFS, "Request: %s de la memoria %i",paqueteRecibido->request, memoria_fd);
		char** requestSeparada = separarRequest(paqueteRecibido->request);

		interpretarRequest(palabraReservada, paqueteRecibido->request, &memoria_fd);

		liberarArrayDeChar(requestSeparada);

		eliminar_paquete(paqueteRecibido);
	}
	return NULL;
}

void* escucharCambiosEnConfig(void* arg) {

	char buffer[BUF_LEN];
	file_descriptor = inotify_init();
	if (file_descriptor < 0) {
		log_error(logger_LFS, "Inotify no se pudo inicializar correctamente");
		return NULL;
	}
	char* configPath;
	if(arg != NULL){
		configPath = (char*) arg;
	}else{
		configPath = "/home/utnso/tp-2019-1c-bugbusters/lfs/LissandraFileSystem.config";
	}

	watch_descriptor = inotify_add_watch(file_descriptor, configPath, IN_MODIFY);
	while(1) {
		int length = read(file_descriptor, buffer, BUF_LEN);
		log_info(logger_LFS, "Cambio el archivo de config");
		pthread_mutex_lock(&mutexConfig);
		config_destroy(config);
		config = leer_config(configPath);
		pthread_mutex_unlock(&mutexConfig);
		if (length < 0) {
			log_error(logger_LFS, "Error en inotify");
			return NULL;
		} else {
			pthread_mutex_lock(&mutexRetardo);
			if(config_has_property(config, "RETARDO")){
				retardo = config_get_int_value(config, "RETARDO");
			}else{
				log_error(logger_LFS, "Error en inotify, sus cambios no han sido actualizados");
			}
			pthread_mutex_unlock(&mutexRetardo);
			pthread_mutex_lock(&mutexTiempoDump);
			if(config_has_property(config, "TIEMPO_DUMP")){
				tiempoDump = config_get_int_value(config, "TIEMPO_DUMP");
			}else{
				log_error(logger_LFS, "Error en inotify, sus cambios no han sido actualizados");
			}
			pthread_mutex_unlock(&mutexTiempoDump);
		}

		inotify_rm_watch(file_descriptor, watch_descriptor);
		close(file_descriptor);

		file_descriptor = inotify_init();
		if (file_descriptor < 0) {
			log_error(logger_LFS, "Inotify no se pudo inicializar correctamente");
		}

		watch_descriptor = inotify_add_watch(file_descriptor, configPath, IN_MODIFY);
	}
}
