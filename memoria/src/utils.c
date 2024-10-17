#include"utils.h"

t_config* config;
t_log* logger;
t_segmento* segmento_0;
t_segmento* segmento_vacio;
int segmento_id_global = 1;
void* memoria;
t_list* lista_segmentos_actuales;

int recibir_crear_segmento(int conexion, int* tam, int* sid){
	int size;
	void* buffer = recibir_buffer(&size, conexion);
	int pid;
	memcpy(&pid, buffer, sizeof(int));
    memcpy(tam, buffer + sizeof(int), sizeof(int));
    memcpy(sid, buffer + 2*sizeof(int), sizeof(int));
    free(buffer);
	return pid;
}
int recibir_fin_proceso(int conexion){
    int size;
	void* buffer = recibir_buffer(&size, conexion);
	int pid;
	memcpy(&pid, buffer, sizeof(int));
    free(buffer);
	return pid;
}
void fin_proceso(int pid){
    t_tabla_proceso* tabla = obtener_tabla_con_pid(pid, lista_tablas);
    for(int i = 0; i<cant_segmentos; i++){
       if(tabla->segmentos[i]->id == segmento_vacio->id)continue;
       if(tabla->segmentos[i]->id == 0) continue;
       eliminar_segmento(pid,i, false);
    }
    list_remove_element(lista_tablas, (void*)tabla);
    
    free(tabla->segmentos);
    free(tabla);
}
int recibir_eliminar_segmento(int conexion, int* sid){
    int size;
	void* buffer = recibir_buffer(&size, conexion);
	int pid;
	memcpy(&pid, buffer, sizeof(int));
    memcpy(sid, buffer + sizeof(int), sizeof(int));
    free(buffer);
	return pid;
}
int recibir_pedido_lectura(int conexion, int* tamanio_a_leer, int* pid){
    int size;
	void* buffer = recibir_buffer(&size, conexion);
	int direccion_fisica;
	memcpy(pid, buffer, sizeof(int));
    memcpy(&direccion_fisica, buffer + sizeof(int), sizeof(int));
    memcpy(tamanio_a_leer, buffer + 2*sizeof(int), sizeof(int));
    free(buffer);
	return direccion_fisica;
}
char* recibir_pedido_escritura(int conexion, int* dir_fisica, int* pid){
    int size;
	void* buffer = recibir_buffer(&size, conexion);
	int tamanio_cadena, offset = 0;
	memcpy(pid, buffer, sizeof(int));
    offset +=sizeof(int);
    memcpy(dir_fisica, buffer + offset, sizeof(int));
    offset +=sizeof(int);
    memcpy(&tamanio_cadena, buffer + offset, sizeof(int)); 
    offset +=sizeof(int);
    char* valor_a_escribir = malloc(tamanio_cadena+1);
    memcpy(valor_a_escribir, buffer + offset, tamanio_cadena);
    valor_a_escribir[tamanio_cadena]='\0';
    free(buffer);
	return valor_a_escribir;
}
void leer_direccion_fisica(char* valor_leido,int dir_fisica, int tamanio_a_leer){
    memcpy(valor_leido, memoria + dir_fisica, tamanio_a_leer);
    usleep(retardo_memoria*1000);
}
void escribir_en_direccion_fisica(char* valor_a_escribir, int dir_fisica){
    memcpy(memoria + dir_fisica, valor_a_escribir, strlen(valor_a_escribir));
    usleep(retardo_memoria*1000);
}
void enviar_ok(int socket){
    t_paquete* paquete_ok = crear_paquete_con_codigo_op(OK);
    enviar_paquete(paquete_ok, socket);
    eliminar_paquete(paquete_ok);
}
void enviar_valor_leido(int socket, char* valor_leido){
    t_paquete* paquete_valor_leido = crear_paquete_con_codigo_op(VALOR_LEIDO);
    agregar_a_paquete(paquete_valor_leido, valor_leido, (int)strlen(valor_leido));
    enviar_paquete(paquete_valor_leido, socket);
    eliminar_paquete(paquete_valor_leido);
}
t_tabla_proceso* obtener_tabla_con_pid(int pid, t_list* lista_tablas){
    for(int i=0 ; i<list_size(lista_tablas);i++ ){
        t_tabla_proceso* tabla = list_get(lista_tablas, i);
        if(pid == tabla->pid) return tabla;
    }
    return NULL;
}
void print_tabla(t_tabla_proceso* tabla){
    printf("PID: %d\n", tabla->pid);
    printf("Cant segmentos actuales: %d\n", tabla->cant_segmentos_actuales);
    for(int i = 0; i<cant_segmentos;i++){
        if(tabla->segmentos[i]->id== segmento_vacio->limite){
            printf("Segmento: %d, VACIO\n", i );
            continue;}
        printf("Segmento: %d, BASE: %d LIMITE: %d SID: %d\n", i, tabla->segmentos[i]->base,tabla->segmentos[i]->limite, tabla->segmentos[i]->id);
    }
}
void enviar_tabla_a_kernel(t_tabla_proceso* tabla, int conexion_kernel){
    t_paquete* paquete_tabla = crear_paquete_con_codigo_op(TABLA_PROCESO);

    int tamanio_serializado = sizeof(t_segmento)* cant_segmentos + 3*sizeof(int);
    int offset=0;
	void* buffer = malloc(tamanio_serializado);
    memset(buffer, 0, tamanio_serializado);

    int cant_segmentos_actuales = tabla->cant_segmentos_actuales;
    int pid = tabla->pid;

    memcpy(buffer +offset, &cant_segmentos, sizeof(int));
    offset += sizeof(int);

    memcpy(buffer + offset, &cant_segmentos_actuales, sizeof(int));
    offset += sizeof(int);

    memcpy(buffer +offset, &pid, sizeof(int));
    offset += sizeof(int);


    for (int i = 0; i < tabla->cant_segmentos_actuales; i++) {
        t_segmento segmento = *(tabla->segmentos[i]);
        int base = segmento.base;
        int limite = segmento.limite;
        int id = segmento.id;

        memcpy(buffer + offset, &base, sizeof(int));
        offset += sizeof(int);

        memcpy(buffer + offset, &limite, sizeof(int));
        offset += sizeof(int);

        memcpy(buffer + offset, &id, sizeof(int));
        offset += sizeof(int);
    }

	paquete_tabla->buffer->stream = buffer;
	paquete_tabla->buffer->size = tamanio_serializado;

    enviar_paquete(paquete_tabla, conexion_kernel);

    eliminar_paquete(paquete_tabla);
}

void inicializar_estructuras_pcb(int pid, int conexion_kernel)
{
	t_tabla_proceso* tabla = malloc(sizeof(t_tabla_proceso));
	t_segmento** segmentos = (t_segmento**)malloc(cant_segmentos * sizeof(t_segmento*));
    tabla->pid = pid;
	log_info(logger,"Se creo la tabla de segmentos para el proceso %d", tabla->pid);
    segmentos[0] = segmento_0;
    tabla->segmentos = segmentos;
    tabla->cant_segmentos_actuales = 1;
    list_add(lista_tablas, tabla);
    for(int i = 1; i < cant_segmentos; i++){
        segmentos[i] = segmento_vacio;
    }
    enviar_tabla_a_kernel(tabla, conexion_kernel);
}

void crear_segmento(int pid, int sid,int tamanio_segmento, int conexion_kernel){
    t_tabla_proceso* tabla = obtener_tabla_con_pid(pid,lista_tablas);
    
    if(tabla == NULL) return ;

    t_segmento* segmento_elegido = buscar_hueco_libre((uint32_t) tamanio_segmento);

    t_paquete* aviso_kernel = malloc(sizeof(t_paquete));
    crear_buffer(aviso_kernel);
	int op_code;
    if (segmento_elegido->id == -1){
        op_code=COMPACTAR;
    }else if(segmento_elegido->id == -2){
        op_code=OUT_OF_MEMORY;
    }else{

        log_info(logger, "PID: %d - Crear Segmento: %d - Base: %d - TAMAÑO: %d", pid, sid, segmento_elegido->base, tamanio_segmento);
        segmento_elegido->id = segmento_id_global;
        tabla->segmentos[sid] = segmento_elegido;
        tabla->cant_segmentos_actuales+=1;
        list_add(lista_segmentos_actuales, segmento_elegido);
        segmento_id_global++;
        op_code = SEGMENTO_CREADO;
        agregar_entero_a_paquete(aviso_kernel, segmento_elegido->base);
    }
    aviso_kernel->codigo_operacion = op_code;
    enviar_paquete(aviso_kernel, conexion_kernel);

    eliminar_paquete(aviso_kernel);
}

void eliminar_segmento(int pid, int sid, bool mostrar){
    t_tabla_proceso* tabla = obtener_tabla_con_pid(pid, lista_tablas);
    t_segmento segmento_anterior, segmento_posterior;
    t_segmento* segmento_a_borrar = segmento_vacio;
    segmento_anterior.id = -1;
    segmento_posterior.id = -1;
    
    //Saco el segmento de la tabla de proceso
    int id_absoluto_seg = tabla->segmentos[sid]->id;
    tabla->segmentos[sid] = segmento_vacio;
    tabla->cant_segmentos_actuales--;

    //Saco el segmento de la lista de todos los segmentos
    for(int j = 0, k = list_size(lista_segmentos_actuales); j < k ; j++){
        segmento_a_borrar = list_get(lista_segmentos_actuales,j);
        if(segmento_a_borrar->id == id_absoluto_seg){
            list_remove(lista_segmentos_actuales,j);
            break;
        }
    }
    //Agregar hueco libre y consolidar de ser necesario
    for(int j = 0, k = list_size(huecos_libres); j < k ; j++){
        t_segmento seg = *(t_segmento*)list_get(huecos_libres,j);
        if(segmento_a_borrar->base == seg.limite){
            segmento_anterior = seg;
            segmento_anterior.id = j;
        }
        if(segmento_a_borrar->limite == seg.base){
            segmento_posterior = seg;
            segmento_posterior.id = j;
        }
        if(segmento_posterior.id > 0 && segmento_anterior.id > 0)break;
    }  
    t_segmento* hueco_nuevo = malloc(sizeof(t_segmento));
    if(segmento_anterior.id != -1){
        hueco_nuevo->base = segmento_anterior.base;
        t_segmento* h = list_get(huecos_libres,segmento_anterior.id);
        remover_hueco_libre(h,segmento_anterior.id);
    }else{
        hueco_nuevo->base = segmento_a_borrar->base;
    }
    if(segmento_posterior.id != -1){
        hueco_nuevo->limite = segmento_posterior.limite;
        // Se hace un -1 para contemplar el segmento 0 que no se elimina nunca
        t_segmento* h = list_get(huecos_libres, (segmento_posterior.id - 1));
        remover_hueco_libre(h,(segmento_posterior.id - 1));
    }else{
        hueco_nuevo->limite = segmento_a_borrar->limite;
    }
    if(mostrar){
        log_info(logger,"PID: %d - Eliminar Segmento: %d - Base: %d - TAMAÑO: %d", pid, sid, segmento_a_borrar->base,obtener_tamanio_segmento(segmento_a_borrar));
    }
    free(segmento_a_borrar);
    list_add(huecos_libres, hueco_nuevo);
}

void compactar_memoria(int conexion_kernel){
    int offset = 0;

    list_clean_and_destroy_elements(huecos_libres, free);
    for(int i = 0, j = list_size(lista_segmentos_actuales); i<j; i++){
        t_segmento* segmento = list_get(lista_segmentos_actuales,i);
        int tam_segmento = obtener_tamanio_segmento(segmento);
        void *valor = malloc(tam_segmento);
        memcpy(valor, memoria+segmento->base, tam_segmento);
        memcpy(memoria+offset, valor, tam_segmento);
        free(valor);

        segmento->base = offset;
        offset += tam_segmento;
        segmento->limite = offset;
    }
    memset(memoria+offset, 0, tam_memoria-offset);
    t_segmento* hueco_libre = malloc(sizeof(t_segmento));
    hueco_libre->base = offset;
    hueco_libre->limite = tam_memoria;
    list_add(huecos_libres, hueco_libre);

    usleep(retardo_compactacion*1000);
    for(int i = 0, j =  list_size(lista_tablas); i<j ; i++){
        t_tabla_proceso* tabla = list_get(lista_tablas,i);
       
        for(int k = 0; k < tabla->cant_segmentos_actuales; k++){
            log_info(logger, "PID: %d - Segmento: %d - Base: %d - Tamaño %d",tabla->pid, tabla->segmentos[k]->id, tabla->segmentos[k]->base, obtener_tamanio_segmento(tabla->segmentos[k]));
        }
        enviar_tabla_a_kernel(tabla, conexion_kernel);
    }
    t_paquete* paquete=crear_paquete_con_codigo_op(OK);
    enviar_paquete(paquete,conexion_kernel);
    eliminar_paquete(paquete);
}

void inicializar_lista_huecos_libres(){
    huecos_libres = list_create();
    t_segmento* hueco_libre_inicial = malloc(sizeof(t_segmento));
    hueco_libre_inicial->base = segmento_0->limite;
    hueco_libre_inicial->limite = tam_memoria;
    list_add(huecos_libres, hueco_libre_inicial);
}

int obtener_tamanio_segmento(t_segmento* segmento){
    return segmento->limite - segmento->base;
}

int tamanio_libre_disponible(){
    int tamanio_libre = 0;
    t_segmento* hueco_libre;
    for(int i=0, j=list_size(huecos_libres); i<j; i++){
        hueco_libre = list_get(huecos_libres, i);
        tamanio_libre += obtener_tamanio_segmento(hueco_libre);
    }
    return tamanio_libre;
}

t_segmento* modificar_lista_hueco_libre(t_segmento* hueco, uint32_t tamanioSegmento, int index) {
    t_segmento* segmento_nuevo = malloc(sizeof(t_segmento));
    memset(segmento_nuevo, 0, sizeof(t_segmento));
    
    segmento_nuevo->base = hueco->base;
    segmento_nuevo->limite = tamanioSegmento + segmento_nuevo->base;

    hueco->base = segmento_nuevo->limite;
    if(hueco->base == hueco->limite) remover_hueco_libre(hueco, index);
    return segmento_nuevo;
}

bool mayor_tamanio(void* param1, void* param2){
    t_segmento* segmento1 = (t_segmento*) param1;
    t_segmento* segmento2 = (t_segmento*) param2;
    
    int tamanio1 = obtener_tamanio_segmento(segmento1);
    int tamanio2 = obtener_tamanio_segmento(segmento2);
    
    if (tamanio1 > tamanio2) return true;
    
    return false;
}

bool menor_tamanio(void* param1, void* param2){
    return !(mayor_tamanio(param1,param2));
}

bool primera_aparicion(void* param1, void* param2){
    t_segmento* segmento1 = (t_segmento*) param1;
    t_segmento* segmento2 = (t_segmento*) param2;

    return (segmento1->base < segmento2->base);
}

bool algoritmo_asignacion_es(char* algoritmo){
    return strcmp(algorimo_asignacion, algoritmo) == 0;
}

t_segmento* buscar_hueco_libre(uint32_t tamanio_segmento){
    t_segmento* hueco_elegido;
    int i = 0;

    if(algoritmo_asignacion_es("WORST")){
        list_sort(huecos_libres, mayor_tamanio);
        hueco_elegido = list_get(huecos_libres,i);
        uint32_t tamanio_hueco = obtener_tamanio_segmento(hueco_elegido);
        if(tamanio_hueco >= tamanio_segmento) return modificar_lista_hueco_libre(hueco_elegido, tamanio_segmento, i); 
    }

    if(algoritmo_asignacion_es("BEST")) list_sort(huecos_libres, menor_tamanio);

    if(algoritmo_asignacion_es("FIRST")){
        list_sort(huecos_libres, primera_aparicion);
    }
    
    while(i < list_size(huecos_libres)){
        hueco_elegido = list_get(huecos_libres,i);
        uint32_t tamanio_hueco = obtener_tamanio_segmento(hueco_elegido);
        if(tamanio_hueco >= tamanio_segmento) return modificar_lista_hueco_libre(hueco_elegido, tamanio_segmento, i);
        i++;
    }
    //NO SE ENCONTRO HUECO, ENTONCES
    int tamanio_libre = tamanio_libre_disponible();
    //Se le asigna -1 para hacer compactacion ya que esta el tamanio libre, pero en distintos huecos
    //-2 Out of Memory
    hueco_elegido->id = (tamanio_libre >= tamanio_segmento) ? -1 : -2;
    return hueco_elegido;
}

void cargar_segmento_en_memoria(t_segmento* segmento){
    memcpy(memoria + segmento->base, &segmento, obtener_tamanio_segmento(segmento));
}

void print_memoria(){
    // Imprimir la memoria estéticamente
    printf("Contenido de la memoria:\n");
    for (int i = 0; i < tam_memoria; i++) {
        // Acceder a cada valor de la memoria
        uint32_t valor = *((uint32_t*)memoria + i);

        // Imprimir el valor en formato hexadecimal con ceros al principio
        printf("%08X ", valor);

        // Agregar un salto de línea después de 8 valores impresos
        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void iniciar_estructuras_memoria()
{
    log_info(logger, "TAM Memoria: %d", tam_memoria);
	log_info(logger, "TAM Segmento_0: %d", tam_segmento_0);
    
    lista_tablas = list_create();
    lista_segmentos_actuales = list_create();

	memoria = calloc(tam_memoria, sizeof(uint32_t));
	segmento_0 = malloc(sizeof(t_segmento));
    segmento_0->id = 0;
    segmento_0->base = 0;
    segmento_0->limite = tam_segmento_0;

    list_add(lista_segmentos_actuales, segmento_0);
    segmento_vacio = malloc(sizeof(t_segmento));
    segmento_vacio->id = -1;
    segmento_vacio->base = -1;
    segmento_vacio->limite = -1;

    memcpy(memoria, &segmento_0, tam_segmento_0);

    inicializar_lista_huecos_libres();
}
void remover_hueco_libre(t_segmento* hueco_libre, int indice){
    list_remove(huecos_libres, indice);
    free(hueco_libre);
}