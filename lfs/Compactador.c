#include "Compactador.h"

void compactacion(char* pathTabla) {
	//int numeroTemporal = 0;
	//char* puntoDeMontaje = config_get_string_value(config, "PUNTO_MONTAJE");
	//char* pathMetadataTabla = string_from_format("%s/Metadata.bin", pathTabla);
	//char* pathMetadata = string_from_format("%sMetadata/Metadata.bin", puntoDeMontaje);
	//t_config* configMetadata = config_create(pathMetadata);
	//t_config* configMetadataTabla = config_create(pathMetadataTabla);
	//int tamanioValue = config_get_int_value(config, "TAMAÑO_VALUE");
	//int numeroDeParticiones = config_get_int_value(configMetadataTabla, "PARTITIONS");
	//FILE* fileTmp;

	DIR *tabla;
	struct dirent *archivoDeLaTabla;

	if((tabla = opendir(pathTabla)) == NULL) {
		perror("opendir");
	}

	// Renombramos los tmp a tmpc
	while((archivoDeLaTabla = readdir(tabla)) != NULL) {
		if(string_ends_with(archivoDeLaTabla->d_name, ".tmp")) {
			char* pathTmp = string_from_format("%s/%s", pathTabla, archivoDeLaTabla->d_name);
			char* pathTmpC = string_from_format("%s/%s%c", pathTabla, archivoDeLaTabla->d_name, 'c');
			int resultadoDeRenombrar = rename(pathTmp, pathTmpC);
			if(resultadoDeRenombrar != 0) {
				log_info(logger_LFS, "No se pudo renombrar el temporal");
			}
		}
	}



	if (closedir(tabla) == -1) {
		perror("closedir");
	}

	/*fileTmp = fopen(pathTmp, "r");
				if (fileTmp != NULL) { // Crear tmpc de los tmp
					char* pathTmpC = string_from_format("%s%c", pathTmp, 'c');
					FILE* fileTmpC = fopen(pathTmpC, "w+");
					char* datosDelArchivoTemporal = malloc((int) (sizeof(uint16_t) + tamanioValue + sizeof(unsigned long long)));
					while (fscanf(fileTmp, "%s", datosDelArchivoTemporal) == 1) {
						fprintf(fileTmpC, "%s", datosDelArchivoTemporal);
						fputc('\n', fileTmpC);
					}
					fclose(fileTmp);
					rewind(fileTmpC);
				}*/
	/*do {
		char* pathTmp = string_from_format("%s/%d.tmp", pathTabla,
				numeroTemporal);
		fileTmp = fopen(pathTmp, "r");
		if (fileTmp != NULL) { // Crear tmpc de los tmp
			char* pathTmpC = string_from_format("%s/%d.tmpc", pathTabla,
					numeroTemporal);
			FILE* fileTmpC = fopen(pathTmpC, "w+");
			char* datosDelArchivoTemporal = malloc(
					(int) (sizeof(uint16_t) + tamanioValue
							+ sizeof(unsigned long long)));
			while (fscanf(fileTmp, "%s", datosDelArchivoTemporal) == 1) {
				fprintf(fileTmpC, "%s", datosDelArchivoTemporal);
				fputc('\n', fileTmpC);
			}
			fclose(fileTmp);
			rewind(fileTmpC);
			while (fscanf(fileTmpC, "%s", datosDelArchivoTemporal) == 1) {
				int escrito = 0;
				char** registro = string_n_split(datosDelArchivoTemporal, 3,
						";");
				int particionDeEstaKey = convertirKey(registro[0])
						% numeroDeParticiones;
				char* pathParticion = string_from_format("%s/%d.bin", pathTabla,
						particionDeEstaKey);
				t_config* particion = config_create(pathParticion);
				char** bloques = config_get_array_value(particion, "BLOCKS");
				for (int i = 0; escrito == 0; i++) {
					if (bloques[i] != NULL) {
						char* pathBloque = string_from_format(
								"%sBloques/%s.bin", puntoDeMontaje, bloques[i]);
						FILE* bloque = fopen(pathBloque, "w+");
						fseek(bloque, 0L, SEEK_END);
						long int tamanioBloqueReal = ftell(bloque);
						fclose(bloque);
						bloque = fopen(pathBloque, "a");
						if ((tamanioBloqueReal + sizeof(uint16_t) + tamanioValue
								+ sizeof(unsigned long long) + 1)
								<= config_get_int_value(configMetadata,
										"BLOCK_SIZE")) {
							fprintf(bloque, "%s", datosDelArchivoTemporal);
							escrito = 1;
						}
						free(pathBloque);
						fclose(bloque);
					} else {
						errorNo errorNo;
						int bloqueDeParticion = obtenerBloqueDisponible(
								&errorNo);
						if (bloqueDeParticion == -1) {
							log_info(logger_LFS, "no hay bloques disponibles");
						} else {
							char* bloquesString = config_get_string_value(
									particion, "BLOCKS");
							bloquesString = strtok(bloquesString, "]");
							string_append_with_format(&bloquesString, "%d]",
									bloqueDeParticion);
							config_set_value(particion, "BLOCKS",
									bloquesString);
							free(bloquesString);
						}
					}
				}
				free(bloques);
				free(registro);
			}
			fclose(fileTmpC);
			free(pathTmpC);
			free(datosDelArchivoTemporal);
		}
		numeroTemporal++;
		free(pathTmp);
	} while (fileTmp != NULL);*/

		// TODO: Separar la lista de datos del tmp en particiones
		// TODO: Comparar con los .bin (leer primero el bin y guardarlo en una lista para comparar listas es mas rapido?
		// TODO: Quedarse con los timestamps mas recientes
		// TODO: Bloquear la tabla: (chmod o mutex?)
		// TODO: Liberar bloques
		// TODO: Pedir mas bloques
		// TODO: Generar nuevo .bin
		// TODO: Desbloquear la tabla y dejar un registro de cuanto tardo la compactacion
}
