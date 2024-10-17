#include "shared_utils.h"

t_log *iniciar_logger(char *relativePath, char *id)
{
	t_log *logger = log_create(relativePath, id, true, LOG_LEVEL_INFO);

	if (logger == NULL)
	{
		log_info(logger, "No se pudo crear el logger");
		exit(EXIT_FAILURE);
	}

	return logger;
}

t_config *iniciar_config(char *path)
{
	t_config *config;
	if ((config = config_create(path)) == NULL)
	{
		printf("No pude crear el config");
		exit(EXIT_FAILURE);
	}
	return config;
}

void terminar_programa(t_log *logger, t_config *config)
{
	if (logger != NULL)
	{
		log_destroy(logger);
	}
	if (config != NULL)
	{
		config_destroy(config);
	}
}

int crear_servidor(char *ip, char *puerto)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del modulo memoria
	if ((socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == 0)
	{
		perror("Error al crear el socket");
		exit(EXIT_FAILURE);
	}

	int yes = 1;
	setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

	// Asociamos el socket a un puerto
	if (bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
	{
		perror("Error al asignar la dirección y el puerto");
		exit(EXIT_FAILURE);
	}
	// Escuchamos las conexiones entrantes
	if (listen(socket_servidor, SOMAXCONN) < 0)
	{
		perror("Error al escuchar conexiones entrantes");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(servinfo);

	return socket_servidor;
}

void crear_buffer(t_paquete *paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete *crear_paquete_con_codigo_op(op_code codigo_op)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo_op;
	crear_buffer(paquete);
	return paquete;
}

int crear_conexion(char *ip, char *puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family,
								server_info->ai_socktype,
								server_info->ai_protocol);

	if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) < 0)
	{
		perror("Error al conectar con server");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(server_info);

	return socket_cliente;
}

void liberar_conexion(int socket)
{
	close(socket);
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}
void recibir_mensaje(int socket_cliente)
{
	int size;
	char *buffer = recibir_buffer(&size, socket_cliente);
	// log_info(logger, "Me llego el mensaje %s", buffer);
	printf("Me llego el mensaje %s\n", buffer);
	free(buffer);
}
int recibir_entero(int conexion)
{
	int size;
	void *buffer = recibir_buffer(&size, conexion);
	int entero;
	memcpy(&entero, buffer, sizeof(int));
	free(buffer);
	return entero;
}
// SERVIDOR
void *recibir_buffer(int *size, int socket_cliente)
{
	void *buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}
// SERVIDOR
int esperar_cliente(int socket_servidor)
{
	int socket_cliente;
	if ((socket_cliente = accept(socket_servidor, NULL, NULL)) == -1)
	{
		printf("No se pudo conectar el modulo\n");
		return EXIT_FAILURE;
	}
	// log_info(logger, "Se conecto un cliente!");
	return socket_cliente;
}

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void agregar_entero_a_paquete(t_paquete *paquete, int x)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &x, sizeof(int));

	paquete->buffer->size += sizeof(int);
}

void *serializar_paquete(t_paquete *paquete, int bytes)
{
	void *magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;

	return magic;
}

void enviar_paquete(t_paquete *paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2 * sizeof(int);
	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete *paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void enviar_mensaje(char *mensaje, int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = HANDSHAKE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void *serializar_instruction_set(instruction_set *instruccion, int *tamanio_serializado)
{
	int tamanio_instruccion = strlen(instruccion->instruccion);
	int tamanio_params = 0;

	for (int i = 0; instruccion->params[i] != NULL; i++)
	{
		tamanio_params += strlen(instruccion->params[i]);
	}

	*tamanio_serializado = sizeof(int) + tamanio_instruccion + sizeof(int) + string_array_size(instruccion->params) * sizeof(int) + tamanio_params;

	void *buffer = malloc(*tamanio_serializado);
	int offset = 0;

	memcpy(buffer + offset, &tamanio_instruccion, sizeof(int));
	offset += sizeof(int);
	memcpy(buffer + offset, instruccion->instruccion, tamanio_instruccion);
	offset += tamanio_instruccion;

	int x = string_array_size(instruccion->params);
	memcpy(buffer + offset, &x, sizeof(int));
	offset += sizeof(int);

	for (int i = 0; instruccion->params[i] != NULL; i++)
	{
		int tamanio_param = strlen(instruccion->params[i]);
		memcpy(buffer + offset, &tamanio_param, sizeof(int));
		offset += sizeof(int);
		memcpy(buffer + offset, instruccion->params[i], tamanio_param);
		offset += tamanio_param;
	}

	return buffer;
}

int recibir_direccion_base(int socket_cliente)
{
	int size;
	void *buffer;
	int direccion_base;

	buffer = recibir_buffer(&size, socket_cliente);

	memcpy(&direccion_base, buffer, sizeof(int));

	free(buffer);

	return direccion_base;
}


t_list *deserializar_tabla_segmentos(void *buffer)
{
	t_list *tabla = list_create();
	/*
		void* stream = buffer->stream;

		memcpy(&tabla->id, stream, sizeof(int));
		stream += sizeof(int);

		memcpy(&tabla->base_address, stream, sizeof(int));
		stream += sizeof(int);

		memcpy(&tabla->size, stream, sizeof(int));
		stream += sizeof(int);
	*/
	return tabla;
}

t_paquete *serializar_tabla_segmentos(table_segment *tabla, int cod_op)
{
	t_paquete *paquete = crear_paquete_con_codigo_op(cod_op);

	int bytes = sizeof(int) * 3;

	paquete->buffer->size = bytes;

	void *stream = malloc(paquete->buffer->size);
	int offset = 0;

	memcpy(stream + offset, &tabla->id, sizeof(int));
	offset += sizeof(int);

	memcpy(stream + offset, &tabla->base_address, sizeof(int));
	offset += sizeof(int);

	memcpy(stream + offset, &tabla->size, sizeof(int));
	offset += sizeof(int);

	paquete->buffer->stream = stream;

	return paquete;
}

t_pcb *deserializarPcb(void *buffer, int size)
{
	t_pcb *pcb = malloc(sizeof(t_pcb));
	int offset = 0;
	pcb->recursos_asignados = list_create();
	pcb->instructions = list_create();
	pcb->rafaga_ejecucion_anterior = NULL;
	pcb->espera_en_ready = NULL;
	// Deserialamos pid
	memcpy(&pcb->pid, buffer + offset, sizeof(int));
	offset += sizeof(int);
	// Deserialamos pc
	memcpy(&pcb->program_counter, buffer + offset, sizeof(int));
	offset += sizeof(int);
	// Deserialamos cpuregs
	memcpy(&pcb->cpu_registers, buffer + offset, sizeof(cpu_regs));
	offset += sizeof(cpu_regs);
	// Deserializar lista instrucciones
	int largoInstrucciones;
	memcpy(&largoInstrucciones, buffer + offset, sizeof(int));
	offset += sizeof(int);
	int tamanio = 0;
	for (int k = 0; k < largoInstrucciones; k++)
	{
		int cantParams;
		instruction_set *i = malloc(sizeof(instruction_set));

		memcpy(&tamanio, buffer + offset, sizeof(int));
		offset += sizeof(int);

		i->instruccion = malloc(tamanio+1);

		memcpy(i->instruccion, buffer + offset, tamanio);
		offset += tamanio;

		i->instruccion[tamanio] = '\0';

		memcpy(&cantParams, buffer + offset, sizeof(int));
		offset += sizeof(int);

		i->params = malloc((cantParams + 1) * sizeof(char *));
		
		for (int j = 0; j < cantParams; j++)
		{
			// TODO tamanio del parametro mal calculada
			memcpy(&tamanio, buffer + offset, sizeof(int));
			offset += sizeof(int);

			i->params[j] = malloc(tamanio+1);
			memcpy(i->params[j], buffer + offset, tamanio);
			offset += tamanio;
			i->params[j][tamanio] = '\0';
		}

		i->params[cantParams] = '\0';
		list_add(pcb->instructions, i);
	}

	// Deserialamos lista recursos
	int largoLista;
	memcpy(&largoLista, buffer + offset, sizeof(int));
	offset += sizeof(int);

	for (int i = 0; i < largoLista; i++)
	{

		int largoCadena;
		memcpy(&largoCadena, buffer + offset, sizeof(int));
		offset += sizeof(int);

		t_recurso *r = malloc(sizeof(t_recurso));

		memcpy(&r->cantRecurso, buffer + offset, sizeof(int));
		offset += sizeof(int);
		r->recurso = malloc(largoCadena+1);
		memcpy(r->recurso, buffer + offset, largoCadena);
		r->recurso[largoCadena] = '\0';
		offset += largoCadena;

		list_add(pcb->recursos_asignados, r);
	}

	pcb->segment_tables = malloc(sizeof(t_tabla_proceso));

	memcpy(&pcb->segment_tables->cant_segmentos_actuales,buffer+offset,sizeof(int));
	offset+=sizeof(int);

	pcb->segment_tables->segmentos = calloc(pcb->segment_tables->cant_segmentos_actuales * sizeof(t_segmento), sizeof(uint32_t));

	memcpy(&pcb->segment_tables->pid,buffer+offset,sizeof(int));
	offset+=sizeof(int);

	for(int i=0; i<pcb->segment_tables->cant_segmentos_actuales; i++){
		memcpy(&pcb->segment_tables->segmentos[i].base_address,buffer+offset,sizeof(int));
		offset+=sizeof(int);
		memcpy(&pcb->segment_tables->segmentos[i].size,buffer+offset,sizeof(int));
		offset+=sizeof(int);
		memcpy(&pcb->segment_tables->segmentos[i].id,buffer+offset,sizeof(int));
		offset+=sizeof(int);
	}

	free(buffer);

	return pcb;
}

t_paquete *crear_paquete_pcb(t_pcb *pcb)
{
	t_paquete *paquete_pcb = crear_paquete_con_codigo_op(PAQUETE_PCB);
	int tamanio_instructions = 0;
	int tamanio_recursos = 0;
	for (int j = 0; j < list_size(pcb->instructions); j++)
	{
		instruction_set *ins = list_get(pcb->instructions, j);
		tamanio_instructions += sizeof(int);
		tamanio_instructions += strlen(ins->instruccion);
		tamanio_instructions += sizeof(int);
		for (int i = 0; ins->params[i] != NULL; i++)
		{
			tamanio_instructions += sizeof(int);
			tamanio_instructions += strlen(ins->params[i]);
		}
	}
	for (int j = 0; j < list_size(pcb->recursos_asignados); j++)
	{
		t_recurso *r = list_get(pcb->recursos_asignados, j);
		tamanio_recursos += strlen(r->recurso);
		tamanio_recursos += 2 * sizeof(int);
	}

	int cant = pcb->segment_tables->cant_segmentos_actuales;
	int tamanio_serializado = sizeof(int) + sizeof(int) + sizeof(cpu_regs) + sizeof(int) + tamanio_instructions + sizeof(int) + tamanio_recursos + sizeof(int) + sizeof(pcb->segment_tables) + sizeof(t_segmento)*cant;

	int offset = 0;
	void *buffer = malloc(tamanio_serializado);
	memset(buffer, 0, tamanio_serializado);

	// Serializar pid
	memcpy(buffer + offset, &(pcb->pid), sizeof(int));
	offset += sizeof(int);

	// Serializar program_counter
	memcpy(buffer + offset, &(pcb->program_counter), sizeof(int));
	offset += sizeof(int);

	// Serializar cpu_registers
	memcpy(buffer + offset, &(pcb->cpu_registers), sizeof(cpu_regs));
	offset += sizeof(cpu_regs);

	// Serializar instructions
	int largoInstrucciones = list_size(pcb->instructions);
	memcpy(buffer + offset, &largoInstrucciones, sizeof(int));
	offset += sizeof(int);

	for (int j = 0; j < largoInstrucciones; j++)
	{
		int tamanio;
		void *valor = serializar_instruction_set(list_get(pcb->instructions, j), &tamanio);
		memcpy(buffer + offset, valor, tamanio);
		offset += tamanio;
		free(valor);
	}
	// Serializar recursos
	int largo;
	largo = list_size(pcb->recursos_asignados);
	memcpy(buffer + offset, &largo, sizeof(int));
	offset += sizeof(int);
	for (int i = 0; i < largo; i++)
	{
		t_recurso *r = list_get(pcb->recursos_asignados, i);
		int tamanioCadena = strlen(r->recurso);
		memcpy(buffer + offset, &tamanioCadena, sizeof(int));
		offset += sizeof(int);
		memcpy(buffer + offset, &r->cantRecurso, sizeof(int));
		offset += sizeof(int);
		memcpy(buffer + offset, r->recurso, tamanioCadena);
		offset += tamanioCadena;
	}

    memcpy(buffer+offset,&pcb->segment_tables->cant_segmentos_actuales,sizeof(int));
    offset+=sizeof(int);

	memcpy(buffer+offset,&pcb->segment_tables->pid,sizeof(int));
	offset+=sizeof(int);

	for(int i=0; i<cant; i++){
		memcpy(buffer+offset,&pcb->segment_tables->segmentos[i].base_address,sizeof(int));
		offset+=sizeof(int);
		memcpy(buffer+offset,&pcb->segment_tables->segmentos[i].size,sizeof(int));
		offset+=sizeof(int);
		memcpy(buffer+offset,&pcb->segment_tables->segmentos[i].id,sizeof(int));
		offset+=sizeof(int);
	}

	paquete_pcb->buffer->stream = buffer;
	paquete_pcb->buffer->size = tamanio_serializado;

	return paquete_pcb;
}

t_pcb *recibir_pcb(int socket_cliente)
{
	int size;
	void *buffer;

	buffer = recibir_buffer(&size, socket_cliente);
	t_pcb *process;
	process = deserializarPcb(buffer, size);

	return process;
}

void liberar_recurso(void *elemento)
{
	t_recurso *r = (t_recurso *)elemento;
	free(r->recurso);
	free(r);
}

void liberar_instruccion(void *elemento)
{
	instruction_set *i = (instruction_set *)elemento;
	free(i->instruccion);
	int j;
	for (j = 0; i->params[j] != NULL; j++)
	{
		free(i->params[j]);
	}
	free(i->params[j]);
	free(i->params);
	free(i);
}

void liberar_archivo(void* elemento){
	table_open_files* archivo = (table_open_files*) elemento;
	free(archivo->file_ptr);
	free(archivo);
}

void liberar_pcb(void *elemento)
{
	t_pcb *pcb = (t_pcb *)elemento;
	//if (pcb->espera_en_ready != NULL)
	//	temporal_destroy(pcb->espera_en_ready);
	//if (pcb->rafaga_ejecucion_anterior != NULL)
	//	temporal_destroy(pcb->rafaga_ejecucion_anterior);
	list_destroy_and_destroy_elements(pcb->instructions, liberar_instruccion);
	if (!list_is_empty(pcb->recursos_asignados) && pcb->recursos_asignados != NULL)
	{
		list_destroy_and_destroy_elements(pcb->recursos_asignados, liberar_recurso);
	}
	else if(list_is_empty(pcb->recursos_asignados))
	{
		list_destroy(pcb->recursos_asignados);
	}
	if(pcb->open_files != NULL && list_is_empty(pcb->open_files)){
		list_destroy(pcb->open_files);
	}else if(pcb->open_files != NULL){
		list_destroy_and_destroy_elements(pcb->open_files, liberar_archivo);
	}
	free(pcb->segment_tables->segmentos);
	free(pcb->segment_tables);
	free(pcb);
}
