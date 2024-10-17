#include <planificacion.h>

t_queue *new_queue;
t_queue *ready_queue;
t_pcb *pcb_a_ejecutar;
t_queue *colaBloqueados;
t_list *exit_list;

int proximo_pid=0;

pthread_mutex_t procesoMutex;
pthread_mutex_t procesosNuevosMutex;
pthread_mutex_t procesosListosMutex;
pthread_mutex_t procesosEjecutandoMutex;
pthread_mutex_t procesosBloqueadosMutex;
pthread_mutex_t procesosTerminadosMutex;
pthread_mutex_t mutex_recursos;
pthread_mutex_t mutexPID;

sem_t semProcesoListo;
sem_t semMultiprogramacion;
sem_t semProcesoNuevo;
void iniciar_listas_y_semaforos() {
    new_queue = queue_create();
    ready_queue = queue_create();
    pcb_a_ejecutar = NULL;

    exit_list = list_create();

    pthread_mutex_init(&procesoMutex, NULL);
    pthread_mutex_init(&procesosNuevosMutex, NULL);
    pthread_mutex_init(&procesosListosMutex, NULL);
    pthread_mutex_init(&procesosEjecutandoMutex, NULL);
    pthread_mutex_init(&procesosBloqueadosMutex, NULL);
    pthread_mutex_init(&procesosTerminadosMutex, NULL);
    pthread_mutex_init(&mutex_recursos,NULL);
	pthread_mutex_init(&mutexPID,NULL);

	//sem_init(&conexionMemoriaLista,0,0);
	//sem_init(&conexionFileSystemLista,0,0);
    sem_init(&semMultiprogramacion, 0, gradoMaxMultiprog);
    sem_init(&semProcesoNuevo, 0, 0);
    sem_init(&semProcesoListo, 0, 0);
}
t_pcb* crear_pcb(t_list* instructions, int conexion_consola){

	pthread_mutex_lock(&procesoMutex);
    t_pcb* pcb = malloc(sizeof(t_pcb));
	t_list* open_files = list_create();
	t_list* recursos_asignados = list_create();
	cpu_regs registers = {0};
	pthread_mutex_lock(&mutexPID);
    pcb->pid = proximo_pid;
	proximo_pid += 1;
	pthread_mutex_unlock(&mutexPID);
	pcb->conexion_consola = conexion_consola;
    pcb->instructions = instructions;
    pcb->program_counter = 0;
    pcb->cpu_registers = registers;
	pcb->espera_en_ready = NULL;
	pcb->rafaga_ejecucion_anterior = NULL;
    pcb->est_next_burst = estimacion_inicial;
    pcb->open_files = open_files;
	pcb->recursos_asignados = recursos_asignados;

    //ENVIAR MENSAJE A MEMORIA
    int conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);

    t_paquete *paquete_pid = crear_paquete_con_codigo_op(INICIALIZAR_ESTRUCTURAS);
    agregar_entero_a_paquete(paquete_pid, pcb->pid);
    
    enviar_paquete(paquete_pid, conexion_memoria);
    eliminar_paquete(paquete_pid);

    int cod_op = recibir_operacion(conexion_memoria);
    if(cod_op == TABLA_PROCESO){
        int size;
        void* buffer = recibir_buffer(&size,conexion_memoria);
        int offset=0;
        int cant;

        memcpy(&cant,buffer+offset,sizeof(int));
        offset+=sizeof(int);

        pcb->segment_tables = malloc(sizeof(t_tabla_proceso));
        pcb->segment_tables->segmentos = calloc(cant * sizeof(t_segmento), sizeof(uint32_t));
        memcpy(&pcb->segment_tables->cant_segmentos_actuales,buffer+offset,sizeof(int));
        offset+=sizeof(int);

        memcpy(&pcb->segment_tables->pid,buffer+offset,sizeof(int));
        offset+=sizeof(int);

        for(int i=0; i<cant; i++){
            memcpy(&pcb->segment_tables->segmentos[i].base_address,buffer+offset,sizeof(int));
            offset+=sizeof(int);
            memcpy(&pcb->segment_tables->segmentos[i].size,buffer+offset,sizeof(int));
            offset+=sizeof(int);
            memcpy(&pcb->segment_tables->segmentos[i].id,buffer+offset,sizeof(int));
            offset+=sizeof(int);
        }
        //print_tabla(pcb->segment_tables,cant);
        free(buffer);
    }
    
    close(conexion_memoria);

    pthread_mutex_unlock(&procesoMutex);

	return pcb;
}
void print_tabla(t_tabla_proceso* tabla, int cant_segmentos){
    printf("PID: %d\n", tabla->pid);
    printf("Cant segmentos actuales: %d\n", tabla->cant_segmentos_actuales);
    for(int i = 0; i<cant_segmentos;i++){
        if(tabla->segmentos[i].id== -1){
            printf("Segmento: %d, VACIO\n", i );
            continue;}
        printf("Segmento: %d, BASE: %d LIMITE: %d SID: %d\n", i, tabla->segmentos[i].base_address,tabla->segmentos[i].size, tabla->segmentos[i].id);
    }
}
t_pcb* buscar_pid(int pid){
    t_pcb* pcb;
    for(int i = 0; i< list_size(ready_queue->elements);i++){
        pcb =list_get(ready_queue->elements,i);
        if(pcb->pid == pid){
            return pcb;
        }
    }
    if(pcb_a_ejecutar->pid == pid){
        pcb = pcb_a_ejecutar;
    }
    
    return pcb;
}

int verificar_finalizacion(int pid){
    int pid_terminado;
    for(int i = 0; i< list_size(exit_list);i++){
        pid_terminado = *(int*)list_get(exit_list,i);
        if(pid_terminado == pid){
            return 1;
        }
    }
    return -1;
}

void actualizar_pcb_ejecucion(t_pcb* pcb){
    pthread_mutex_lock(&procesoMutex);
    pcb_a_ejecutar->open_files = list_create();
    for(int i= 0; i<list_size(pcb->open_files);i++){
		table_open_files* valor = list_get(pcb->open_files,i);
		table_open_files* nuevo = malloc(sizeof(table_open_files));
        nuevo->file_ptr = malloc(strlen(valor->file_ptr)+1);
        strcpy(nuevo->file_ptr,valor->file_ptr);
        nuevo->pointer_pos = valor->pointer_pos;
		list_add(pcb_a_ejecutar->open_files,nuevo);
	}
    pthread_mutex_unlock(&procesoMutex);
}

void estimacion_next_burst(t_pcb* pcb){
	pcb->est_next_burst = hrrn_alfa * pcb->est_next_burst + (1 - hrrn_alfa) * temporal_gettime(pcb->rafaga_ejecucion_anterior);
}

void actualizar_archivos(t_pcb* pcbActualizado,t_pcb* pcbEnEjecucion){
    pthread_mutex_lock(&procesoMutex);

    pcbActualizado->open_files = list_create();
	for(int i= 0; i<list_size(pcbEnEjecucion->open_files);i++){
		table_open_files* valor = list_get(pcbEnEjecucion->open_files,i);
		table_open_files* nuevo = malloc(sizeof(table_open_files));
        nuevo->file_ptr = malloc(strlen(valor->file_ptr)+1);
        strcpy(nuevo->file_ptr,valor->file_ptr);
        nuevo->pointer_pos = valor->pointer_pos;
		list_add(pcbActualizado->open_files,nuevo);
	}

    pthread_mutex_unlock(&procesoMutex);
}

void actualizar_archivos_seguir_exec(t_pcb* pcbActualizado){
    pthread_mutex_lock(&procesoMutex);
    pcbActualizado->open_files = list_create();
    for(int i= 0; i<list_size(pcb_a_ejecutar->open_files);i++){
		table_open_files* valor = list_get(pcb_a_ejecutar->open_files,i);
		table_open_files* nuevo = malloc(sizeof(table_open_files));
        nuevo->file_ptr = malloc(strlen(valor->file_ptr)+1);
        strcpy(nuevo->file_ptr,valor->file_ptr);
        nuevo->pointer_pos = valor->pointer_pos;
		list_add(pcbActualizado->open_files,nuevo);
	}

    pthread_mutex_unlock(&procesoMutex);
}

void actualizar_rafaga_ejecucion_del_pcb(t_pcb* pcbActualizado, t_pcb* pcbEnEjecucion) {
    
    pthread_mutex_lock(&procesoMutex);
    
    temporal_stop(pcbEnEjecucion->rafaga_ejecucion_anterior);
    pcbActualizado->rafaga_ejecucion_anterior = pcbEnEjecucion->rafaga_ejecucion_anterior;

    pcbActualizado->est_next_burst = pcbEnEjecucion->est_next_burst;
    pthread_mutex_unlock(&procesoMutex);
}
void actualizar_conexion_consola(t_pcb* pcbActualizado, t_pcb* pcbEnEjecucion){
    pthread_mutex_lock(&procesoMutex);

    pcbActualizado->conexion_consola = pcbEnEjecucion->conexion_consola;

    pthread_mutex_unlock(&procesoMutex);
}

void actualizar_proxima_rafaga_estimada_del_pcb(t_pcb* pcbActualizado) {
    
    pthread_mutex_lock(&procesoMutex);

    estimacion_next_burst(pcbActualizado);

    pthread_mutex_unlock(&procesoMutex);
}


/* --------------- Funciones Procesos Nuevos --------------- */

void agregar_pcb_cola_nuevos(t_pcb* proceso) {

    pthread_mutex_lock(&procesosNuevosMutex);

    queue_push(new_queue, proceso);

    log_info(logger,"Se crea el proceso %d en NEW",proceso->pid);

    pthread_mutex_unlock(&procesosNuevosMutex);

    sem_post(&semProcesoNuevo);
}

t_pcb* sacar_pcb_de_cola_nuevo() {

    pthread_mutex_lock(&procesosNuevosMutex);

    t_pcb* proceso = queue_pop(new_queue);

    pthread_mutex_unlock(&procesosNuevosMutex);

  return proceso;
}

/* --------------- Funciones Procesos Listos --------------- */

void agregar_pcb_en_cola_listos(t_pcb* proceso) 
{
    pthread_mutex_lock(&procesosListosMutex);

    queue_push(ready_queue, proceso);

    pthread_mutex_unlock(&procesosListosMutex);
    
    if(algoritmo_es_hrrn(algoritmo_planificacion))empezar_espera_ready(proceso);

    mostrar_cola_ready();

    sem_post(&semProcesoListo);
}
void empezar_espera_ready(t_pcb* pcb){
    pthread_mutex_lock(&procesoMutex);
    pcb->espera_en_ready = temporal_create();
    pthread_mutex_unlock(&procesoMutex);
}
t_pcb* sacar_pcb_cola_listos() 
{

    pthread_mutex_lock(&procesosListosMutex);

    t_pcb* proceso = queue_pop(ready_queue);

    pthread_mutex_unlock(&procesosListosMutex);

  return proceso;
}
void mostrar_cola_ready(){
	char* string = string_new();
	string_append(&string, "[");    

	for(int i=0; i<queue_size(ready_queue);i++){
		t_pcb* e = list_get(ready_queue->elements,i);
		char* pid_string = string_itoa(e->pid);
        string_append(&string, pid_string);
		if(i!=queue_size(ready_queue)-1)string_append(&string, ",");
		free(pid_string);
	}

	string_append(&string, "]");
	log_info(logger,"Cola Ready %s: %s\n", algoritmo_planificacion,string);
	free(string);
}
/* --------------- Funciones Procesos en Ejecución --------------- */
void pcb_terminar_espera_ready(t_pcb* pcb){
    temporal_destroy(pcb->espera_en_ready);
}
bool algoritmo_es_hrrn(){
	return (strcmp(algoritmo_planificacion,"HRRN") == 0);
}
void meter_pcb_en_ejecucion(t_pcb* proceso) 
{
    pthread_mutex_lock(&procesosEjecutandoMutex);
    
    pcb_a_ejecutar = proceso;

    if(algoritmo_es_hrrn()){
        pcb_iniciar_rafaga_ejecucion(pcb_a_ejecutar);
        pcb_terminar_espera_ready(pcb_a_ejecutar);
    }

    pthread_mutex_unlock(&procesosEjecutandoMutex);
}

t_pcb* sacar_pcb_de_ejecucion() 
{
    pthread_mutex_lock(&procesosEjecutandoMutex);

    t_pcb* proceso = pcb_a_ejecutar;
    
    pcb_a_ejecutar = NULL;

    pthread_mutex_unlock(&procesosEjecutandoMutex);

  return proceso;
}


/* --------------- Funciones Procesos Bloqueados --------------- */




/*
t_pcb* sacar_pcb_de_cola_bloqueado() 
{

    pthread_mutex_lock(&procesosBloqueadosMutex);

    t_pcb* proceso = list_remove(colaBloqueados, 0);

    pthread_mutex_unlock(&procesosBloqueadosMutex);

  return proceso;
}
*/



/* --------------- Funciones Procesos Terminados --------------- */

void agregar_pcb_en_cola_terminados(int* pid) 
{
    pthread_mutex_lock(&procesosTerminadosMutex);

    list_add(exit_list, pid);

    pthread_mutex_unlock(&procesosTerminadosMutex);
}


/* --------------- Funciones Generales --------------- */


bool mayor_RR(t_pcb* proceso_a, t_pcb* proceso_b){      
	double ratio_a = calcularRR(proceso_a);     
	double ratio_b = calcularRR(proceso_b);
	if(ratio_a == ratio_b)
	{
		return temporal_gettime(proceso_a->espera_en_ready) > temporal_gettime(proceso_b->espera_en_ready);
	}
	return ratio_a > ratio_b; 
}  

double calcularRR(t_pcb* pcb) {
	return 1 + (temporal_gettime(pcb->espera_en_ready)/ pcb->est_next_burst); 
}

void ordenar_cola_listos() {

    pthread_mutex_lock(&procesosListosMutex);
   
    list_sort(ready_queue->elements,(void*) mayor_RR);
    
    pthread_mutex_unlock(&procesosListosMutex);
}



void pcb_iniciar_rafaga_ejecucion(t_pcb *pcb) 
{
	pcb->rafaga_ejecucion_anterior = temporal_create();
}

/*
void liberar_variables(){
	//Liberar semaforos
	sem_destroy(&hayPcb);
	sem_destroy(&long_term);
	sem_destroy(&hayProcesoReady);
	sem_destroy(&short_term);
	sem_destroy(&contadorMaxMultiprogramacion);
	sem_destroy(&conexionCPULista);
	sem_destroy(&conexionMemoriaLista);
	sem_destroy(&conexionFileSystemLista);
	pthread_mutex_destroy(&mutexNew);
    pthread_mutex_destroy(&mutexReady);
	pthread_mutex_destroy(&mutex_recursos);
	pthread_mutex_destroy(&mutex_pcb);
	pthread_mutex_destroy(&mutex_esperar_clientes);
	//Liberar 
	free(estimacion_inicial);
	free(algoritmo_planificacion);
	//Liberar listas
	list_destroy_and_destroy_elements(exit_list, liberar_pcb);
	list_destroy_and_destroy_elements(recursos_disponibles, liberar_pcb);
	queue_destroy_and_destroy_elements(new_queue, liberar_pcb);
	queue_destroy_and_destroy_elements(ready_queue, liberar_pcb);
}*/