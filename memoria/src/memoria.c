#include "memoria.h"

void handle_conection(int* socket_cliente){
	int pid = -1, dir_fisica_cpu, dir_fisica_fs, tamanio_a_leer_fs, tamanio_a_leer_cpu;
	int socket = *socket_cliente;
	int sid;
	while(1){
		int cod_op = recibir_operacion(socket);
		
		switch (cod_op){
			case HANDSHAKE:
				recibir_mensaje(socket);
				break;
			case INICIALIZAR_ESTRUCTURAS:
				pid = recibir_entero(socket);
				log_info(logger, "Creación de Proceso PID: %d", pid);
				inicializar_estructuras_pcb(pid, socket);
				break;
			case CREAR_SEGMENTO:
				int tam;
				pid = recibir_crear_segmento(socket, &tam, &sid);		
				log_info(logger, "Peticion recibida: CREAR_SEGMENTO");
				crear_segmento(pid, sid, tam, socket);
				break;
			case BORRAR_SEGMENT:
				
				pid = recibir_eliminar_segmento(socket, &sid);
				
				log_info(logger, "Peticion recibida: ELIMINAR_SEGMENTO");
				eliminar_segmento(pid, sid, true);
				
				
				
				enviar_ok(socket);
				break;
			case PEDIDO_LECTURA_CPU:	
				dir_fisica_cpu = recibir_pedido_lectura(socket, &tamanio_a_leer_cpu, &pid);
				
				log_info(logger, "PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: CPU", pid, dir_fisica_cpu, tamanio_a_leer_cpu);
				
				char* valor_leido_cpu = malloc(tamanio_a_leer_cpu+1);
				leer_direccion_fisica(valor_leido_cpu, dir_fisica_cpu, tamanio_a_leer_cpu);
				valor_leido_cpu[tamanio_a_leer_cpu]='\0';
				enviar_valor_leido(socket, valor_leido_cpu);
				free(valor_leido_cpu);
			break;
			case PEDIDO_LECTURA_FS:
			
				dir_fisica_fs = recibir_pedido_lectura(socket, &tamanio_a_leer_fs, &pid);
				
				log_info(logger, "PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: FS", pid, dir_fisica_fs, tamanio_a_leer_fs);
				
				char* valor_leido_fs = malloc(tamanio_a_leer_fs+1);
				leer_direccion_fisica(valor_leido_fs, dir_fisica_fs, tamanio_a_leer_fs);
				valor_leido_fs[tamanio_a_leer_fs] = '\0';

				enviar_valor_leido(socket, valor_leido_fs);
				free(valor_leido_fs);
			break;
			case PEDIDO_ESCRITURA_CPU:
				char* valor_a_escribir = recibir_pedido_escritura(socket, &dir_fisica_cpu, &pid);
				
				log_info(logger, "PID: %d - Acción: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: CPU", pid, dir_fisica_cpu, (int)strlen(valor_a_escribir));
				
				escribir_en_direccion_fisica(valor_a_escribir, dir_fisica_cpu);
				free(valor_a_escribir);
				enviar_ok(socket);
			break;
			case PEDIDO_ESCRITURA_FS:
				char* valor_a_escribir_fs = recibir_pedido_escritura(socket, &dir_fisica_fs, &pid);
				
				log_info(logger, "PID: %d - Acción: ESCRIBIR - Dirección física: %d - Tamaño: %d - Origen: FS", pid, dir_fisica_fs, tamanio_a_leer_fs);
				
				escribir_en_direccion_fisica(valor_a_escribir_fs, dir_fisica_fs);
				free(valor_a_escribir_fs);
			
				enviar_ok(socket);
			break;
			case FIN_PROCESO:
				pid = recibir_fin_proceso(socket);
				
				log_info(logger,  "Eliminación de Proceso PID: %d", pid);
		
				fin_proceso(pid);
			break;
			case COMPACTAR:
				log_info(logger, "Solicitud de Compactación");
				compactar_memoria(socket);
			break;
			case -1:
				return;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
		}
	}
}

void handle_clients(int* server_fd)
{
	while(1){
		int socket_cliente= esperar_cliente(*server_fd);

		int *socket_ptr = malloc(sizeof(int));
		*socket_ptr = socket_cliente;
		pthread_t hilo;
		pthread_create(&hilo, NULL, (void*) handle_conection, (void*) socket_ptr);
	}
}

void procesar_archivo_config_memoria(t_config* config) {
    ip = config_get_string_value(config, "IP_MEMORIA");
    puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    tam_segmento_0 = config_get_int_value(config, "TAM_SEGMENTO_0");
    cant_segmentos = config_get_int_value(config, "CANT_SEGMENTOS");
    retardo_memoria = config_get_int_value(config, "RETARDO_MEMORIA");
    retardo_compactacion = config_get_int_value(config, "RETARDO_COMPACTACION");
    algorimo_asignacion = config_get_string_value(config, "ALGORITMO_ASIGNACION");;
}

int main() {
	logger = iniciar_logger("./config/memoria.log","MEMORIA");
	config = iniciar_config("./config/memoria.config");
	
	procesar_archivo_config_memoria(config);
	
	iniciar_estructuras_memoria();
	
	int server_fd = crear_servidor(ip,puerto);

	int *server_fd_ptr = malloc(sizeof(int));
	*server_fd_ptr = server_fd;

	log_info(logger, "Memoria listo para recibir a un Modulo");

	pthread_t client_thread;
	pthread_create(&client_thread, NULL,(void*) handle_clients, (void*)server_fd_ptr);

	pthread_join(client_thread,NULL);
}
