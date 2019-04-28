#include "lfs.h"
int main(void){
	t_log* logger_LFS;
	t_config* config = leer_config("/home/utnso/tp-2019-1c-bugbusters/lfs/lfs.config");

	logger_LFS = log_create("lfs.log", "Lfs", 1, LOG_LEVEL_DEBUG);
	log_info(logger_LFS, "----------------INICIO DE LISSANDRA FS--------------");

	int lissandraFS_fd = iniciar_servidor(config_get_string_value(config, "IP"),config_get_string_value(config, "PUERTO"));
	log_info(logger_LFS, "Lissandra lista para recibir a Memoria");

	int memoria_fd = esperar_cliente(lissandraFS_fd);

}
