#include "Lissandra.h"

void* hiloCompactacion(void*);

void compactar(char*);

void renombrarTmp_a_TmpC(char*, struct dirent*, DIR*);
t_list* leerDeTodosLosTmpC(char*, struct dirent*, DIR*, t_list*, int, char*);
t_list* leerDeTodasLasParticiones(char*, t_list*, char*);
void liberarBloquesDeTmpCyParticiones(char*, struct dirent*, DIR*, t_list*, char*);
void guardarDatosNuevos(char*, t_list*, t_list*, int, char*, int);

void eliminarParticion(t_int*);
void eliminarRegistro(t_registro*);
