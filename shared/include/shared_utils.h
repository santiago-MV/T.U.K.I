#ifndef SHARED_UTILS_H
#define SHARED_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <string.h> 
#include <commons/config.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <commons/collections/list.h>
#include <assert.h>
#include<commons/temporal.h>
#include<commons/string.h>
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
      SEG_FAULT
}op_code;
typedef struct
{
	int size;
	void* stream;
} t_buffer;
typedef struct
{
      char *instruccion;
      char **params;
} instruction_set;

typedef struct
{
      char AX[4]; // Registros de 4 bytes
      char BX[4];
      char CX[4];
      char DX[4];
      char EAX[8]; // Registros de 8 bytes
      char EBX[8];
      char ECX[8];
      char EDX[8];
      char RAX[16]; // Registros de 16 bytes
      char RBX[16];
      char RCX[16];
      char RDX[16];
} cpu_regs;

typedef struct
{
      int id;
      int base_address;
      int size;
} table_segment;

typedef struct
{
      FILE *file_ptr;
      int pointer_pos;
} table_open_files;

typedef struct
{
      int id;
      u_int32_t base_address;
      u_int32_t size;
} t_segmento;
typedef struct
{
      char *recurso;
      int cantRecurso;
} t_recurso;

typedef struct
{
    int pid;
    t_segmento* segmentos;
    int cant_segmentos_actuales;
} t_tabla_proceso;
typedef struct{
  int pid;
  t_list* instructions;         // lista de tipo t_segment_info
  int program_counter;
  int conexion_consola;
  cpu_regs cpu_registers;
  t_tabla_proceso* segment_tables;    // lista de tipo table_segment
  double est_next_burst;
  t_list* open_files;        // lista de tipo table_open_files
  t_list* recursos_asignados;
  t_temporal* espera_en_ready;
  t_temporal* rafaga_ejecucion_anterior;
  int cant_max_seg;
} t_pcb;
typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

t_log* iniciar_logger(char*,char*);
t_config* iniciar_config(char*);
void terminar_programa(t_log*,t_config*);
int crear_servidor(char*, char*);
int crear_conexion(char*, char*);
void liberar_conexion(int);
int recibir_operacion(int );
void recibir_mensaje(int);
void* recibir_buffer(int* , int );
int esperar_cliente(int );
int recibir_entero(int );
t_pcb* deserializarPcb(void* buffer,int size);
t_paquete* crear_paquete_pcb(t_pcb* pcb);
void* serializar_instruction_set(instruction_set* , int* );
void agregar_entero_a_paquete(t_paquete *paquete, int x);
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
t_pcb* recibir_pcb(int socket_cliente);
t_paquete* serializar_tabla_segmentos(table_segment* tabla, int cod_op);
t_list* deserializar_tabla_segmentos(void* buffer);
#endif