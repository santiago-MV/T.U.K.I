#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/log.h>
#include<readline/readline.h>
typedef enum
{
	HANDSHAKE,
	PAQUETE_INSTRUCCIONES,
	PAQUETE_PCB,
    FIN_PROCESO
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct{
	char* instruccion;
	char** params;
} instruction_set;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

t_log* iniciar_logger(char*,char*);
t_config* iniciar_config(char*);
void terminar_programa(t_log*,t_config*);
int crear_conexion(char*, char*);
void liberar_conexion(int);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(void);
t_paquete* crear_super_paquete(void);
t_paquete* crear_paquete_con_codigo_op(op_code codigo_op);
void* serializar_instruction_set(instruction_set* , int* );
int recibir_operacion(int );
void* recibir_buffer(int*, int);
int recibir_entero(int);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void agrega_instruccion_a_paquete(t_paquete* paquete,instruction_set* i);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void printInstruccion(instruction_set* );
t_list* deserializarListaInstruccion(void* ,int );
t_paquete* paquete_instrucciones(FILE* codigo);
//instruction_set* deserializarInstruccion(void* , int );
#endif /* UTILS_H_ */