/*
 * harness.c -- Corre un programa de ArquiSim SIN interfaz, con los botones puestos,
 *              y saca el estado final en JSON para compararlo con referencia.py.
 *
 * NO cambia el procesador: se enlaza con los mismos archivos de src/ (ciclo.c, cpu.c,
 * isa.c, ensamblador.c, sistema.c y ejecutar.c) sin modificarlos. Solo reemplaza a
 * main.c/tui.c, que son la interfaz. Hace lo mismo que hace la interfaz cuando uno
 * escribe  mem[0x600] = valor  en el paso 0: llama a cambiar_botones() (cpu.c) y
 * después ejecuta con ejecutar_paso() (ciclo.c) hasta que el programa termina.
 *
 * Compilar (desde la carpeta del simulador):
 *   gcc -std=c99 -O2 -Isrc -o entrega/herramientas/harness entrega/herramientas/harness.c \
 *       src/ciclo.c src/cpu.c src/isa.c src/ensamblador.c src/sistema.c src/ejecutar.c
 *
 * Usar:
 *   harness isa.txt programa.asm BOTONES [MAX_PASOS] [--llamadas] [--pasos]
 *
 * Además del estado final registra, mirando cada instrucción ANTES de ejecutarla:
 *   - cada color escrito en 0x608 (en orden)          -> "rgb_escrituras"
 *   - cuántas veces se escribió la posición 0x604     -> "escrituras_pos"
 *   - cuántas veces se leyeron los botones (0x600)    -> "lecturas_botones"
 *   - cuántas veces se escribió sobre la imagen       -> "escrituras_imagen"
 *   - el valor más bajo que tuvo $sp                  -> "sp_min"
 *   - con --llamadas: ($a0 al entrar, $v0 al salir) de cada función llamada con jal
 *   - con --pasos: una línea por instrucción ejecutada en la salida de errores (stderr):
 *         PC | texto de la instrucción | salto (0/1) | los 32 registros después de ejecutarla
 *     (sirve para armar las trazas de la guía de sustentación)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "isa.h"
#include "ensamblador.h"
#include "ciclo.h"

#define MAX_RGB      8192
#define MAX_LLAMADAS 8192
#define MAX_PILA     64

typedef struct { uint32_t destino, a0, v0; } Llamada;

static ISA      isa;
static CPU      cpu, antes;
static Programa prog;
static uint32_t rgb_log[MAX_RGB];
static Llamada  llamadas[MAX_LLAMADAS], pila[MAX_PILA];

static int es(const Instruccion *in, const char *nombre)
{
    return in->def && strcmp(in->def->nombre, nombre) == 0;
}

static void hex_bytes(const char *clave, const uint8_t *p, int n)
{
    int i;
    printf("  \"%s\": \"", clave);
    for (i = 0; i < n; i++)
        printf("%02X", p[i]);
    printf("\",\n");
}

int main(int argc, char **argv)
{
    Instruccion in;
    EstadoPaso r;
    long long botones = 0, max_pasos = 100000;
    int pasos = 0, limite = 0, con_llamadas = 0, con_pasos = 0, i, x, y;
    int n_rgb = 0, n_pos = 0, n_bot = 0, n_img = 0, n_llam = 0, tope = 0;
    uint32_t sp_min, dir;

    if (argc < 4 || !leer_numero(argv[3], &botones)) {
        fprintf(stderr, "uso: %s isa.txt programa.asm BOTONES [MAX_PASOS] [--llamadas] [--pasos]\n", argv[0]);
        return 2;
    }
    for (i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--llamadas") == 0)
            con_llamadas = 1;
        else if (strcmp(argv[i], "--pasos") == 0)
            con_pasos = 1;
        else if (!leer_numero(argv[i], &max_pasos) || max_pasos <= 0) {
            fprintf(stderr, "MAX_PASOS inválido: %s\n", argv[i]);
            return 2;
        }
    }
    if (!cargar_isa(argv[1], &isa))
        return 2;
    if (!cargar_programa(&isa, argv[2], &prog, &cpu)) {
        fputs(prog.errores, stderr);
        fprintf(stderr, "No se pudo ensamblar '%s'.\n", argv[2]);
        return 2;
    }
    cambiar_botones(&cpu, (uint32_t)botones);   /* igual que  mem[0x600] = valor  en la interfaz */
    sp_min = cpu.reg[REG_SP];

    for (;;) {
        if (pasos >= max_pasos && !programa_terminado(&prog, &cpu)) {
            limite = 1;
            break;
        }
        antes = cpu;
        r = ejecutar_paso(&isa, &prog, &cpu, &in);
        if (r == PASO_FIN)
            break;
        pasos++;

        /* --- solo observar: nada de lo que sigue modifica la máquina --- */
        dir = antes.reg[in.rs] + (uint32_t)in.simm;
        if (es(&in, "sw") && dir == RGB_COLOR_DIR && n_rgb < MAX_RGB)
            rgb_log[n_rgb++] = antes.reg[in.rt];
        if (es(&in, "sw") && dir == RGB_POS_DIR)
            n_pos++;
        if ((es(&in, "lw") || es(&in, "lb") || es(&in, "lbu")) && dir >= BOTONES_DIR && dir < BOTONES_DIR + 4)
            n_bot++;
        if ((es(&in, "sw") || es(&in, "sb")) && dir >= DATA_BASE && dir < DATA_BASE + 0x100)
            n_img++;
        if (es(&in, "jal") && tope < MAX_PILA) {
            pila[tope].destino = cpu.pc;
            pila[tope].a0 = antes.reg[4];
            tope++;
        }
        if (es(&in, "jr") && in.rs == REG_RA && tope > 0) {
            tope--;
            if (con_llamadas && n_llam < MAX_LLAMADAS) {
                llamadas[n_llam] = pila[tope];
                llamadas[n_llam].v0 = cpu.reg[2];
                n_llam++;
            }
        }
        if (cpu.reg[REG_SP] < sp_min)
            sp_min = cpu.reg[REG_SP];
        if (con_pasos) {
            const LineaFuente *lf = buscar_fuente(&prog, antes.pc);
            fprintf(stderr, "0x%03X|%s|%d", antes.pc, lf ? lf->texto : "?", cpu.pc != antes.pc + 4);
            for (i = 0; i < NUM_REGISTROS; i++)
                fprintf(stderr, "|%u", cpu.reg[i]);
            fputc('\n', stderr);
        }
        if (r == PASO_EXCEPCION)
            break;
    }

    printf("{\n");
    printf("  \"botones\": %lld,\n", botones);
    printf("  \"pasos\": %d,\n", pasos);
    printf("  \"R\": %u, \"I\": %u, \"J\": %u,\n", cpu.est.instr[FMT_R], cpu.est.instr[FMT_I], cpu.est.instr[FMT_J]);
    printf("  \"lecturas\": %u, \"escrituras\": %u, \"saltos\": %u,\n",
           cpu.est.lecturas, cpu.est.escrituras, cpu.est.saltos);
    printf("  \"limite\": %d,\n", limite);
    printf("  \"excepcion\": \"%s\",\n", cpu.excepcion);
    printf("  \"num_instr\": %d, \"text_fin\": %u, \"data_fin\": %u,\n", prog.num_instr, prog.text_fin, prog.data_fin);
    printf("  \"pc_final\": %u, \"sp_final\": %u, \"sp_min\": %u,\n", cpu.pc, cpu.reg[REG_SP], sp_min);
    printf("  \"lecturas_botones\": %d, \"escrituras_pos\": %d, \"escrituras_imagen\": %d,\n", n_bot, n_pos, n_img);
    printf("  \"botones_final\": %u, \"rgb_pos_final\": %u,\n", palabra_en(&cpu, BOTONES_DIR), palabra_en(&cpu, RGB_POS_DIR));
    printf("  \"reg\": [");
    for (i = 0; i < NUM_REGISTROS; i++)
        printf("%s%u", i ? ", " : "", cpu.reg[i]);
    printf("],\n");
    hex_bytes("imagen", cpu.ram + DATA_BASE, 0x100);
    hex_bytes("datos", cpu.ram + DATA_BASE + 0x100, 0x100);
    hex_bytes("video", cpu.ram + VIDEO_BASE, VIDEO_BYTES);
    printf("  \"rgb_escrituras\": [");
    for (i = 0; i < n_rgb; i++)
        printf("%s%u", i ? ", " : "", rgb_log[i]);
    printf("],\n");
    printf("  \"rgb_final\": [");
    for (y = 0; y < RGB_ALTO; y++)
        for (x = 0; x < RGB_ANCHO; x++)
            printf("%s%u", (x || y) ? ", " : "", cpu.rgb[y][x]);
    printf("],\n");
    printf("  \"llamadas\": [");
    for (i = 0; i < n_llam; i++) {
        const char *nombre = etiqueta_en(&prog, llamadas[i].destino);
        printf("%s[\"%s\", %u, %u]", i ? ", " : "", nombre ? nombre : "?", llamadas[i].a0, llamadas[i].v0);
    }
    printf("]\n}\n");
    return (cpu.excepcion[0] || limite) ? 1 : 0;
}
