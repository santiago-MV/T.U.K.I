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
#include<commons/log.h>
#include<commons/config.h>
#include<commons/collections/queue.h>
#include<commons/collections/list.h>
#include<pthread.h>
#include<semaphore.h>
#include<assert.h>
#include<commons/temporal.h>
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
      RESPONSE
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
      u_int32_t base_address;
      u_int32_t size;
} t_segmento;

typedef struct
{
      char *file_ptr;
      int pointer_pos;
} table_open_files;
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
} t_pcb;
typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;
typedef struct
{
	t_pcb* pcb;
	int tiempoBloqueo;
} t_args;
extern t_log* logger;
extern t_config* config;

t_log* iniciar_logger(char*,char*);
t_config* iniciar_config(char*);
void terminar_programa(t_log*,t_config*);
int recibir_operacion(int );
void recibir_mensaje(int);
t_list* recibir_instrucciones(int );
int crear_servidor(char*, char*);
int crear_conexion(char*, char*);
void terminar_conexion(int );
void enviar_mensaje(char* , int );
void eliminar_paquete(t_paquete* );
void agregar_entero_a_paquete(t_paquete* paquete,int x);
int esperar_cliente(int);
void* recibir_buffer(int*, int);
void liberar_conexion(int);
bool algoritmo_es_hrrn();
void enviar_pcb_cpu(t_pcb* pcb, int conexion_cpu);
void agregar_a_paquete(t_paquete*,void*,int);
t_paquete* serializar_tabla_segmentos(t_tabla_proceso* tabla, int cod_op);
int recibir_direccion_base(int socket_cliente);
void enviar_pcb(t_pcb* pcb, int conexion_cpu);
t_paquete* crear_paquete_con_codigo_op(op_code codigo_op);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
t_list* deserializarListaInstruccion(void* ,int );
void* serializar_instruction_set(instruction_set*, int*);
void printInstruccion(instruction_set* i);
void printListaInstruccion(t_list*);
void analizarMotivo(t_pcb*,int);
int recibir_direccion_fisica(int);
t_pcb* deserializarPcb(void*,int);
t_pcb* recibir_pcb(int);
int encontrarIndiceRecursoEnLista(t_list* , char* );
void doExit(t_pcb* process);
void doWait(t_pcb* process, char* recurso);
void doYield(t_pcb* process);
void doIO(t_pcb* process, int tiempoBloqueo);
void doSignal(t_pcb* process, char* recurso);
t_paquete* crear_paquete_pcb(t_pcb* pcb);
void print_tabla(t_tabla_proceso* tabla, int cant_segmentos);
void recibir_pcb_de_CPU(int conexion);
void terminar_conexiones(int,int);
t_pcb* buscar_pid(int);
t_list* recibir_tabla_segmentos(int socket_cliente);
void liberar_recurso(void*);
void liberar_instruccion(void*);
void liberar_pcb(void*);
void liberarRecursosAsignados(t_list* recursosDisponibles, t_list* recursosAsignados);
void liberar_archivo(void* elemento);
#endif /* UTILS_H_ */