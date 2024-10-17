#include "utils.h"

#include "utils.h"

char* lista_params_to_string(instruction_set* i){
    char* string_final = string_new();
    int k = 0;
    while(i->params[k] != NULL){
        string_append(&string_final, " ");
        string_append(&string_final, i->params[k]);
        k++;
    }
    if(k == 0) {
        string_append(&string_final, " SIN PARAMETROS");
    }
    return string_final;
}

void liberar_direccion(void* elemento){
    t_dir_logica_proc* dir_fisica = (t_dir_logica_proc*)elemento;
    free(dir_fisica);
}
int concatenarEnteros(int a, int b) {
    int multiplicador = 1;
    while (multiplicador <= b) {
        multiplicador *= 10;
    }
    return a * multiplicador + b;
}
t_segmento* get_t_segmento_by_id(t_list* lista, int id){
    int tamanio_t_segmento = sizeof(int) + sizeof(u_int32_t) + sizeof(u_int32_t);
    t_segmento* segmento_encontrado = malloc(tamanio_t_segmento);
    int i = 0;

    do{
        segmento_encontrado = list_get(lista, i);
        i++;
    } while( segmento_encontrado && segmento_encontrado->id != id );
    
    return segmento_encontrado;
}
void enviar_seg_fault(int socket_cliente, t_pcb* pcb){
    t_paquete* paquete_seg_fault = crear_paquete_pcb(pcb);
    paquete_seg_fault->codigo_operacion = SEG_FAULT;
    enviar_paquete(paquete_seg_fault, socket_cliente);
    eliminar_paquete(paquete_seg_fault);
}
void enviar_direccion_fisica(char* dir_logica, t_tabla_proceso* tabla,int socket_cliente){
    t_paquete* paquete_direc_fisica = crear_paquete_con_codigo_op(DIREC_FISICA);
    t_dir_logica_proc* dir = dir_logica_a_nro_y_offset(atoi(dir_logica));
    int dir_fisica = mmu(dir,tabla);
    agregar_entero_a_paquete(paquete_direc_fisica, dir_fisica);
    enviar_paquete(paquete_direc_fisica, socket_cliente);
    eliminar_paquete(paquete_direc_fisica);
    liberar_direccion(dir);
}
/*
t_paquete* crear_paquete_movout(int dir_fisica, char* contenido){
    t_paquete* paquete = crear_paquete_con_codigo_op(PAQUETE_MOVOUT);
    int tamanio_buffer = 0;
    size_t tamanio_contenido = 0;

    tamanio_contenido += strlen(contenido);
    
    // tamanio_buffer = dir_fisica + tamanio_contenido + contenido
    tamanio_buffer += sizeof(int) + sizeof(int) + tamanio_contenido;

    int offset = 0;
    void* buffer = malloc(tamanio_buffer);

    // Serializo dir_fisica
    memcpy(buffer + offset, &(dir_fisica), sizeof(int));
    offset += sizeof(int);

    // Serializo contenido
    memcpy(buffer + offset, &(tamanio_contenido), sizeof(int));
    offset += sizeof(int);
    memcpy(buffer + offset, &(contenido), tamanio_contenido);
    offset += tamanio_contenido;

    // Armo paquete
    paquete->buffer->stream = buffer;
    paquete->buffer->size = tamanio_buffer;   
    return paquete;
}
*/

/*
t_paquete* crear_paquete_movin(int dir_fisica, t_registro_info registro_info){
    t_paquete* paquete = crear_paquete_con_codigo_op(PAQUETE_MOVIN);
    size_t tamanio_buffer = 0;

    size_t tamanio_regname = strlen(registro_info.reg_name);
    
    // tamanio_buffer = dir_fisica + tamanio_regname + regname + tamanio_contenido + contenido
    tamanio_buffer += sizeof(int) + sizeof(int) + tamanio_regname + sizeof(int) + registro_info.tamanio_contenido;

    int offset = 0;
    void* buffer = malloc(tamanio_buffer);

    // Serializo dir_fisica
    memcpy(buffer + offset, &(dir_fisica), sizeof(int));
    offset += sizeof(int);

    // Serializo reg_name
    memcpy(buffer + offset, &(tamanio_regname), sizeof(int));
    offset += sizeof(int);
    memcpy(buffer + offset, &(registro_info.reg_name), tamanio_regname);
    offset += tamanio_regname;

    // Serializo contenido
    memcpy(buffer + offset, &(registro_info.tamanio_contenido), sizeof(int));
    offset += sizeof(int);
    memcpy(buffer + offset, &(registro_info.contenido), registro_info.tamanio_contenido);
    offset += registro_info.tamanio_contenido;

    // Armo paquete
    paquete->buffer->stream = buffer;
    paquete->buffer->size = tamanio_buffer;   
    return paquete;
}
*/

int get_tamanio_registro(char* reg_name){
    int tam = 0;
	if(strcmp("AX" , reg_name) == 0){
		tam += 4;
	}
	else if(strcmp("BX" , reg_name) == 0){
		tam += 4;
	}
	else if(strcmp("CX" , reg_name) == 0){
		tam += 4;
	}
	else if(strcmp("DX" , reg_name) == 0){
		tam += 4;
	}
	else if(strcmp("EAX" , reg_name) == 0){
		tam += 8;
	}
	else if(strcmp("EBX" , reg_name) == 0){
		tam += 8;
	}
	else if(strcmp("ECX" , reg_name) == 0){
		tam += 8;
	}
	else if(strcmp("EDX" , reg_name) == 0){
		tam += 8;
	}
	else if(strcmp("RAX" , reg_name) == 0){
		tam += 16;
	}
	else if(strcmp("RBX" , reg_name) == 0){
		tam += 16;
	}
	else if(strcmp("RCX" , reg_name) == 0){
		tam += 16;
	}
	else if(strcmp("RDX" , reg_name) == 0){
		tam += 16;
	}
	return tam;
}
