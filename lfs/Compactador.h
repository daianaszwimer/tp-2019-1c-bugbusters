#include "Helper.h"

void* hiloCompactacion(void*);

void compactar(char*);

void renombrarTmp_a_TmpC(char*, struct dirent*, DIR*);
t_list* leerDeTodosLosTmpC(char*, struct dirent*, DIR*, t_list*, int, int);
t_list* leerDeTodasLasParticiones(char*, t_list*, int);
void liberarBloquesDeTmpCyParticiones(char*, struct dirent*, DIR*, t_list*);
void guardarDatosNuevos(char*, t_list*, t_list*, int, int);
void setBlockTo(char*, int);
void procesarRequestsEncoladas(char*);
void* procesarRequests(void*);

void eliminarParticion(t_int*);
void eliminarRegistro(t_registro*);
