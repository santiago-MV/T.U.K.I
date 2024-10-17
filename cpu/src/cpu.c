#include "cpu.h"

#define getName(var)  #var

t_log* logger;
// Declaro el config como global para que todos los métodos puedan acceder a él
t_config* config;
int tamanio_segmento_max;
char* ip_memoria;
char* puerto_memoria;
int retardo;
////////////////////// CLIENTE //////////////////////

/*void handle_memoria(void){
	printf("Conectando a Memoria...\n");

	int conexion_memoria;
	char* ip_memoria;
	char* puerto_memoria;

	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	ip_memoria = config_get_string_value(config, "IP_MEMORIA");

	conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
	if(conexion_memoria == -1) {
		perror("Error conexion cpu-memoria");
		log_warning(logger, "cpu.c_handle_memoria --> Error conexion cpu-memoria");
		exit(-1);
	}
	log_info(logger,"CPU conectado al servidor de módulo memoria");

	//Ejecución
	enviar_mensaje("Handshake CPU-Memoria",conexion_memoria);

	terminar_conexion(conexion_memoria);
	log_info(logger,"CPU se desconectó del módulo memoria");
	free(ip_memoria);
	free(puerto_memoria);
}*/

void enviar_paquete_memoria(t_paquete* paquete){
	printf("Conectando a Memoria...\n");

	int conexion_memoria;

	conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
	if(conexion_memoria == -1) {
		perror("Error conexion cpu-memoria");
		log_warning(logger, "cpu.c_handle_memoria --> Error conexion cpu-memoria");
		exit(-1);
	}
	log_info(logger,"CPU conectado al servidor de módulo memoria: enviando paquete");

	//Ejecución
	enviar_paquete(paquete, conexion_memoria);

	terminar_conexion(conexion_memoria);
	log_info(logger,"CPU se desconectó del módulo memoria");
}

////////////////////// SERVIDOR //////////////////////

void handle_clients(void)
{
	char* puerto_escucha;
	char* ip_cpu;
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	ip_cpu = config_get_string_value(config, "IP_CPU");

	int server_cpu = crear_servidor(ip_cpu, puerto_escucha);
	free(puerto_escucha);
	free(ip_cpu);
	if(server_cpu == -1) {
		perror("Error al iniciar el servidor");
		log_warning(logger, "cpu.c_handle_clients --> Error al iniciar el servidor");
		exit(-2);
	}

	while(1){
		log_info(logger, "Servidor listo para recibir al cliente...\n");

		int socket_cliente;
		socket_cliente = esperar_cliente(server_cpu);

		int *socket_ptr = malloc(sizeof(int));
		*socket_ptr = socket_cliente;
		pthread_t hilo;
		pthread_create(&hilo, NULL, (void*) handle_conection, (void*) socket_ptr);
		pthread_join(hilo, NULL);
		free(socket_ptr);
	}
}
void handle_conection(int* socket_cliente){
	t_pcb* pcb;
	//t_paquete paquete_movin;
	int cod_op;

	while(1)
	{
		cod_op = recibir_operacion(*socket_cliente);
		switch (cod_op) {
			case HANDSHAKE:
				recibir_mensaje(*socket_cliente);
				break;
			case PAQUETE_PCB:
				pcb = recibir_pcb(*socket_cliente);
				execute_instrucciones(pcb, *socket_cliente);
				break;
			case RESPONSE:
				pcb = recibir_pcb(*socket_cliente);
				execute_instrucciones(pcb, *socket_cliente);
				break;
			case -1:
				log_info(logger, "El cliente se desconectó.");
				return;
			default:
				log_warning(logger,"Operacion desconocida.");
				break;
		}
		liberar_pcb(pcb); //si no lo comento me salta double free detected core dumped
	}
	free(&cod_op);
}

///////////////////// MMU /////////////////////

t_dir_logica_proc* dir_logica_a_nro_y_offset(int direccion_logica){
	t_dir_logica_proc* direccion_fisica;
	direccion_fisica = malloc(sizeof(t_dir_logica_proc));

	direccion_fisica->numero_segmento = floor(direccion_logica / tamanio_segmento_max);
	direccion_fisica->offset_segmento = direccion_logica % tamanio_segmento_max;

	return direccion_fisica;
}

int mmu(t_dir_logica_proc* direc_logica, t_tabla_proceso* tabla){
	int direccion_fisica;
	t_segmento registro_tabla_segmentos;
	registro_tabla_segmentos = tabla->segmentos[direc_logica->numero_segmento];
	direccion_fisica = registro_tabla_segmentos.base_address + direc_logica->offset_segmento;
	return direccion_fisica;
}


void printInstruccion(instruction_set* i){
	printf("%s",i->instruccion);
	for(int j=0;j<string_array_size(i->params);j++){
		printf(" %s",i->params[j]);
	}
	printf("\n");
}
void printListaInstruccion(t_list* instrucciones){
	printf("\n");
	for(int j=0;j<list_size(instrucciones);j++){
		printInstruccion(list_get(instrucciones, j));
	}
}
///////////////////// EJECUCION /////////////////////

void execute_instrucciones(t_pcb* pcb, int socket_cliente){

	instruction_set* instruccion;
	
	char* string_params;
	//printListaInstruccion(pcb->instructions);
	// Loggear ejecución
	while((instruccion = list_get(pcb->instructions, pcb->program_counter)) != NULL){
		char* codigo_instruccion = malloc(strlen(instruccion->instruccion)+1);
		codigo_instruccion[strlen(instruccion->instruccion)] = '\0';
		strcpy(codigo_instruccion,instruccion->instruccion);

		string_params = lista_params_to_string(instruccion);
		log_info(logger, "PID: %d - PC: %d - Ejecutando: %s - Parametros:%s\n", pcb->pid, pcb->program_counter, codigo_instruccion, string_params);
		free(string_params);
		pcb->program_counter++;
		if(codigo_instruccion != NULL){
			if(strcmp(codigo_instruccion, "MOV_IN") == 0){
				if(execute_movin(pcb, instruccion, socket_cliente) == -1){
					free(codigo_instruccion);
					break;
				}
			} 
			else if(strcmp(codigo_instruccion, "MOV_OUT") == 0){
				if(execute_movout(pcb, instruccion, socket_cliente)==-1){
					free(codigo_instruccion);
					break;
				}
			} 
			else if(strcmp(codigo_instruccion, "SET") == 0){
				execute_set(pcb, instruccion);
			} 
			else if(strcmp(codigo_instruccion, "I/O") == 0){
				execute_bloq(pcb, socket_cliente);
				free(codigo_instruccion);
				break;
			}
			else if(strcmp(codigo_instruccion, "WAIT") == 0){
				execute_bloq(pcb, socket_cliente);
				free(codigo_instruccion);
				break;
			}
			else if(strcmp(codigo_instruccion, "SIGNAL") == 0){
				execute_bloq(pcb, socket_cliente);
				free(codigo_instruccion);
				break;
			}
			else if(strcmp(codigo_instruccion, "F_OPEN") == 0){
				execute_in_kernel(pcb, CREAR_ARCHIVO, socket_cliente);
				free(codigo_instruccion);
				break;
			}
			else if(strcmp(codigo_instruccion, "F_CLOSE") == 0){
				execute_bloq(pcb, socket_cliente);
				free(codigo_instruccion);
				break;
			}
			else if(strcmp(codigo_instruccion, "F_SEEK") == 0){
				execute_in_kernel(pcb, F_SEEK, socket_cliente);
				free(codigo_instruccion);
				break;
			}
			else if(strcmp(codigo_instruccion, "F_READ") == 0){
				execute_bloq(pcb, socket_cliente);
				enviar_direccion_fisica(instruccion->params[1], pcb->segment_tables,socket_cliente);
				free(codigo_instruccion);
				break;
			}
			else if(strcmp(codigo_instruccion, "F_WRITE") == 0){
				execute_bloq(pcb, socket_cliente);
				enviar_direccion_fisica(instruccion->params[1], pcb->segment_tables, socket_cliente);
				free(codigo_instruccion);
				break;
			}
			else if(strcmp(codigo_instruccion, "F_TRUNCATE") == 0){
				execute_bloq(pcb, socket_cliente);
				free(codigo_instruccion);
				break;
			}
			else if(strcmp(codigo_instruccion, "CREATE_SEGMENT") == 0){
				execute_in_kernel(pcb, CREAR_SEGMENTO, socket_cliente);
				free(codigo_instruccion);
				break;
			}
			else if(strcmp(codigo_instruccion, "DELETE_SEGMENT") == 0){
				execute_in_kernel(pcb, BORRAR_SEGMENT, socket_cliente);
				free(codigo_instruccion);
				break;
			}
			else if(strcmp(codigo_instruccion, "EXIT") == 0){
				execute_bloq(pcb, socket_cliente);
				free(codigo_instruccion);
				break;
			}
			else if(strcmp(codigo_instruccion, "YIELD") == 0){
				execute_bloq(pcb, socket_cliente);
				free(codigo_instruccion);
				break;
			}
			else {
				log_warning(logger,"Instrucción desconocida.");
			}
		}
		free(codigo_instruccion);
		
	}
}

void execute_in_kernel(t_pcb* pcb, op_code op_code,int socket_cliente){
	t_paquete* paquete_pcb = crear_paquete_pcb(pcb);
	paquete_pcb->codigo_operacion = op_code;
	enviar_paquete(paquete_pcb, socket_cliente);
	eliminar_paquete(paquete_pcb);
}

void execute_bloq(t_pcb* pcb, int socket_cliente){
	t_paquete* paquete_pcb = crear_paquete_pcb(pcb);
	enviar_paquete(paquete_pcb, socket_cliente);

	eliminar_paquete(paquete_pcb);
}

int execute_movin(t_pcb* pcb, instruction_set* instruccion, int socket_cliente){
	char* reg_name = instruccion->params[0];
	int tamanio_reg = get_tamanio_registro(reg_name);
	int direc_logica = atoi(instruccion->params[1]);
	t_dir_logica_proc* direc_logica_proc;

	// Obtengo direccion fisica
	direc_logica_proc = dir_logica_a_nro_y_offset(direc_logica);
	
	int direc_fisica = mmu(direc_logica_proc, pcb->segment_tables);
	int tam_max = pcb->segment_tables->segmentos[direc_logica_proc->numero_segmento].size - pcb->segment_tables->segmentos[direc_logica_proc->numero_segmento].base_address;

	if (direc_logica_proc->offset_segmento + tamanio_reg > tam_max){
		log_error(logger,"MOV_IN: PID: %d - Error SEG_FAULT- SEGMENTO: %d- OFFSET: %d - TAMANIO REGISTRO: %d \n",pcb->pid, direc_logica_proc->numero_segmento, direc_logica_proc->offset_segmento, tamanio_reg);
		enviar_seg_fault(socket_cliente, pcb);
		return -1;
	}

	// Creo el paquete para enviar a memoria
	t_paquete* pedido_lectura = crear_paquete_con_codigo_op(PEDIDO_LECTURA_CPU);
	agregar_entero_a_paquete(pedido_lectura, pcb->pid);
	agregar_entero_a_paquete(pedido_lectura, direc_fisica);
	agregar_entero_a_paquete(pedido_lectura, tamanio_reg);

	// Envío a memoria
	int conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
	enviar_paquete(pedido_lectura, conexion_memoria);
	eliminar_paquete(pedido_lectura);

	//if(recibir_operacion(conexion) != FIN_PROCESO) return EXIT_FAILURE;
	if(recibir_operacion(conexion_memoria) != VALOR_LEIDO){
		log_error(logger, "MOV_IN: No se obtuvo correctamente el valor de memoria");
		return -1;
	}
	
    int size;
	void* buffer = recibir_buffer(&size, conexion_memoria);
	int tamanio_valor_leido;
	memcpy(&tamanio_valor_leido, buffer, sizeof(int));

	char* valor_leido = malloc(tamanio_valor_leido+1);
    memcpy(valor_leido, buffer + sizeof(int), tamanio_valor_leido);
	valor_leido[tamanio_valor_leido] = '\0';
	log_info(logger, "PID: %d - Acción: Leído - Segmento: %d - Dirección Física: %d - Valor: %s \n", pcb->pid, direc_logica_proc->numero_segmento, direc_fisica, valor_leido);

	// Guardar valor_leido en reg
	guardar_en_reg(pcb, valor_leido, reg_name);

	terminar_conexion(conexion_memoria);
	free(valor_leido);
	//free(reg_name);
	liberar_direccion(direc_logica_proc);
	free(buffer);
	return 1;
}

int execute_movout(t_pcb* pcb, instruction_set* instruccion, int socket_kernel){
	int offset_total;
	int tamanio_contenido;
	char* reg_name = instruccion->params[1];
	int direc_logica = atoi(instruccion->params[0]);
	t_dir_logica_proc* direc_logica_proc;
	t_paquete* paquete_response; // Envía el pcb al kernel si hay SEG_FAULT o envía el registro a guardar a memoria
	
	// Separo la direccion logica en nro_segmento y offset
	char* contenido_reg = get_reg_value(pcb, reg_name);
	direc_logica_proc = dir_logica_a_nro_y_offset(direc_logica);
    int dir_fisica = mmu(direc_logica_proc,pcb->segment_tables);
	
	// Obtengo el tamanio de lo que voy a guardar y lo comparo con el tamanio del segmento
	//tamanio_contenido = get_tamanio_registro(reg_name);
	tamanio_contenido = strlen(contenido_reg);
	offset_total = direc_logica_proc->offset_segmento + tamanio_contenido;
	int tam_max = pcb->segment_tables->segmentos[direc_logica_proc->numero_segmento].size - pcb->segment_tables->segmentos[direc_logica_proc->numero_segmento].base_address;

	if(offset_total > tam_max){
		log_error(logger,"MOV_OUT: PID: %d - Error SEG_FAULT- SEGMENTO: %d- OFFSET: %d - TAMANIO CONTENIDO: %d \n",pcb->pid, direc_logica_proc->numero_segmento, direc_logica_proc->offset_segmento, tamanio_contenido);
		enviar_seg_fault(socket_kernel, pcb);
		free(contenido_reg);
		liberar_direccion(direc_logica_proc);
		return -1;
	}
	else
	{
		log_info(logger, "PID: %d - Acción: Escribir - Segmento: %d - Dirección Física: %d - Valor: %s \n", pcb->pid, direc_logica_proc->numero_segmento, dir_fisica, contenido_reg);
		
		//paquete_response = crear_paquete_movout(direc_fisica, contenido_reg);
		paquete_response = crear_paquete_con_codigo_op(PEDIDO_ESCRITURA_CPU);
		agregar_entero_a_paquete(paquete_response, pcb->pid);
		agregar_entero_a_paquete(paquete_response, dir_fisica);
		agregar_a_paquete(paquete_response, contenido_reg, tamanio_contenido);
		
		int conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
		enviar_paquete(paquete_response, conexion_memoria);
		eliminar_paquete(paquete_response);

		if(recibir_operacion(conexion_memoria) != OK){
			log_error(logger, "MOV_OUT: No se pudo guardar correctamente el valor en memoria");

		}
		
		terminar_conexion(conexion_memoria);
	}

	//free(reg_name);
	free(contenido_reg);
	liberar_direccion(direc_logica_proc);
	//free(registro_tabla_segmentos);
	return 1;
}

void execute_set(t_pcb* pcb, instruction_set* instruccion){
	char* reg_name = instruccion->params[0]; 
	char* contenido = instruccion->params[1];
	usleep(retardo*1000);
	guardar_en_reg(pcb, contenido, reg_name);
	//free(reg_name);
	//free(contenido);
}



char* get_reg_value(t_pcb* pcb, char* reg_name){
	char* contenido;
	if(strcmp("AX" , reg_name) == 0){
		contenido = malloc(5);
		strncpy(contenido, pcb->cpu_registers.AX, 4);
		contenido[4]='\0';
	}
	else if(strcmp("BX" , reg_name) == 0){
		contenido = malloc(5);
		strncpy(contenido, pcb->cpu_registers.BX, 4);
		contenido[4]='\0';
	}
	else if(strcmp("CX" , reg_name) == 0){
		contenido = malloc(5);
		strncpy(contenido, pcb->cpu_registers.CX, 4);
		contenido[4]='\0';
	}
	else if(strcmp("DX" , reg_name) == 0){
		contenido = malloc(5);
		strncpy(contenido, pcb->cpu_registers.DX, 4);
		contenido[4]='\0';
	}
	else if(strcmp("EAX" , reg_name) == 0){
		contenido = malloc(9);
		strncpy(contenido, pcb->cpu_registers.EAX, 8);
		contenido[8]='\0';
	}
	else if(strcmp("EBX" , reg_name) == 0){
		contenido = malloc(9);
		strncpy(contenido, pcb->cpu_registers.EBX, 8);
		contenido[8]='\0';
	}
	else if(strcmp("ECX" , reg_name) == 0){
		contenido = malloc(9);
		strncpy(contenido, pcb->cpu_registers.ECX, 8);
		contenido[8]='\0';
	}
	else if(strcmp("EDX" , reg_name) == 0){
		contenido = malloc(9);
		strncpy(contenido, pcb->cpu_registers.EDX, 8);
		contenido[8]='\0';
	}
	else if(strcmp("RAX" , reg_name) == 0){
		contenido = malloc(17);
		strncpy(contenido, pcb->cpu_registers.RAX, 16);
		contenido[16]='\0';
	}
	else if(strcmp("RBX" , reg_name) == 0){
		contenido = malloc(17);
		strncpy(contenido, pcb->cpu_registers.RBX, 16);
		contenido[16]='\0';
	}
	else if(strcmp("RCX" , reg_name) == 0){
		contenido = malloc(17);
		strncpy(contenido, pcb->cpu_registers.RCX, 16);
		contenido[16]='\0';
	}
	else if(strcmp("RDX" , reg_name) == 0){
		contenido = malloc(17);
		strncpy(contenido, pcb->cpu_registers.RDX, 16);
		contenido[16]='\0';
	}
	return contenido;
}

void guardar_en_reg(t_pcb* pcb, char* contenido, char* reg_name){
	if(strcmp("AX", reg_name) == 0){
		strncpy(pcb->cpu_registers.AX, contenido, 4);
	}
	else if(strcmp("BX", reg_name) == 0){
		strncpy(pcb->cpu_registers.BX, contenido, 4);
	}
	else if(strcmp("CX", reg_name) == 0){
		strncpy(pcb->cpu_registers.CX, contenido, 4);
	}
	else if(strcmp("DX", reg_name) == 0){
		strncpy(pcb->cpu_registers.DX, contenido, 4);
	}
	else if(strcmp("EAX" , reg_name) == 0){
		strncpy(pcb->cpu_registers.EAX, contenido, 8);
	}
	else if(strcmp("EBX" , reg_name) == 0){
		strncpy(pcb->cpu_registers.EBX, contenido, 8);
	}
	else if(strcmp("ECX", reg_name) == 0){
		strncpy(pcb->cpu_registers.ECX, contenido, 8);
	}
	else if(strcmp("EDX" , reg_name) == 0){
		strncpy(pcb->cpu_registers.EDX, contenido, 8);
	}
	else if(strcmp("RAX" , reg_name) == 0){
		strncpy(pcb->cpu_registers.RAX, contenido, 16);
	}
	else if(strcmp("RBX" , reg_name) == 0){
		strncpy(pcb->cpu_registers.RBX, contenido, 16);
	}
	else if(strcmp("RCX" , reg_name) == 0){
		strncpy(pcb->cpu_registers.RCX, contenido, 16);
	}
	else if(strcmp("RDX" , reg_name) == 0){
		strncpy(pcb->cpu_registers.RDX, contenido, 16);
	}
}


/////////////////////////////////////////////////////

int main()
{

	logger = iniciar_logger("./config/cpu.log", "CPU_logger");
	config = iniciar_config("./config/cpu.config");
	
	retardo = config_get_int_value(config,"RETARDO_INSTRUCCION");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	tamanio_segmento_max = config_get_int_value(config, "TAM_MAX_SEGMENTO");

	////////////////// CLIENTE ////////////////////

	//pthread_t client_thread;
	//pthread_create(&client_thread, NULL,(void*) handle_memoria, NULL);

	////////////////// SERVIDOR ////////////////////

	pthread_t server_thread;
	pthread_create(&server_thread, NULL,(void*) handle_clients, NULL);

	////////////////// FINALIZAR //////////////////

	pthread_join(server_thread,NULL);
	//pthread_join(client_thread,NULL);

	free(ip_memoria);
	free(puerto_memoria);

	terminar_programa(logger, config);
}

///////////////// INICIADORES Y FINALIZADORES ////////////////////

void terminar_conexion(int conexion)
{
	if(conexion!=0)
	{
		liberar_conexion(conexion);
	}
}
