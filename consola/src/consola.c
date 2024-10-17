#include <stdio.h>
#include <stdlib.h>
#include "consola.h"

t_log* logger;


int main(int argc, char** argv) {

	if (argc < 3){
		return EXIT_FAILURE;
	}

	int conexion;
	char *ip,*puerto;

	// Inicializacion de variables
    t_config* config = iniciar_config(argv[1]);
    logger = iniciar_logger("./config/consola.log","CONSOLA");
	ip = config_get_string_value(config, "IP_KERNEL");
	puerto = config_get_string_value(config, "PUERTO_KERNEL");
	
	FILE* archivo_pseudocodigo = fopen(argv[2],"r");
	if(archivo_pseudocodigo == NULL) return EXIT_FAILURE;
	
	//Se crea paquete instrucciones para kernel
	t_paquete* instrucciones = paquete_instrucciones(archivo_pseudocodigo);
	
	conexion = crear_conexion(ip, puerto);

	//enviar_mensaje("HANDSHAKE CONSOLA-KERNEL",conexion);
	enviar_paquete(instrucciones,conexion);

	//Recibir fin proceso
	
	int op_code = recibir_operacion(conexion);
	if(op_code != FIN_PROCESO) return EXIT_FAILURE;
    int pid = recibir_entero(conexion);

    close(conexion);
	
	log_info(logger, "El proceso %d finalizó", pid);

	// Libera la memoria utilizada
	eliminar_paquete(instrucciones);
	close(conexion);
	terminar_programa(logger,config);
	fclose(archivo_pseudocodigo);
	
	return EXIT_SUCCESS;
}
