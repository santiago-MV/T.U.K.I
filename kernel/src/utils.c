#include "utils.h"



t_list* recibir_instrucciones(int socket_cliente)
{
	int size;
	void * buffer;
	
	buffer = recibir_buffer(&size, socket_cliente);
	t_list* instrucciones;
	instrucciones = deserializarListaInstruccion(buffer,size);
	
	return instrucciones;
}

int recibir_direccion_fisica(int conexion){
	int size;
	void * buffer;
	int direccion;
	int cod_op = recibir_operacion(conexion);
	if(cod_op==DIREC_FISICA){
		buffer = recibir_buffer(&size, conexion);
		memcpy(&direccion, buffer,sizeof(int));
		free(buffer);
	}else{
		return -1;
	}
	return direccion;
}

void printListaInstruccion(t_list* instrucciones){
	printf("\n");
	for(int j=0;j<list_size(instrucciones);j++){
		printInstruccion(list_get(instrucciones, j));
	}
}
void printInstruccion(instruction_set* i){
	printf("%s",i->instruccion);
	for(int j=0;j<string_array_size(i->params);j++){
		printf(" %s",i->params[j]);
	}
}
void enviar_pcb(t_pcb* pcb, int conexion_cpu){
	t_paquete* paquete_pcb = crear_paquete_pcb(pcb);

	enviar_paquete(paquete_pcb, conexion_cpu);

	eliminar_paquete(paquete_pcb);
}
void enviar_pcb_cpu(t_pcb* pcb, int conexion_cpu){
	t_paquete* paquete_pcb = crear_paquete_pcb(pcb);
	paquete_pcb->codigo_operacion=RESPONSE;
	enviar_paquete(paquete_pcb, conexion_cpu);

	eliminar_paquete(paquete_pcb);
}
t_list* deserializarListaInstruccion(void* buffer,int size){
	int offset = 0;
	
	t_list* instrucciones = list_create();
	int tamanio;
	while(offset < size)
	{ 
		//Deserializar instruccion y meter en lista
		int cantParams;
		instruction_set* i = malloc(sizeof(instruction_set));

		// Crear nueva instancia en cada iteración
		//No entiendo porque viene el tamaño entero de la primara instruccion
		memcpy(&tamanio, buffer + offset, sizeof(int));
		offset += sizeof(int);

		memcpy(&tamanio, buffer + offset, sizeof(int));
		offset += sizeof(int);
		//Sacamos instruccion
		i->instruccion = malloc(tamanio + 1);
		memcpy(i->instruccion, buffer+offset, tamanio);
		i->instruccion[tamanio] = '\0';
		offset+=tamanio;
		//OBTENEMOS PARAMETROS
		//Sacamos cantidad parametros
		memcpy(&cantParams, buffer + offset, sizeof(int));
		offset+=sizeof(int);
		
		i->params = malloc((cantParams + 1) * sizeof(char*));

		for (int j = 0; j < cantParams; j++){
			memcpy(&tamanio, buffer + offset, sizeof(int));
			offset += sizeof(int);

			i->params[j] = malloc(tamanio + 1);
			memcpy(i->params[j], buffer + offset, tamanio);
			i->params[j][tamanio] = '\0';
			offset += tamanio;
		}

		// Asegurarse de que el último elemento sea NULL para indicar el final de la lista
		i->params[cantParams] = NULL;
		list_add(instrucciones, i);
	}
	free(buffer);
	return instrucciones;
}

void terminar_conexion(int conexion){
	if(conexion!=0){
		liberar_conexion(conexion);
	}
}

int encontrarIndiceRecursoEnLista(t_list* recursos, char* recurso) {
    for (int i = 0; i < list_size(recursos); i++) {
        t_recurso* elem = list_get(recursos, i);
        if (strcmp(elem->recurso, recurso) == 0) {
            return i;
        }
    }
    return -1;
}

void liberarRecursosAsignados(t_list* recursosDisponibles, t_list* recursosAsignados) {
    for (int i = 0; i < list_size(recursosAsignados); i++) {
        t_recurso* recursoAsignado = list_get(recursosAsignados, i);
        for (int j = 0; j < list_size(recursosDisponibles); j++) {
            t_recurso* recursoDisponible = list_get(recursosDisponibles, j);
            if (strcmp(recursoAsignado->recurso, recursoDisponible->recurso) == 0) {
                recursoDisponible->cantRecurso += recursoAsignado->cantRecurso;
                break; // Se encontró el recurso asignado, se sale del bucle interno
            }
        }
    }

    //list_destroy_and_destroy_elements(recursosAsignados, free);
}

void terminar_conexiones(int conexion_mem, int conexion_fs){
	terminar_conexion(conexion_mem);
	terminar_conexion(conexion_fs);
}

t_list* recibir_tabla_segmentos(int con){
	t_list* tablas = list_create();
	while(recibir_operacion(con)==TABLA_PROCESO){
		int size;
		void * buffer;
		int cant=0;
		int offset=0;

		t_tabla_proceso* table = malloc(sizeof(t_tabla_proceso));

		buffer = recibir_buffer(&size, con);

		memcpy(&cant, buffer + offset, sizeof(int));
		offset+=sizeof(int);

		memcpy(&table->cant_segmentos_actuales, buffer + offset, sizeof(int));
		offset+=sizeof(int);

		table->segmentos = calloc(cant * sizeof(t_segmento), sizeof(uint32_t));

		memcpy(&table->pid, buffer + offset, sizeof(int));
		offset+=sizeof(int);

		for(int i=0; i<cant; i++){
			memcpy(&table->segmentos[i].base_address, buffer + offset, sizeof(int));
			offset+=sizeof(int);
			memcpy(&table->segmentos[i].size, buffer + offset, sizeof(int));
			offset+=sizeof(int);
			memcpy(&table->segmentos[i].id, buffer + offset, sizeof(int));
			offset+=sizeof(int);
		}

		list_add(tablas,table);

		free(buffer);
	}

	return tablas;
}