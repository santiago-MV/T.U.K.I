#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/config.h>
#include<commons/log.h>
#include<pthread.h>

extern t_log* logger;
extern t_config* config;
extern int conexion_memoria;
typedef enum
{
      HANDSHAKE,
      PAQUETE_INSTRUCCIONES,
      PAQUETE_PCB,
      FIN_PROCESO,
      CREACION_INICIAL_PROCESO,
      CREAR_SEGMENTO,
      BORRAR_SEGMENT,
      OUT_OF_MEMORY,
      SEGMENTO_CREADO,
      COMPACTAR,
      CREAR_ARCHIVO,
      INICIALIZAR_ESTRUCTURAS,
      TABLA_PROCESO,
      ESCRIBIR_ARCHIVO,
      LEER_ARCHIVO,
      TRUNCAR_ARCHIVO,
      SEG_FAULT,
      PEDIDO_LECTURA_CPU,
      PEDIDO_ESCRITURA_CPU,
      PEDIDO_LECTURA_FS,
      PEDIDO_ESCRITURA_FS,
      VALOR_LEIDO,
      OK,
      DIREC_FISICA,
      F_SEEK,
      ABRIR_ARCHIVO,
      RESPONSE,
      NO_EXISTE_ARCHIVO
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;


void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(void);
t_paquete* crear_super_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void agregar_entero_a_paquete(t_paquete* paquete , int entero);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
t_paquete* paquete_instrucciones(FILE* codigo);
int crear_servidor(char*,char*);
int recibir_operacion(int );
void recibir_mensaje(int);
void* recibir_buffer(int*,int);
int esperar_cliente(int );
int crear_conexion(char*, char*);
void liberar_conexion(int);
t_log* iniciar_logger(char*,char*);
t_config* iniciar_config(char*);
t_paquete* crear_paquete_con_codigo_op(op_code codigo_op);
void finalizar_programa(t_log* , t_config*);
char* recibir_archivo(int socket_cliente);
void terminar_programa(t_log *logger, t_config *config);


#endif /* UTILS_H_ */