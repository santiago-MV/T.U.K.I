#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<math.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>
#include<commons/collections/list.h>
#include<assert.h>

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
      BUSCAR_ARCHIVO,
      RESPONSE
}op_code;

typedef enum
{
  MOV_IN,
  MOV_OUT,
	SET,
  IO,
  WAIT,
  SIGNAL,
	YIELD,
	EXIT
}instruction_code;

typedef struct
{
      int id;
      u_int32_t base_address;
      u_int32_t size;
} t_segmento;
typedef struct
{
    int pid;
    t_segmento* segmentos;
    int cant_segmentos_actuales;
} t_tabla_proceso;
typedef struct
{
  int numero_segmento;
  int offset_segmento;
} t_dir_logica_proc;

typedef struct
{
  char* reg_name;
  char* contenido;
  int tamanio_contenido;
} t_registro_info;



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

typedef struct{
	char* instruccion;
	char** params;
} instruction_set;

typedef struct {
    char AX[4];  // Registros de 4 bytes
    char BX[4];
    char CX[4];
    char DX[4];
    char EAX[8];  // Registros de 8 bytes
    char EBX[8];
    char ECX[8];
    char EDX[8];
    char RAX[16]; // Registros de 16 bytes
    char RBX[16];
    char RCX[16];
    char RDX[16];
}cpu_regs;


typedef struct{
	  FILE* file_ptr;
	  int pointer_pos;
}table_open_files;


typedef struct{
  int pid;
  t_list* instructions;         // lista de tipo t_segment_info
  int program_counter;
  cpu_regs cpu_registers;
  t_tabla_proceso* segment_tables;    // lista de tipo t_segment_info
  double est_next_burst;
  t_list* open_files;        // lista de tipo t_file_info
  t_list* recursos_asignados;
  int cant_max_seg;
} t_pcb;

typedef struct{
      char* recurso;
      int cantRecurso;
}t_recurso;

t_dir_logica_proc* dir_logica_a_nro_y_offset(int direccion_logica);
int mmu(t_dir_logica_proc* direc_logica, t_tabla_proceso* tabla);
t_paquete* crear_paquete_movin(int dir_fisica, t_registro_info registro_info);
t_paquete* crear_paquete_movout(int dir_fisica, char* contenido);
t_segmento* get_t_segmento_by_id(t_list* lista, int id);
int get_tamanio_registro(char* reg_name);
void printInstruccion(instruction_set* i);
int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
void liberar_conexion(int socket_cliente);
t_paquete* crear_paquete_pcb(t_pcb* pcb);
t_log* iniciar_logger(char* , char*);
t_config* iniciar_config(char*);
void crear_buffer_pcb(t_paquete* paquete, t_pcb* pcb);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
int concatenarEnteros(int,int);
void eliminar_paquete(t_paquete* paquete);
int crear_servidor(char*, char*);
void enviar_direccion_fisica(char* dir_logica,t_tabla_proceso*, int socket_cliente);
void enviar_seg_fault(int socket_cliente,t_pcb* pcb);
int esperar_cliente(int);
void recibir_mensaje(int);
int recibir_operacion(int);
void* recibir_buffer(int*, int);
void crear_buffer(t_paquete* paquete);
t_pcb* recibir_pcb(int socket_cliente);
t_paquete* crear_paquete_con_codigo_op(op_code codigo_op);
void* serializar_instruction_set(instruction_set* instruccion, int* tamanio_serializado);
char* lista_params_to_string(instruction_set* i);
void agregar_entero_a_paquete(t_paquete *paquete, int x);
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
void liberar_pcb(void* elemento);
void liberar_instruccion(void* elemento);
void liberar_recurso(void* elemento);
void liberar_direccion(void* elemento);
#endif /* UTILS_H_ */