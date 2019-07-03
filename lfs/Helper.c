#include "Helper.h"

int obtenerBloqueDisponible() {

	int index = 0;
	while (index < blocks && bitarray_test_bit(bitarray, index) == 1) index++;
	if(index >= blocks) {
		index = -1;
	}else{
		bitarray_set_bit(bitarray, index);
	}
	return index;
}

/* vaciarTabla()
  * Parametros:
 * 	-> tabla :: t_tabla*
 * Descripcion: vacia una tabla y todos sus registros
 * Return: void */
void vaciarTabla(t_tabla *tabla) {
	void eliminarRegistros(t_registro* registro) {
	    free(registro->value);
	    free(registro);
	}
    free(tabla->nombreTabla);
    list_clean_and_destroy_elements(tabla->registros, (void*) eliminarRegistros);
    free(tabla->registros);
    free(tabla);
}
