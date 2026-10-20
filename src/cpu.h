/*
 * cpu.h -- Estado de la máquina ArquiSim y herramientas para implementar instrucciones.
 */
#ifndef CPU_H
#define CPU_H

#include <stdint.h>

/* ---------------- Parámetros de la máquina ---------------- */
#define NUM_REGISTROS  32
#define RAM_BYTES      1024u        /* memoria unificada (Von Neumann)     */
#define TEXT_BASE      0x000u       /* inicio del segmento de código       */
#define DATA_BASE      0x200u       /* inicio del segmento de datos        */
#define PILA_INICIAL   RAM_BYTES    /* $sp inicial; la pila crece hacia abajo */

/* Pantalla mapeada en memoria (E/S): 32 x 16 píxeles, 1 byte por píxel.
 * Dirección del píxel (x, y) = VIDEO_BASE + 32*y + x.  Color = 4 bits menos
 * significativos: 0 negro, 1 rojo, 2 verde, 3 amarillo, 4 azul, 5 magenta,
 * 6 cian, 7 blanco, 8-15 las mismas versiones brillantes. */
#define VIDEO_BASE     0x400u
#define VIDEO_ANCHO    32
#define VIDEO_ALTO     16
#define VIDEO_BYTES    (VIDEO_ANCHO * VIDEO_ALTO)

/* Registros de E/S (una palabra cada uno):
 *   0x600 BOTONES       bits 7..0 = botones (1 = activado)            solo lectura
 *   0x604 TEMPORIZADOR  interrumpir cada N instrucciones (0 = apagado)
 *   0x608 VECTOR        dirección de la rutina de servicio (ISR)
 *   0x60C CAUSA         qué pidió la última interrupción              solo lectura
 *   0x610 RGB_POSICION  pantalla RGB: (y << 16) | x
 *   0x614 RGB_COLOR     pantalla RGB: escribir 0x00RRGGBB dibuja el píxel en la posición
 *                       y avanza x (autoincremento). La imagen vive DENTRO del dispositivo. */
#define BOTONES_DIR    (VIDEO_BASE + VIDEO_BYTES)   /* 0x600 */
#define TEMPORIZADOR_DIR (BOTONES_DIR + 4)          /* 0x604 */
#define VECTOR_DIR     (BOTONES_DIR + 8)            /* 0x608 */
#define CAUSA_DIR      (BOTONES_DIR + 12)           /* 0x60C */

#define RGB_POS_DIR    (BOTONES_DIR + 16)           /* 0x610 */
#define RGB_COLOR_DIR  (BOTONES_DIR + 20)           /* 0x614 */
#define RGB_ANCHO      32
#define RGB_ALTO       32

#define MEM_BYTES      (BOTONES_DIR + 24)           /* espacio de direcciones total */

/* Fuentes de interrupción (bits de CAUSA y de las pendientes) */
#define INT_BOTONES      0x1u   /* un botón pasó de 0 a 1           */
#define INT_TEMPORIZADOR 0x2u   /* se cumplió el periodo del timer  */

#define REG_SP 29
#define REG_RA 31

/* ---------------- Definición de una instrucción (viene de isa.txt) ---------------- */
#define MAX_MNEMONICO 8
#define MAX_PATRON    32

typedef enum { FMT_R, FMT_I, FMT_J } Formato;

typedef struct {
    char     mnemonico[MAX_MNEMONICO];  /* lo que se escribe: "add", "lw", ... */
    char     nombre[MAX_MNEMONICO];     /* identidad en el hardware (ejecutar.c): normalmente
                                           igual al mnemónico; "adda" para add $t1 */
    Formato  formato;                   /* R, I o J                          */
    uint32_t opcode;                    /* 6 bits                            */
    int      funct;                     /* 6 bits (tipo R) o -1 si no aplica */
    char     operandos[MAX_PATRON];     /* sintaxis, p. ej. "rd,rs,rt" o "rd+rs,rt" */
} DefInstruccion;

/* ---------------- Instrucción decodificada ---------------- */
typedef struct {
    uint32_t binario;     /* los 32 bits de la instrucción                 */
    uint32_t op;          /* bits 31-26                                    */
    uint32_t rs;          /* bits 25-21                                    */
    uint32_t rt;          /* bits 20-16                                    */
    uint32_t rd;          /* bits 15-11                                    */
    uint32_t shamt;       /* bits 10-6                                     */
    uint32_t funct;       /* bits 5-0                                      */
    uint32_t imm;         /* bits 15-0 SIN extender (0 .. 65535)           */
    int32_t  simm;        /* bits 15-0 con extensión de signo (-32768 .. 32767) */
    uint32_t direccion;   /* bits 25-0 (tipo J)                            */
    const DefInstruccion *def;   /* NULL si el opcode no existe en el ISA  */
} Instruccion;

/* ---------------- Registro de banderas (como en el PDUA) ---------------- */
#define BANDERA_Z  0x8u    /* el resultado es cero                          */
#define BANDERA_N  0x4u    /* el resultado es negativo (bit 31 = 1)         */
#define BANDERA_C  0x2u    /* acarreo (suma) o préstamo (resta) sin signo   */
#define BANDERA_V  0x1u    /* desbordamiento con signo                      */

/* ---------------- Estadísticas de ejecución ---------------- */
typedef struct {
    uint32_t instr[3];        /* instrucciones ejecutadas por formato R, I, J  */
    uint32_t lecturas;        /* lecturas de datos en memoria (sin contar el fetch) */
    uint32_t escrituras;      /* escrituras en memoria (RAM y video)           */
    uint32_t saltos;          /* saltos tomados (el PC no avanzó +4)           */
    uint32_t interrupciones;  /* interrupciones atendidas                      */
} Estadisticas;

/* ---------------- Estado del procesador ---------------- */
typedef struct {
    uint32_t reg[NUM_REGISTROS];   /* banco de registros                    */
    uint32_t pc;                   /* Program Counter                       */
    uint32_t ir;                   /* Instruction Register                  */
    uint32_t a, b;                 /* registros internos: operandos de la ALU    */
    uint32_t temp;                 /* registro interno: resultado de la ALU      */
    uint32_t hi, lo;               /* resultado de mult/div (fuera del banco)    */
    uint32_t acc;                  /* acumulador (fuera del banco de registros) */
    uint32_t banderas;             /* registro de banderas: Z N C V             */
    uint32_t ie;                   /* interrupciones habilitadas (1) o no (0)   */
    uint32_t epc;                  /* PC al que se vuelve con eret               */
    uint32_t pendientes;           /* interrupciones pedidas y no atendidas      */
    uint32_t cuenta_temporizador;  /* instrucciones desde la última del timer    */
    uint32_t int_atendida;         /* causa atendida en este paso (0 = ninguna)  */
    uint8_t  ram[MEM_BYTES];       /* RAM 0x000-0x3FF, video 0x400-0x5FF, botones 0x600 */
    uint32_t rgb[RGB_ALTO][RGB_ANCHO];  /* memoria interna de la pantalla RGB  */
    char     excepcion[160];       /* mensaje de excepción ("" = ninguna)   */
    char     consola[1024];        /* salida de los syscall de impresión         */
    int      pide_entero;          /* syscall 5: la interfaz debe leer un entero */
    int      terminado;            /* syscall 10                                 */
    uint32_t espera_ms;            /* syscall 32: pausa pedida en este paso      */
    Estadisticas est;
} CPU;

typedef enum { OK = 0, EXCEPCION = 1 } Resultado;

/* ---------------- Herramientas (cpu.c) ---------------- */

/* Escribe un registro. Las escrituras a $zero se ignoran. */
void      escribir_registro(CPU *cpu, uint32_t r, uint32_t valor);

/* Acceso a memoria. Verifican rango y alineación; si fallan generan una excepción.
 * La memoria es big-endian: el byte más significativo va en la dirección menor. */
Resultado leer_palabra(CPU *cpu, uint32_t dir, uint32_t *valor);
Resultado escribir_palabra(CPU *cpu, uint32_t dir, uint32_t valor);
Resultado leer_byte(CPU *cpu, uint32_t dir, uint8_t *valor);
Resultado escribir_byte(CPU *cpu, uint32_t dir, uint8_t valor);

/* Fetch: lee una instrucción (verifica rango y alineación, no cuenta como lectura de datos). */
Resultado leer_instruccion(CPU *cpu, uint32_t dir, uint32_t *valor);

/* Escribe una palabra sin verificar ni contar (ediciones desde la interfaz y dispositivos). */
void      poner_palabra(CPU *cpu, uint32_t dir, uint32_t valor);

/* Pantalla RGB: dibuja 'color' en la posición actual y avanza la posición. */
void      rgb_escribir_color(CPU *cpu, uint32_t color);

/* Cambia el estado de los botones; si alguno pasa de 0 a 1 pide una interrupción. */
void      cambiar_botones(CPU *cpu, uint32_t botones);

/* Lee una palabra sin verificar nada (para mostrar la memoria). dir debe ser válida. */
uint32_t  palabra_en(const CPU *cpu, uint32_t dir);

/* Detiene el procesador con un mensaje (se usa como en printf). Devuelve EXCEPCION. */
Resultado excepcion(CPU *cpu, const char *formato, ...);

/* Calculan a + b o a - b y actualizan las banderas Z, N, C y V. Devuelven el resultado. */
uint32_t  sumar_con_banderas(CPU *cpu, uint32_t a, uint32_t b);
uint32_t  restar_con_banderas(CPU *cpu, uint32_t a, uint32_t b);

/* Operaciones lógicas: actualiza Z y N según el resultado y pone C = V = 0. Devuelve r. */
uint32_t  banderas_logicas(CPU *cpu, uint32_t r);

/* Indican si a + b o a - b se salen del rango de un entero de 32 bits con signo. */
int       desborda_suma(int32_t a, int32_t b);
int       desborda_resta(int32_t a, int32_t b);

/* ---------------- Lo implementa el estudiante (ejecutar.c) ---------------- */
Resultado ejecutar(CPU *cpu, const Instruccion *in);

#endif
