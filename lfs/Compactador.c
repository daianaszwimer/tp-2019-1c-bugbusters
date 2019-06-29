#include "Compactador.h"

void compactar(char* pathTabla) {

	DIR *tabla;
	struct dirent *archivoDeLaTabla;
	t_list* registrosDeParticiones;
	t_list* particiones = list_create();
	char* puntoDeMontaje = config_get_string_value(config, "PUNTO_MONTAJE");

	// Leemos el tamanio del bloque
	char* pathMetadata = string_from_format("%sMetadata/Metadata.bin", puntoDeMontaje);
	t_config* configMetadata = config_create(pathMetadata);
	int tamanioBloque = config_get_int_value(configMetadata, "BLOCK_SIZE");
	free(pathMetadata);
	config_destroy(configMetadata);

	// Leemos el numero de particiones de la tabla
	char* pathMetadataTabla = string_from_format("%s/Metadata.bin", pathTabla);
	t_config* configMetadataTabla = config_create(pathMetadataTabla);
	int numeroDeParticiones = config_get_int_value(configMetadataTabla, "PARTITIONS");
	free(pathMetadataTabla);
	config_destroy(configMetadataTabla);

	t_registro* obtenerTimestampMasAltoSiExiste(t_registro* registroDeTmpC) {
		int tieneMismaKey(t_registro* registroBuscado) {
			return registroDeTmpC->key == registroBuscado->key;
		}
    	t_registro* registroEncontrado = list_find(registrosDeParticiones, (void*)tieneMismaKey);
    	if(registroEncontrado != NULL) {
    		if(registroEncontrado->timestamp > registroDeTmpC->timestamp) {
    			registroDeTmpC->timestamp = registroEncontrado->timestamp;
    			registroDeTmpC->value = strdup(registroEncontrado->value);
    		}
    	}
    	return registroDeTmpC;
	}

	// Abrimos el directorio de la tabla para poder recorrerlo
	if((tabla = opendir(pathTabla)) == NULL) {
		perror("opendir");
	}

	// Renombramos los tmp a tmpc
	renombrarTmp_a_TmpC(pathTabla, archivoDeLaTabla, tabla);

	// Leemos todos los registros de los temporales a compactar y los guardamos en una lista
	t_list* registrosDeTmpC = leerDeTodosLosTmpC(pathTabla, archivoDeLaTabla, tabla, particiones, numeroDeParticiones, puntoDeMontaje);

	/*
	// Verificamos si hay datos que compactar
	if (registrosDeTmpC->elements_count != 0) {

		// Leemos todos los registros de las particiones y los guardamos en una lista
		registrosDeParticiones = leerDeTodasLasParticiones(pathTabla, particiones, puntoDeMontaje);

		// Mergeamos la lista de registros de tmpC con la lista de registros de las particiones y obtenemos una nueva lista
		// (filtrando por timestamp mas alto en caso de que hayan keys repetidas)
		t_list* registrosAEscribir = list_map(registrosDeTmpC, (void*) obtenerTimestampMasAltoSiExiste);

		// TODO: Bloquear tabla

		// Liberamos los bloques que contienen el archivo “.tmpc” y los que contienen el archivo “.bin”
		liberarBloquesDeTmpCyParticiones(pathTabla, archivoDeLaTabla, tabla, particiones, puntoDeMontaje);

		// Grabamos los datos en el nuevo archivo “.bin”
		guardarDatosNuevos(pathTabla, registrosAEscribir, particiones, tamanioBloque, puntoDeMontaje, numeroDeParticiones);

		// TODO: Desbloquear la tabla y dejar un registro de cuánto tiempo estuvo bloqueada la tabla para realizar esta operatoria.
		list_destroy_and_destroy_elements(registrosDeParticiones, (void*) eliminarRegistro);
		list_destroy_and_destroy_elements(registrosAEscribir, (void*) eliminarRegistro);
	}*/

	// Cerramos el directorio
	if (closedir(tabla) == -1) {
		perror("closedir");
	}

	list_destroy_and_destroy_elements(registrosDeTmpC, (void*) eliminarRegistro);
	list_destroy_and_destroy_elements(particiones, (void*) eliminarParticion);
	//free(puntoDeMontaje);
}

void renombrarTmp_a_TmpC(char* pathTabla, struct dirent* archivoDeLaTabla, DIR* tabla) {
	while ((archivoDeLaTabla = readdir(tabla)) != NULL) {
		if (string_ends_with(archivoDeLaTabla->d_name, ".tmp")) {
			char* pathTmp = string_from_format("%s/%s", pathTabla, archivoDeLaTabla->d_name);
			char* pathTmpC = string_from_format("%s/%s%c", pathTabla, archivoDeLaTabla->d_name, 'c');
			int resultadoDeRenombrar = rename(pathTmp, pathTmpC);
			if (resultadoDeRenombrar != 0) {
				log_error(logger_LFS, "No se pudo renombrar el temporal");
			}
			free(pathTmp);
			free(pathTmpC);
		}
	}
	rewinddir(tabla);
}

t_list* leerDeTodosLosTmpC(char* pathTabla, struct dirent* archivoDeLaTabla, DIR* tabla, t_list* particiones, int numeroDeParticiones, char* puntoDeMontaje) {

	t_registro* tRegistro;
	t_int* particion;
	int particionDeEstaKey;
	t_list* registrosDeTmpC = list_create();

	int existeParticion(t_int* particionAComparar) {
		return particionAComparar->valor == particionDeEstaKey;
	}

	int tieneMismaKey(t_registro* registroBuscado) {
		return tRegistro->key == registroBuscado->key;
	}

	while ((archivoDeLaTabla = readdir(tabla)) != NULL) {

		if (string_ends_with(archivoDeLaTabla->d_name, ".tmpc")) {

			char* pathTmpC = string_from_format("%s/%s", pathTabla, archivoDeLaTabla->d_name);
			t_config* configTmpC = config_create(pathTmpC);
			char** bloques = config_get_array_value(configTmpC, "BLOCKS");
			free(pathTmpC);
			config_destroy(configTmpC);

			int i = 0;
			int j = 0;

			char* registro = malloc(sizeof(char));
			//char* registro = malloc((sizeof(uint16_t) + config_get_int_value(config, "TAMAÑO_VALUE") + sizeof(unsigned long long)));
			//strcpy(registro, "");
			while (bloques[i] != NULL) {
				char* pathBloque = string_from_format("%sBloques/%s.bin", puntoDeMontaje, bloques[i]);
				FILE* bloque = fopen(pathBloque, "r");
				free(pathBloque);
				if (bloque == NULL) {
					perror("Error");
				}
				do {
					char caracterLeido = fgetc(bloque);
					if (feof(bloque)) {
						break;
					}
					if (caracterLeido == '\n') {
						registro = realloc(registro, sizeof(char) * (j+1));
						strcat(registro, "\0");
						char** registroSeparado = string_n_split(registro, 3, ";");
						tRegistro = (t_registro*) malloc(sizeof(t_registro));
						convertirTimestamp(registroSeparado[0], &(tRegistro->timestamp));
						tRegistro->key = convertirKey(registroSeparado[1]);
						tRegistro->value = strdup(registroSeparado[2]);

						liberarArrayDeChar(registroSeparado);

						particionDeEstaKey = tRegistro->key % numeroDeParticiones;

						particion = list_find(particiones, (void*) existeParticion);
						if (particion == NULL) {
							t_int* particionAAgregar = malloc(sizeof(t_int*));
							particionAAgregar->valor = particionDeEstaKey;
							list_add(particiones, particionAAgregar);
						}

						t_registro* registroEncontrado = list_find(registrosDeTmpC, (void*) tieneMismaKey);
						if (registroEncontrado != NULL) {
							if (tRegistro->timestamp > registroEncontrado->timestamp) {
								list_remove_and_destroy_by_condition(registrosDeTmpC, (void*) tieneMismaKey, (void*) eliminarRegistro);
								list_add(registrosDeTmpC, tRegistro);
							}
						} else {
							list_add(registrosDeTmpC, tRegistro);
						}
						j = 0;
						strcpy(registro, "");
					} else {
						registro = realloc(registro, sizeof(char) * (j+1));
						registro[j] = caracterLeido;
						j++;
					}
				} while (1);
				fclose(bloque);
				i++;
			}
			liberarArrayDeChar(bloques);
			free(registro);
		}
	}
	rewinddir(tabla);
	return registrosDeTmpC;
}

t_list* leerDeTodasLasParticiones(char* pathTabla, t_list* particiones, char* puntoDeMontaje) {

	t_int* particion;
	t_registro* tRegistro;
	t_list* registrosDeParticiones = list_create();

	for (int i = 0; list_get(particiones, i) != NULL; i++) {
		particion = list_get(particiones, i);
		char* pathParticion = string_from_format("%s/%d.bin", pathTabla, particion->valor);
		t_config* particionConfig = config_create(pathParticion);
		char** bloques = config_get_array_value(particionConfig, "BLOCKS");
		char* registro = malloc((int) (sizeof(unsigned long long) + sizeof(uint16_t) + config_get_int_value(config, "TAMAÑO_VALUE")));
		int z = 0;
		int j = 0;
		while (bloques[z] != NULL) {
			char* pathBloque = string_from_format("%sBloques/%s.bin", puntoDeMontaje, bloques[z]);
			FILE* bloque = fopen(pathBloque, "r");
			if (bloque == NULL) {
				perror("Error");
			}
			do {
				char caracterLeido = fgetc(bloque);
				if (feof(bloque)) {
					break;
				}
				if (caracterLeido == '\n') {
					char** registroSeparado = string_n_split(registro, 3, ";");
					tRegistro = (t_registro*) malloc(sizeof(t_registro));
					convertirTimestamp(registroSeparado[0], &tRegistro->timestamp);
					tRegistro->key = convertirKey(registroSeparado[1]);
					tRegistro->value = strdup(registroSeparado[2]);
					list_add(registrosDeParticiones, tRegistro);
					j = 0;
					strcpy(registro, "");
				} else {
					registro[j] = caracterLeido;
					j++;
				}
			} while (1);
			free(pathBloque);
			fclose(bloque);
			z++;
		}
		free(registro);
		free(bloques);
	}
	return registrosDeParticiones;
}

void liberarBloquesDeTmpCyParticiones(char* pathTabla, struct dirent* archivoDeLaTabla, DIR* tabla, t_list* particiones, char* puntoDeMontaje) {

	while ((archivoDeLaTabla = readdir(tabla)) != NULL) {
		if (string_ends_with(archivoDeLaTabla->d_name, ".tmpc")) {
			char* pathTmpC = string_from_format("%s/%s", pathTabla, archivoDeLaTabla->d_name);
			t_config* configTmpC = config_create(pathTmpC);
			char** bloques = config_get_array_value(configTmpC, "BLOCKS");
			int i = 0;
			while (bloques[i] != NULL) {
				char* pathBloque = string_from_format("%sBloques/%s.bin", puntoDeMontaje, bloques[i]);
				FILE* bloque = fopen(pathBloque, "w");
				bitarray_clean_bit(bitarray, i);
				free(pathBloque);
				fclose(bloque);
				i++;
			}
			remove(pathTmpC);
			free(pathTmpC);
			config_destroy(configTmpC);
		}

		if (string_ends_with(archivoDeLaTabla->d_name, ".bin") && !string_equals_ignore_case(archivoDeLaTabla->d_name, "Metadata.bin")) {
			char** numeroDeParticionString = string_split(archivoDeLaTabla->d_name, ".");
			int numeroDeParticion = strtol(numeroDeParticionString[0], NULL, 10);

			int particionActual(t_int* particion_actual) {
				return particion_actual->valor == numeroDeParticion;
			}

			t_int* particionEncontrada = list_find(particiones, (void*) particionActual);
			if (particionEncontrada != NULL) {
				char* particionPath = string_from_format("%s/%s", pathTabla, archivoDeLaTabla->d_name);
				t_config* configParticion = config_create(particionPath);
				char** bloques = config_get_array_value(configParticion, "BLOCKS");
				int i = 0;
				while (bloques[i] != NULL) {
					char* pathBloque = string_from_format("%sBloques/%s.bin", puntoDeMontaje, bloques[i]);
					FILE* bloque = fopen(pathBloque, "w");
					bitarray_clean_bit(bitarray, i);
					free(pathBloque);
					fclose(bloque);
					i++;
				}
				free(particionPath);
				config_destroy(configParticion);
			}
		}
	}
}

void guardarDatosNuevos(char* pathTabla, t_list* registrosAEscribir, t_list* particiones, int tamanioBloque, char* puntoDeMontaje, int numeroDeParticiones) {

	errorNo error;
	t_int* particion;

	int keyCorrespondeAParticion(t_registro* registro) {
		return particion->valor == registro->key % numeroDeParticiones;
	}

	for (int i = 0; list_get(particiones, i) != NULL; i++) {
		particion = list_get(particiones, i);
		char* pathParticion = string_from_format("%s/%d.bin", pathTabla, particion->valor);
		t_config* configParticion = config_create(pathParticion);

		t_list* registrosPorParticion = list_filter(registrosAEscribir, (void*) keyCorrespondeAParticion);

		char* datosACompactar = malloc(sizeof(unsigned long long) + sizeof(uint16_t) + (size_t) config_get_int_value(config, "TAMAÑO_VALUE"));
		strcpy(datosACompactar, "");
		for (int j = 0; list_get(registrosPorParticion, j) != NULL; j++) {
			t_registro* registro = list_get(registrosPorParticion, j);
			string_append_with_format(&datosACompactar, "%llu;%u;%s\n", registro->timestamp, registro->key, registro->value);
		}

		list_destroy_and_destroy_elements(registrosPorParticion, (void*) eliminarParticion);

		int cantidadDeBloquesAPedir = strlen(datosACompactar) / tamanioBloque;
		if (strlen(datosACompactar) % tamanioBloque != 0) {
			cantidadDeBloquesAPedir++;
		}
		char* tamanioDatos = strdup("");
		sprintf(tamanioDatos, "%d", strlen(datosACompactar));
		config_set_value(configParticion, "SIZE", tamanioDatos);
		free(tamanioDatos);
		char* bloques = strdup("");
		for (int i = 0; i < cantidadDeBloquesAPedir; i++) {
			int bloqueDeParticion = obtenerBloqueDisponible(&error); //si hay un error se setea en errorNo
			if (bloqueDeParticion == -1) {
				log_info(logger_LFS, "no hay bloques disponibles");
			} else {
				if (i == cantidadDeBloquesAPedir - 1) {
					string_append_with_format(&bloques, "%d", bloqueDeParticion);
				} else {
					string_append_with_format(&bloques, "%d,", bloqueDeParticion);
				}
				char* pathBloque = string_from_format("%sBloques/%d.bin", puntoDeMontaje, bloqueDeParticion);
				FILE* bloque = fopen(pathBloque, "a+");
				if (cantidadDeBloquesAPedir != 1 && i < cantidadDeBloquesAPedir - 1) {
					char* registrosAEscribirString = string_substring_until(datosACompactar, tamanioBloque);
					char* stringAuxiliar = string_substring_from(datosACompactar, tamanioBloque);
					free(datosACompactar);
					datosACompactar = stringAuxiliar;
					fprintf(bloque, "%s", registrosAEscribirString);
					free(registrosAEscribirString);
				} else {
					fprintf(bloque, "%s", datosACompactar);
				}
				fclose(bloque);
				free(pathBloque);
			}
		}
		char* blocks = string_from_format("[%s]", bloques);
		config_set_value(configParticion, "BLOCKS", blocks);
		free(blocks);
		free(bloques);
		free(datosACompactar);
		config_save(configParticion);
		config_destroy(configParticion);
	}
}

void eliminarParticion(t_int* particionAEliminar) {
	free(particionAEliminar);
}

void eliminarRegistro(t_registro* registroAEliminar) {
	free(registroAEliminar->value);
	free(registroAEliminar);
}
