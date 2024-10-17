#include "utils.h"

void printInstruccion(instruction_set* i){
	printf("%s",i->instruccion);
	for(int j=0;j<string_array_size(i->params);j++){
		printf(" %s",i->params[j]);
	}
	printf("\n");
}

void agregar_instruccion_a_paquete(t_paquete* paquete,instruction_set* i){
	int tamanio_instruccion;
	void* valor = serializar_instruction_set(i,&tamanio_instruccion);
	agregar_a_paquete(paquete,valor,tamanio_instruccion);
	free(valor);
}



t_paquete* paquete_instrucciones(FILE* codigo){
	//Creo paquete instrucciones
	t_paquete* instrucciones = crear_paquete_con_codigo_op(PAQUETE_INSTRUCCIONES);
    char linea[100];

    while (fgets(linea, sizeof(linea), codigo) != NULL) {
        instruction_set* instruction = malloc(sizeof(instruction_set));
		strcpy(linea,string_replace(linea, "\n", ""));
        char** lineaSeparada = string_split(linea, " ");
		
		if (lineaSeparada == NULL) EXIT_FAILURE;

		instruction->instruccion = strdup(lineaSeparada[0]);
		int param_count = string_array_size(lineaSeparada)-1;
        instruction->params = malloc((param_count + 1) * sizeof(char*));
		for (int i = 0; i < param_count; i++) {
            instruction->params[i] = strdup(lineaSeparada[i+1]);
        }
		instruction->params[param_count]=NULL;

		agregar_instruccion_a_paquete(instrucciones,instruction);
		
        string_array_destroy(lineaSeparada);
        free(instruction->instruccion);  // Liberar memoria de instruccion->instruccion
        for (int i = 0; instruction->params[i] != NULL; i++) {
            free(instruction->params[i]);  // Liberar memoria de instruction->params[i]
        }
        free(instruction->params);  // Liberar memoria de instruction->params
        free(instruction);
    }
    return instrucciones;
}
