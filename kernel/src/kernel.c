#include "kernel.h"
t_log* logger;
t_config* config;
t_pcb* proceso_en_ejecucion;
t_queue** colasRecursos;
t_list* colasArchivos;
t_list* recursos_disponibles;
t_list* tabla_archivos_globales;

pthread_mutex_t mutexTablaGlobal;
sem_t semFS;

char* ip;
char* puerto_escucha;
char* ip_cpu;
char* puerto_cpu;
char* ip_filesystem;
char* puerto_filesystem;
char* ip_memoria;
char* puerto_memoria;
double estimacion_inicial;
double hrrn_alfa;
char* algoritmo_planificacion;
int gradoMaxMultiprog;

void long_term_scheduler() {
    
	while (true){
        sem_wait(&semProcesoNuevo);
		sem_wait(&semMultiprogramacion);
		t_pcb* pcb = sacar_pcb_de_cola_nuevo();

		agregar_pcb_en_cola_listos(pcb);

		log_info(logger,"PID: %d - Estado Anterior: NEW - Estado Actual: READY\n", pcb->pid);
    }
}

void short_term_scheduler() {
    
	int conexion_cpu = crear_conexion(ip_cpu, puerto_cpu);
	while (true) {
		sem_wait(&semProcesoListo);

		if(algoritmo_es_hrrn()){
			ordenar_cola_listos();
		}

		t_pcb* pcb = sacar_pcb_cola_listos();

		if(algoritmo_es_hrrn()){
			temporal_destroy(pcb->rafaga_ejecucion_anterior);
		}
		meter_pcb_en_ejecucion(pcb);

		// Ejecutar proceso
		log_info(logger,"PID: %d - Estado Anterior: READY - Estado Actual: EXEC\n", pcb->pid);
		
		enviar_pcb(pcb, conexion_cpu);

		recibir_pcb_de_CPU(conexion_cpu);
    }
	close(conexion_cpu);
	
}

int buscar_en_colas(int pid){
	for(int i=0;i<list_size(colasArchivos);i++){
		t_queue* cola = list_get(colasArchivos,i);
		if(!queue_is_empty(cola)){
			for(int y=0; y<queue_size(cola);y++){
				t_pcb* pcb = list_get(cola->elements,y);
				if(pcb->pid == pid){
					return 1;
				}
			}
		}
	}
	return -1;
}

void printear_archivos(t_list* archivos){
	for(int i = 0; i<list_size(archivos);i++){
		table_open_files* file=list_get(archivos,i);
		log_info(logger,"NOMBRE: %s ", file->file_ptr);
		log_info(logger," PUNTERO: %d \n",file->pointer_pos);
	}
}

void recibir_pcb_de_CPU(int conexion) {
    int op_code = recibir_operacion(conexion);
    t_pcb *pcb_en_ejecucion, *pcb_actualizado;
    switch (op_code){
			case PAQUETE_PCB:
			 	pcb_en_ejecucion = sacar_pcb_de_ejecucion();

				pcb_actualizado = recibir_pcb(conexion);
				
				actualizar_conexion_consola(pcb_actualizado,pcb_en_ejecucion);
				actualizar_archivos(pcb_actualizado,pcb_en_ejecucion);

				if(algoritmo_es_hrrn()){
					actualizar_rafaga_ejecucion_del_pcb(pcb_actualizado, pcb_en_ejecucion);
					actualizar_proxima_rafaga_estimada_del_pcb(pcb_actualizado);
				}
				
				liberar_pcb(pcb_en_ejecucion);
				analizarMotivo(pcb_actualizado,conexion);
				break;
			case SEG_FAULT:
			 	pcb_en_ejecucion = sacar_pcb_de_ejecucion();
				pcb_actualizado = recibir_pcb(conexion);
				actualizar_conexion_consola(pcb_actualizado,pcb_en_ejecucion);
				actualizar_archivos(pcb_actualizado,pcb_en_ejecucion);
				
				log_info(logger,"Finaliza el proceso %d - Motivo: SEG_FAULT \n", pcb_actualizado->pid);
				liberar_pcb(pcb_en_ejecucion);
				doExit(pcb_actualizado);
				break;
			case CREAR_SEGMENTO:
				pcb_actualizado = recibir_pcb(conexion);
				actualizar_archivos_seguir_exec(pcb_actualizado);
				int pid = pcb_actualizado->pid;
				analizarMotivo(pcb_actualizado,conexion);
				if(verificar_finalizacion(pid)==1){
					break;
				}
				enviar_pcb_cpu(pcb_actualizado, conexion);
				liberar_pcb(pcb_actualizado);
				recibir_pcb_de_CPU(conexion);
				break;
			case BORRAR_SEGMENT:
				pcb_actualizado = recibir_pcb(conexion);
				actualizar_archivos_seguir_exec(pcb_actualizado);
				analizarMotivo(pcb_actualizado,conexion);
				enviar_pcb_cpu(pcb_actualizado, conexion);
				liberar_pcb(pcb_actualizado);
				recibir_pcb_de_CPU(conexion);
				break;
			case CREAR_ARCHIVO:
				pcb_actualizado = recibir_pcb(conexion);
				actualizar_archivos_seguir_exec(pcb_actualizado);
				analizarMotivo(pcb_actualizado,conexion);
				if(buscar_en_colas(pcb_actualizado->pid)==1){
					pcb_en_ejecucion = sacar_pcb_de_ejecucion();
					actualizar_conexion_consola(pcb_actualizado,pcb_en_ejecucion);
					break;
				}
				actualizar_pcb_ejecucion(pcb_actualizado);
				enviar_pcb_cpu(pcb_actualizado, conexion);
				liberar_pcb(pcb_actualizado);
				recibir_pcb_de_CPU(conexion);
				break;
			case F_SEEK:
				pcb_actualizado = recibir_pcb(conexion);
				actualizar_archivos_seguir_exec(pcb_actualizado);
				analizarMotivo(pcb_actualizado,conexion);
				actualizar_pcb_ejecucion(pcb_actualizado);
				enviar_pcb_cpu(pcb_actualizado, conexion);
				liberar_pcb(pcb_actualizado);
				recibir_pcb_de_CPU(conexion);
				break;
			default:
				log_info(logger,"Operacion desconocida. No quieras meter la pata");
				break;
			
	}
}
void doSignal(t_pcb* process, char* recurso){
	int i = encontrarIndiceRecursoEnLista(recursos_disponibles,recurso);

	if(i == -1){
		log_info(logger,"Finaliza el proceso %d - Motivo: INVALID_RESOURCE \n", process->pid);
		doExit(process);
		return;
	}else{
		t_recurso* elem = list_get(recursos_disponibles, i);

		int j = encontrarIndiceRecursoEnLista(process->recursos_asignados,recurso);

		if(!(j != -1)){
			elem->cantRecurso+=1;
			log_info(logger,"PID: %d - Signal: %s - Instancias: 1\n", process->pid,recurso);
			log_info(logger,"PID: %d - Estado Anterior: EXEC- Estado Actual: READY\n", process->pid);
		}else{
			t_recurso* e = list_get(process->recursos_asignados, j);
			if(e->cantRecurso<=0){
				//El proceso no tiene instancias para liberar
			}else{
				//pthread_mutex_lock(&mutex_pcb);
				e->cantRecurso-=1;
				//pthread_mutex_unlock(&mutex_pcb);

				log_info(logger,"PID: %d - Signal: %s - Instancias: %d\n", process->pid,recurso,e->cantRecurso);
				log_info(logger,"PID: %d - Estado Anterior: EXEC- Estado Actual: READY\n", process->pid);
			}
		}
		if(!queue_is_empty(colasRecursos[i])){
			t_pcb* waitingProcess =  queue_pop(colasRecursos[i]);
			int x = encontrarIndiceRecursoEnLista(waitingProcess->recursos_asignados,recurso);

			if(x != -1){
				t_recurso* recursoAAsignar = list_get(waitingProcess->recursos_asignados,x);
				recursoAAsignar->cantRecurso += 1;
			}else{
				t_recurso* recursoAAsignar = malloc(sizeof(t_recurso));
				recursoAAsignar->recurso = strdup(recurso);
				recursoAAsignar->cantRecurso = 1;
				list_add(waitingProcess->recursos_asignados,recursoAAsignar);
			}

			log_info(logger,"PID: %d - Estado Anterior: BLOCK- Estado Actual: READY\n", waitingProcess->pid);
			agregar_pcb_en_cola_listos(waitingProcess);
		}else{
			//pthread_mutex_lock(&mutex_recursos);
			elem->cantRecurso += 1;
			//pthread_mutex_unlock(&mutex_recursos);
		}
		agregar_pcb_en_cola_listos(process);
		
	}
}

void bloquear_pcb(t_args* args){
	log_info(logger,"PID: %d - Bloqueado por: IO\n", args->pcb->pid);
	log_info(logger,"PID: %d - Ejecuta IO: %d\n", args->pcb->pid, args->tiempoBloqueo);

	sleep(args->tiempoBloqueo);

	log_info(logger,"PID: %d - Estado Anterior: BLOCK - Estado Actual: READY\n", args->pcb->pid);
	
	agregar_pcb_en_cola_listos(args->pcb);
	
	free(args);
}

void doIO(t_pcb* process, int tiempoBloqueo){
	log_info(logger,"PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCK\n", process->pid);
	
	t_args* args = malloc(sizeof(t_args));
	args->pcb=process;
	args->tiempoBloqueo=tiempoBloqueo;
	pthread_t bloqueo;
	pthread_create(&bloqueo, NULL, (void*) bloquear_pcb, (void*) args);
	pthread_detach(bloqueo);
}

void doYield(t_pcb* process){
	log_info(logger,"PID: %d - Yield \n", process->pid);
	
	log_info(logger,"PID: %d - Estado Anterior: EXEC- Estado Actual: READY\n", process->pid);
	
	agregar_pcb_en_cola_listos(process);
}

void doWait(t_pcb* process, char* recurso){
	int i = encontrarIndiceRecursoEnLista(recursos_disponibles,recurso);
	if(i == -1){
		log_info(logger,"Finaliza el proceso %d - Motivo: INVALID_RESOURCE \n", process->pid);	
		doExit(process);
	}else{

		t_recurso* elem = list_get(recursos_disponibles, i);
		
		if(elem->cantRecurso>0){
			//pthread_mutex_lock(&mutex_recursos);
			elem->cantRecurso -= 1;
			int j = encontrarIndiceRecursoEnLista(process->recursos_asignados,recurso);
			//pthread_mutex_unlock(&mutex_recursos);

			if(!(j != -1)){
				t_recurso* r = malloc(sizeof(t_recurso));
				r->recurso = strdup(recurso);
				r->cantRecurso = 1;
				//pthread_mutex_lock(&mutex_pcb);
				list_add(process->recursos_asignados,r);
				//pthread_mutex_unlock(&mutex_pcb);
				log_info(logger,"PID: %d - Wait: %s - Instancias: %d\n", process->pid,recurso,r->cantRecurso);
			}else{
				t_recurso* e = list_get(process->recursos_asignados, j);
				//pthread_mutex_lock(&mutex_pcb);
				e->cantRecurso+=1;
				//pthread_mutex_unlock(&mutex_pcb);
				log_info(logger,"PID: %d - Wait: %s - Instancias: %d\n", process->pid,recurso,e->cantRecurso);
			}
			
			log_info(logger,"PID: %d - Estado Anterior: EXEC- Estado Actual: READY\n", process->pid);

			agregar_pcb_en_cola_listos(process);
		}else{
			//LOGICA ESPERAR EN LA COLA DEL RECURSO HASTA QUE SE DESOCUPE
			log_info(logger,"PID: %d - Estado Anterior: EXEC- Estado Actual: BLOCK\n", process->pid);
			log_info(logger,"PID: %d - Bloqueado por: %s\n", process->pid, recurso);
			queue_push(colasRecursos[i],process);
		}
	}	
}

int indice_archivos(char* file_ptr, t_list* lista){
	for(int i = 0; i<list_size(lista);i++){
		table_open_files* file = list_get(lista,i);
		if(strcmp(file->file_ptr, file_ptr)==0)
		{
			return i;
		}
	}
	return -1;
}

void do_f_open(char* file_ptr, t_pcb* process){
	
	int indice = 0;
	table_open_files* file = malloc(sizeof(table_open_files));
	file->file_ptr = malloc(strlen(file_ptr)+1);
	strcpy(file->file_ptr,file_ptr);
	file->pointer_pos = 0;

	log_info(logger,"PID: %d - Abrir Archivo: %s",process->pid,file_ptr);

	pthread_mutex_lock(&mutexTablaGlobal);

	indice = indice_archivos(file_ptr,tabla_archivos_globales);

	if(indice!= -1){

		t_queue* cola = list_get(colasArchivos,indice);
		queue_push(cola,process);

		log_info(logger,"PID: %d - Bloqueado por: %s",process->pid,file_ptr);

	}else{
		int conexion_fileSystem = crear_conexion(ip_filesystem, puerto_filesystem);

		t_paquete* paquete = crear_paquete_con_codigo_op(ABRIR_ARCHIVO);
		agregar_a_paquete(paquete,file_ptr,strlen(file_ptr));
		enviar_paquete(paquete,conexion_fileSystem);
		eliminar_paquete(paquete);

		int cod_op = recibir_operacion(conexion_fileSystem);

		if(cod_op != OK){
			t_paquete* paquete = crear_paquete_con_codigo_op(CREAR_ARCHIVO);
			agregar_a_paquete(paquete,file_ptr,strlen(file_ptr));
			agregar_entero_a_paquete(paquete,0);

			enviar_paquete(paquete,conexion_fileSystem);
			eliminar_paquete(paquete);

			cod_op = recibir_operacion(conexion_fileSystem);
		}
		
		table_open_files* file_global = malloc(sizeof(table_open_files));
		file_global->file_ptr = malloc(strlen(file_ptr)+1);
		strcpy(file_global->file_ptr,file_ptr);
		file_global->pointer_pos = 0;
		list_add(tabla_archivos_globales,file_global);

		indice = indice_archivos(file_ptr,tabla_archivos_globales);

		t_queue* cola = queue_create();
		list_add(colasArchivos,cola);

		close(conexion_fileSystem);
	}

	pthread_mutex_unlock(&mutexTablaGlobal);

	list_add(process->open_files,file);
}

void do_f_close(char* file_ptr, t_pcb* process){
	int indice_proceso = indice_archivos(file_ptr,process->open_files);
	
	pthread_mutex_lock(&mutexTablaGlobal);

	int indice = indice_archivos(file_ptr,tabla_archivos_globales);

	log_info(logger,"PID: %d - Cerrar Archivo: %s",process->pid,file_ptr);
	t_queue* cola = list_get(colasArchivos,indice);
	if(queue_is_empty(cola))
	{
		list_remove(tabla_archivos_globales,indice);
		list_remove(colasArchivos,indice);
		queue_destroy(cola);
	}else{
		t_pcb* pcb = queue_pop(cola);
		agregar_pcb_en_cola_listos(pcb);
		log_info(logger,"PID: %d - Estado Anterior: BLOCK- Estado Actual: READY\n", pcb->pid);
	}

	pthread_mutex_unlock(&mutexTablaGlobal);

	list_remove_and_destroy_element(process->open_files,indice_proceso, liberar_archivo);
	agregar_pcb_en_cola_listos(process);
	log_info(logger,"PID: %d - Estado Anterior: EXEC- Estado Actual: READY\n", process->pid);
}

void do_f_seek(int ptr_pos, t_pcb* process, char* file_ptr){

	int indice = indice_archivos(file_ptr,process->open_files);

	if(indice != -1){
		table_open_files* file = list_get(process->open_files, indice);
		file->pointer_pos = ptr_pos;
		log_info(logger,"PID: %d - Actualizar puntero Archivo: %s - Puntero %d",process->pid,file_ptr,file->pointer_pos);
	}
}

void bloquear_pcb_fs_r_w(t_args* args){
	recibir_operacion(args->tiempoBloqueo);

	agregar_pcb_en_cola_listos(args->pcb);
	close(args->tiempoBloqueo);
	log_info(logger,"PID: %d - Estado Anterior: BLOCK- Estado Actual: READY\n", args->pcb->pid);
	sem_post(&semFS);
	free(args);
}

void do_f_write(t_pcb* process,char* file ,int direccion_fisica, int cant_bytes){
	
	int indice = indice_archivos(file,process->open_files);
	table_open_files* file_open =list_get(process->open_files, indice);

	log_info(logger,"PID: %d - Escribir Archivo: %s - Puntero %d - Dirección Memoria %d - Tamaño %d",process->pid,file,file_open->pointer_pos,direccion_fisica,cant_bytes);
	
	sem_wait(&semFS);
	int conexion_fileSystem = crear_conexion(ip_filesystem, puerto_filesystem);

	t_paquete* paquete = crear_paquete_con_codigo_op(ESCRIBIR_ARCHIVO);
	agregar_a_paquete(paquete,file,strlen(file));
	agregar_entero_a_paquete(paquete,file_open->pointer_pos);
	agregar_entero_a_paquete(paquete,cant_bytes);
	agregar_entero_a_paquete(paquete,direccion_fisica);
	agregar_entero_a_paquete(paquete,process->pid);

	enviar_paquete(paquete,conexion_fileSystem);
	eliminar_paquete(paquete);

	log_info(logger,"PID: %d - Estado Anterior: EXEC- Estado Actual: BLOCK\n", process->pid);
	log_info(logger,"PID: %d - Bloqueado por: %s",process->pid,file);
	//file_open->pointer_pos += cant_bytes;

	t_args* args = malloc(sizeof(t_args));
	args->pcb=process;
	args->tiempoBloqueo=conexion_fileSystem;

	pthread_t bloqueo;
	pthread_create(&bloqueo, NULL, (void*) bloquear_pcb_fs_r_w, (void*) args);
	pthread_detach(bloqueo);

}

void do_f_read(t_pcb* process,char* file ,int direccion_fisica, int cant_bytes){

	int indice = indice_archivos(file,process->open_files);
	table_open_files* file_open =list_get(process->open_files, indice);

	log_info(logger,"PID: %d - Leer Archivo: %s - Puntero %d - Dirección Memoria %d - Tamaño %d",process->pid,file,file_open->pointer_pos,direccion_fisica,cant_bytes);
	sem_wait(&semFS);

	int conexion_fileSystem = crear_conexion(ip_filesystem, puerto_filesystem);

	t_paquete* paquete = crear_paquete_con_codigo_op(LEER_ARCHIVO);
	agregar_a_paquete(paquete,file,strlen(file));
	agregar_entero_a_paquete(paquete,file_open->pointer_pos);
	agregar_entero_a_paquete(paquete,cant_bytes);
	agregar_entero_a_paquete(paquete,direccion_fisica);
	agregar_entero_a_paquete(paquete,process->pid);

	enviar_paquete(paquete,conexion_fileSystem);
	eliminar_paquete(paquete);

	log_info(logger,"PID: %d - Estado Anterior: EXEC- Estado Actual: BLOCK\n", process->pid);
	log_info(logger,"PID: %d - Bloqueado por: %s",process->pid,file);
	//file_open->pointer_pos += cant_bytes;

	t_args* args = malloc(sizeof(t_args));
	args->pcb=process;
	args->tiempoBloqueo=conexion_fileSystem;

	pthread_t bloqueo;
	pthread_create(&bloqueo, NULL, (void*) bloquear_pcb_fs_r_w, (void*) args);
	pthread_detach(bloqueo);
	
}

void bloquear_pcb_fs(t_args* args){
	recibir_operacion(args->tiempoBloqueo);

	agregar_pcb_en_cola_listos(args->pcb);
	close(args->tiempoBloqueo);
	log_info(logger,"PID: %d - Estado Anterior: BLOCK- Estado Actual: READY\n", args->pcb->pid);
	free(args);
}

void do_f_truncate(t_pcb* process,char* file, int tam){
	int conexion_fileSystem = crear_conexion(ip_filesystem, puerto_filesystem);

	log_info(logger,"PID: %d - Archivo: %s - Tamaño: %d",process->pid,file,tam);

	t_paquete* paquete = crear_paquete_con_codigo_op(TRUNCAR_ARCHIVO);
	agregar_a_paquete(paquete,file,strlen(file));
	agregar_entero_a_paquete(paquete,tam);

	enviar_paquete(paquete,conexion_fileSystem);
	eliminar_paquete(paquete);

	log_info(logger,"PID: %d - Estado Anterior: EXEC- Estado Actual: BLOCK\n", process->pid);
	log_info(logger,"PID: %d - Bloqueado por: %s",process->pid,file);

	t_args* args = malloc(sizeof(t_args));
	args->pcb=process;
	args->tiempoBloqueo=conexion_fileSystem;

	pthread_t bloqueo;
	pthread_create(&bloqueo, NULL, (void*) bloquear_pcb_fs, (void*) args);
	pthread_detach(bloqueo);
	
}

void do_create_segment(t_pcb* process, int tamanio,int id){
	int conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
	t_paquete* segment_process = crear_paquete_con_codigo_op(CREAR_SEGMENTO);
	agregar_entero_a_paquete(segment_process,process->pid);
	agregar_entero_a_paquete(segment_process,tamanio);
	agregar_entero_a_paquete(segment_process,id);
	enviar_paquete(segment_process,conexion_memoria);
    eliminar_paquete(segment_process);

	log_info(logger,"PID: %d - Crear Segmento - Id: %d - Tamaño: %d", process->pid,id,tamanio);

	int cod_op = recibir_operacion(conexion_memoria);
	switch (cod_op){
		case OUT_OF_MEMORY:
			log_info(logger,"Finaliza el proceso %d - Motivo: OUT OF MEMORY\n", process->pid);
			t_pcb* pcb_en_ejecucion = sacar_pcb_de_ejecucion();
			actualizar_conexion_consola(process,pcb_en_ejecucion);
			close(conexion_memoria);
			if(algoritmo_es_hrrn())temporal_destroy(pcb_en_ejecucion->rafaga_ejecucion_anterior);
			liberar_pcb(pcb_en_ejecucion);
			doExit(process);
			break;
		case SEGMENTO_CREADO:
			int direccion_base=recibir_direccion_base(conexion_memoria);

			t_segmento* seg = malloc(sizeof(t_segmento));
			seg->base_address = direccion_base;
			seg->id = id;
			seg->size = direccion_base + tamanio;

			process->segment_tables->cant_segmentos_actuales+=1;
			process->segment_tables->segmentos[id]= *seg;
			free(seg);
			close(conexion_memoria);
			break;
		case COMPACTAR:
			log_info(logger,"Compactación: Esperando Fin de Operaciones de FS");
			sem_wait(&semFS);
			
			log_info(logger,"Compactación: Se solicitó compactación");

			t_paquete* compactacion = crear_paquete_con_codigo_op(COMPACTAR);
			enviar_paquete(compactacion,conexion_memoria);
			eliminar_paquete(compactacion);

		    // Atrapar el codigo de operacion que llega en cero antes que el codigo de operacion real
			recibir_operacion(conexion_memoria);
			t_list* tablas_segmentos = recibir_tabla_segmentos(conexion_memoria);

			for(int i = 0; i<list_size(tablas_segmentos);i++){
				t_tabla_proceso* tabla = list_get(tablas_segmentos,i);
				t_pcb* pcb = buscar_pid(tabla->pid);

				// Libero las segment tables, para que no se pise el espacio de memoria con la tabla nueva
				// Solo sirve en el caso de la compactacion
				free(pcb->segment_tables->segmentos);
				free(pcb->segment_tables);
				pcb->segment_tables = tabla;
			}

			list_destroy(tablas_segmentos);

			log_info(logger,"Se finalizó el proceso de compactación");

			do_create_segment(process,tamanio,id);

			sem_post(&semFS);
			break;
		default:
			close(conexion_memoria);
			break;
	}
}

void do_delete_segment(t_pcb* process, int id){

	log_info(logger,"PID: %d - Eliminar Segmento - Id Segmento: %d", process->pid,id);

	int conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
	t_paquete* paquete = crear_paquete_con_codigo_op(BORRAR_SEGMENT);
	agregar_entero_a_paquete(paquete,process->pid);
	agregar_entero_a_paquete(paquete,id);
	enviar_paquete(paquete,conexion_memoria);
    eliminar_paquete(paquete);
 	
	if(recibir_operacion(conexion_memoria)!=OK){
		return;
	}

	close(conexion_memoria);

	t_segmento* seg = malloc(sizeof(t_segmento));
	seg->base_address = 0;
	seg->id = -1;
	seg->size = 0;

	process->segment_tables->segmentos[id] = *seg;
	free(seg);
	//print_tabla(process->segment_tables,process->segment_tables->cant_segmentos_actuales);
}


void doExit(t_pcb* process){
	//pasar al mismo al estado EXIT
	log_info(logger,"PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT\n", process->pid);
	int* pid = malloc(sizeof(int));
	*pid = process->pid;

	//liberar todos los recursos que tenga asignados
	//process->espera_en_ready = temporal_create();
	//process->rafaga_ejecucion_anterior = temporal_create();
	liberarRecursosAsignados(recursos_disponibles, process->recursos_asignados);

	agregar_pcb_en_cola_terminados(pid);
	
	sem_post(&semMultiprogramacion);  
	
	//dar aviso a consola y a módulo Memoria para que éste libere sus estructuras
	
	t_paquete* fin = crear_paquete_con_codigo_op(FIN_PROCESO);
	agregar_entero_a_paquete(fin, process->pid);
	enviar_paquete(fin,process->conexion_consola);
	int conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
	enviar_paquete(fin,conexion_memoria);
	eliminar_paquete(fin);
	close(conexion_memoria);
	if(algoritmo_es_hrrn()) temporal_destroy(process->rafaga_ejecucion_anterior);
	liberar_pcb(process);
}

void analizarMotivo(t_pcb* process,int conexion_cpu){
	instruction_set* i = list_get(process->instructions,process->program_counter - 1);
	char* motivoDesalojo = i->instruccion;

	if(strcmp(motivoDesalojo, "EXIT") == 0){
		log_info(logger,"Finaliza el proceso %d - Motivo: SUCCESS\n", process->pid);
		doExit(process);
	}else if(strcmp(motivoDesalojo, "YIELD") == 0){
		doYield(process);
	}else if(strcmp(motivoDesalojo, "WAIT") == 0){
		char* recurso = i->params[0];
		doWait(process,recurso);		
	}else if(strcmp(motivoDesalojo, "I/O") == 0){
		char* tiempoABloquear_str = i->params[0];
		doIO(process, atoi(tiempoABloquear_str));
	}else if(strcmp(motivoDesalojo, "SIGNAL") == 0){
		char* recurso = i->params[0];
		doSignal(process, recurso);
	}
	else if(strcmp(motivoDesalojo, "F_OPEN") == 0){
		char* archivo = i->params[0];
		do_f_open(archivo,process);
	}
	else if(strcmp(motivoDesalojo, "F_READ") == 0){
		char* archivo = i->params[0];
		int direccion_fisica = recibir_direccion_fisica(conexion_cpu);
		int cantBytes = atoi(i->params[2]);
		if(direccion_fisica!=-1) do_f_read(process,archivo,direccion_fisica,cantBytes);
	}
	else if(strcmp(motivoDesalojo, "F_WRITE") == 0){
		char* archivo = i->params[0];
		int direccion_fisica = recibir_direccion_fisica(conexion_cpu);
		int cantBytes = atoi(i->params[2]);
		if(direccion_fisica!=-1) do_f_write(process,archivo,direccion_fisica,cantBytes);
	}
	else if(strcmp(motivoDesalojo, "F_SEEK") == 0){
		char* archivo = i->params[0];
		int pointer = atoi(i->params[1]);
		do_f_seek(pointer,process,archivo);
	}
	else if(strcmp(motivoDesalojo, "F_CLOSE") == 0){
		char* archivo = i->params[0];
		do_f_close(archivo,process);
	}
	else if(strcmp(motivoDesalojo, "F_TRUNCATE") == 0){
		char* archivo = i->params[0];
		int tam = atoi(i->params[1]);
		do_f_truncate(process,archivo,tam);
	}
	else if(strcmp(motivoDesalojo, "CREATE_SEGMENT") == 0){
		int id = atoi(i->params[0]);
		int tam = atoi(i->params[1]);
		do_create_segment(process,tam,id);
	}
	else if(strcmp(motivoDesalojo, "DELETE_SEGMENT") == 0){
		int id = atoi(i->params[0]);
		do_delete_segment(process,id);
	}
}

void handle_consola(int* socket_cliente){
	log_info(logger, "Cliente conectado");
    int conexion_consola = *socket_cliente;
    int op_code = recibir_operacion(conexion_consola);

	switch (op_code){
		case PAQUETE_INSTRUCCIONES:
			t_list* instrucciones = recibir_instrucciones(conexion_consola);
			t_pcb* pcb_nuevo = crear_pcb(instrucciones, conexion_consola);
			agregar_pcb_cola_nuevos(pcb_nuevo);
			//list_destroy(instrucciones);
			free(socket_cliente);
			break;
		default: 
			log_error(logger, "El codigo de operacion recibido es inválido");
			break;
	}
}

void esperar_clientes() {
	int socket_servidor = crear_servidor(ip,puerto_escucha);
	
   	while (1) {
		log_info(logger, "Esperar Cliente...");
		pthread_t hilo_cliente;
        int socket_cliente = esperar_cliente(socket_servidor);

		int* consola_ptr = malloc(sizeof(int));
		*consola_ptr = socket_cliente;

        pthread_create(&hilo_cliente, NULL, (void*) handle_consola, consola_ptr);
        pthread_detach(hilo_cliente);
    }

	close(socket_servidor);
}




void inicializacionDeVariables(){
	
	pthread_mutex_init(&mutexTablaGlobal,NULL);
	sem_init(&semFS,0,1);
	colasArchivos = list_create();
	recursos_disponibles= list_create();
	tabla_archivos_globales = list_create();
	
	ip = config_get_string_value(config, "IP_KERNEL");
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

    ip_cpu = config_get_string_value(config, "IP_CPU");
	puerto_cpu = config_get_string_value(config, "PUERTO_CPU");

    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

	ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
	puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");

	gradoMaxMultiprog = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");
	algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	hrrn_alfa = config_get_double_value(config, "HRRN_ALFA");
	estimacion_inicial = config_get_double_value(config, "ESTIMACION_INICIAL");

	char** recursos = config_get_array_value(config, "RECURSOS");
	char** instancias_recursos_str = config_get_array_value(config, "INSTANCIAS_RECURSOS");

	// Obtener el número de recursos contando los elementos hasta encontrar el puntero nulo
	int numRecursos = string_array_size(recursos);
    // Crear un arreglo de colas
	colasRecursos= (t_queue **)malloc((numRecursos + 1) * sizeof(t_queue *));
	// Crear una cola por cada elemento del arreglo RECURSOS
    for (int i = 0; i < numRecursos; i++) {
        colasRecursos[i] = queue_create();
		t_recurso* r = malloc(sizeof(t_recurso));
		r->recurso = strdup(recursos[i]);
		r->cantRecurso = atoi(instancias_recursos_str[i]);
        
		list_add(recursos_disponibles,r);
    }
	colasRecursos[numRecursos]=NULL;
	string_array_destroy(instancias_recursos_str);
	string_array_destroy(recursos);

}

void crear_hilo_planificador_largo_plazo() {
   pthread_t long_term_scheduler_thread;
	if(pthread_create(&long_term_scheduler_thread, NULL,(void*) long_term_scheduler, NULL)<0){
		perror("Error al crear el hilo long term");
		exit(EXIT_FAILURE);
	}
	pthread_detach(long_term_scheduler_thread);
}
void crear_hilo_planificador_corto_plazo() {
   pthread_t short_term_scheduler_thread;
	if(pthread_create(&short_term_scheduler_thread, NULL,(void*) short_term_scheduler, NULL)<0){
		perror("Error al crear el hilo short term");
		exit(EXIT_FAILURE);
	}
	pthread_detach(short_term_scheduler_thread);
}
int main()
{
	logger = iniciar_logger("./config/kernel.log","KERNEL");
	config = iniciar_config("./config/kernel.config");

	inicializacionDeVariables();
	
	iniciar_listas_y_semaforos();

	//iniciar_memoria();
	
	/////////////////PLANIFICADORES///////////////////////////
	
	crear_hilo_planificador_largo_plazo();
	
	crear_hilo_planificador_corto_plazo();
	//////////////////PARTE SERVIDOR///////////////////////////
	pthread_t server_thread;
	if(pthread_create(&server_thread, NULL,(void*) esperar_clientes, NULL)<0){
		perror("Error al crear el hilo cliente");
		exit(EXIT_FAILURE);
	}

	pthread_join(server_thread,NULL);
	//liberar_variables();
	
	terminar_programa(logger, config);
	//terminar_conexiones(conexion_memoria,conexion_filesystem);
}

