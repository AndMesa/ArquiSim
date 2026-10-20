/*
 * main.c -- ArquiSim: simulador de un procesador MIPS reducido.
 *
 * Uso: ./sim [opciones] programa.asm
 *
 * Modos:
 *   interactivo (por defecto en una terminal): pantalla completa que se refresca en cada paso
 *   --traza: imprime todos los pasos uno tras otro (útil para guardar en un archivo)
 *   -q:      solo el estado final
 *   -t:      prueba automática contra programa.esperado
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "isa.h"
#include "ensamblador.h"
#include "ciclo.h"
#include "mostrar.h"
#include "terminal.h"
#include "tui.h"
#include "sistema.h"

#define MAX_PASOS_DEFECTO 10000

typedef enum { MODO_INTERACTIVO, MODO_TRAZA, MODO_SILENCIOSO, MODO_PRUEBA } Modo;

typedef struct {
    const char *programa;
    const char *isa;
    Modo modo;
    int tabla;           /* -a */
    int color;
    int max_pasos;       /* -n */
    FILE *entrada;       /* de dónde lee syscall 5 sin interfaz */
} Opciones;

static void uso(const char *nombre)
{
    fprintf(stderr,
        "Uso: %s [opciones] [programa.asm]\n"
        "\n"
        "Sin opciones abre la interfaz interactiva (pantalla completa). Sin programa,\n"
        "muestra la lista de programas de la carpeta programas/ para elegir uno.\n"
        "\n"
        "Opciones:\n"
        "  --traza     imprime todos los pasos uno tras otro, sin interfaz\n"
        "  -a          con --traza: muestra el banco de registros en cada paso\n"
        "  -q          silencioso: solo el estado final\n"
        "  -t          prueba: compara el estado final con programa.esperado\n"
        "  -i archivo  archivo del ISA (por defecto: isa.txt)\n"
        "  -n N        máximo de instrucciones a ejecutar (por defecto: %d)\n"
        "  --sin-color desactiva los colores\n"
        "  --ascii     bordes con + - | (si la terminal no muestra bien las cajas)\n"
        "  --tema T    tema de colores: %s\n"
        "              (también con la variable de entorno ARQUISIM_TEMA)\n"
        "\n"
        "Comandos de la interfaz interactiva:\n"
        "  Enter  ejecutar una instrucción     b  retroceder una instrucción\n"
        "  c      continuar hasta el final     a [ms]  tiempo real: una instrucción cada ms\n"
        "         (mientras corre: 0-7 botones, + / - velocidad, espacio pausa)\n"
        "  m dir  mostrar la memoria desde dir 0  reiniciar     q  salir\n"
        "  p      alternar programa/pantalla   l  cargar otro programa  r  recargar\n"
        "  e      editar el programa (usa $EDITOR; por defecto nano) y recargarlo\n"
        "  t      cambiar el tema de colores\n",
        nombre, MAX_PASOS_DEFECTO, lista_temas());
}

static int leer_opciones(int argc, char **argv, Opciones *op)
{
    int i;

    memset(op, 0, sizeof *op);
    op->isa = "isa.txt";
    op->color = terminal_salida_es_tty();
    op->max_pasos = MAX_PASOS_DEFECTO;
    op->modo = terminal_interactiva() ? MODO_INTERACTIVO : MODO_TRAZA;
    if (getenv("ARQUISIM_TEMA"))
        elegir_tema(getenv("ARQUISIM_TEMA"));

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--traza") == 0)
            op->modo = MODO_TRAZA;
        else if (strcmp(argv[i], "-a") == 0)
            op->tabla = 1;
        else if (strcmp(argv[i], "-q") == 0)
            op->modo = MODO_SILENCIOSO;
        else if (strcmp(argv[i], "-t") == 0)
            op->modo = MODO_PRUEBA;
        else if (strcmp(argv[i], "--sin-color") == 0)
            op->color = 0;
        else if (strcmp(argv[i], "--ascii") == 0)
            tui_usar_ascii(1);
        else if (strcmp(argv[i], "--tema") == 0 && i + 1 < argc) {
            if (!elegir_tema(argv[++i])) {
                fprintf(stderr, "tema desconocido '%s' (use: %s)\n", argv[i], lista_temas());
                return 0;
            }
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc)
            op->isa = argv[++i];
        else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc)
            op->max_pasos = atoi(argv[++i]);
        else if (argv[i][0] == '-')
            return 0;
        else if (!op->programa)
            op->programa = argv[i];
        else
            return 0;
    }
    return (op->programa != NULL || op->modo == MODO_INTERACTIVO) && op->max_pasos > 0;
}

static char *recortar(char *s)
{
    char *fin;

    while (isspace((unsigned char)*s))
        s++;
    fin = s + strlen(s);
    while (fin > s && isspace((unsigned char)fin[-1]))
        *--fin = '\0';
    return s;
}

/* Elimina los espacios: permite escribir "mem [ 0x610 ]" o "rgb[3, 2]". */
static void quitar_espacios(char *t)
{
    char *d = t;

    for (; *t; t++)
        if (*t != ' ' && *t != '\t')
            *d++ = *t;
    *d = '\0';
}

/* Compara el estado final con el archivo .esperado. Devuelve 1 si todo coincide. */
static int verificar(const char *ruta_asm, const CPU *cpu, int limite)
{
    char ruta[512], linea[256], fallas_txt[4096] = "";
    char *punto, *s, *igual, *lhs, *rhs;
    FILE *f;
    int fallas = 0, total = 0, espera_excepcion = 0, espera_limite = 0, r, num = 0;
    long long v, dir;
    uint32_t esperado, obtenido;
    size_t usado;

#define FALLA(...) do { usado = strlen(fallas_txt); \
        snprintf(fallas_txt + usado, sizeof fallas_txt - usado, __VA_ARGS__); fallas++; } while (0)

    snprintf(ruta, sizeof ruta, "%s", ruta_asm);
    punto = strrchr(ruta, '.');
    if (punto && strcmp(punto, ".asm") == 0)
        *punto = '\0';
    strncat(ruta, ".esperado", sizeof ruta - strlen(ruta) - 1);

    f = fopen(ruta, "r");
    if (!f) {
        printf("%sFALLA%s %s: no existe %s\n", color_rojo(), color_reset(), ruta_asm, ruta);
        return 0;
    }
    while (fgets(linea, sizeof linea, f)) {
        num++;
        if ((s = strchr(linea, '#')) != NULL)
            *s = '\0';
        s = recortar(linea);
        if (*s == '\0')
            continue;
        igual = strchr(s, '=');
        if (!igual) {
            FALLA("    %s:%d: se esperaba 'nombre = valor'\n", ruta, num);
            continue;
        }
        *igual = '\0';
        lhs = recortar(s);
        quitar_espacios(lhs);                /* "mem [ 0x610 ]" = "mem[0x610]" */
        rhs = recortar(igual + 1);

        if (strcmp(lhs, "excepcion") == 0) {
            espera_excepcion = (strcmp(rhs, "si") == 0);
            continue;
        }
        if (strcmp(lhs, "limite") == 0) {        /* programas que nunca terminan */
            espera_limite = (strcmp(rhs, "si") == 0);
            continue;
        }
        if (!leer_numero(rhs, &v)) {
            FALLA("    %s:%d: valor inválido '%s'\n", ruta, num, rhs);
            continue;
        }
        esperado = (uint32_t)v;
        if ((r = numero_registro(lhs)) >= 0) {
            obtenido = cpu->reg[r];
        } else if (strcmp(lhs, "acc") == 0) {
            obtenido = cpu->acc;
        } else if (strncmp(lhs, "rgb[", 4) == 0) {        /* rgb[x,y]: píxel de la pantalla RGB */
            unsigned x, y;
            if (sscanf(lhs + 4, "%u,%u]", &x, &y) != 2 || x >= RGB_ANCHO || y >= RGB_ALTO) {
                FALLA("    %s:%d: píxel inválido '%s' (use rgb[x,y])\n", ruta, num, lhs);
                continue;
            }
            obtenido = cpu->rgb[y][x];
        } else if (strcmp(lhs, "hi") == 0) {
            obtenido = cpu->hi;
        } else if (strcmp(lhs, "lo") == 0) {
            obtenido = cpu->lo;
        } else if (strcmp(lhs, "ie") == 0) {
            obtenido = cpu->ie;
        } else if (strcmp(lhs, "epc") == 0) {
            obtenido = cpu->epc;
        } else if (strlen(lhs) == 1 && strchr("zncv", lhs[0])) {
            uint32_t bit = lhs[0] == 'z' ? BANDERA_Z : lhs[0] == 'n' ? BANDERA_N :
                           lhs[0] == 'c' ? BANDERA_C : BANDERA_V;
            obtenido = (cpu->banderas & bit) != 0;
        } else if (strcmp(lhs, "pc") == 0) {
            obtenido = cpu->pc;
        } else if (strncmp(lhs, "mem[", 4) == 0 && lhs[strlen(lhs) - 1] == ']') {
            lhs[strlen(lhs) - 1] = '\0';
            if (!leer_numero(lhs + 4, &dir) || dir < 0 || dir > MEM_BYTES - 4 || dir % 4) {
                FALLA("    %s:%d: dirección inválida '%s'\n", ruta, num, lhs + 4);
                continue;
            }
            obtenido = palabra_en(cpu, (uint32_t)dir);
            lhs[strlen(lhs)] = ']';
        } else {
            FALLA("    %s:%d: '%s' no es un registro, acc, pc ni mem[dir]\n", ruta, num, lhs);
            continue;
        }
        total++;
        if (obtenido != esperado)
            FALLA("    %s: esperado 0x%08X (%d), obtenido 0x%08X (%d)\n", lhs,
                  esperado, (int32_t)esperado, obtenido, (int32_t)obtenido);
    }
    fclose(f);

    if (cpu->excepcion[0] && !espera_excepcion)
        FALLA("    excepción: %s\n", cpu->excepcion);
    if (!cpu->excepcion[0] && espera_excepcion)
        FALLA("    se esperaba una excepción y no ocurrió\n");
    if (limite && !espera_limite)
        FALLA("    se alcanzó el límite de pasos (¿ciclo infinito?)\n");
    if (!limite && espera_limite)
        FALLA("    se esperaba que el programa no terminara\n");
#undef FALLA

    if (fallas == 0)
        printf("%sPASA %s %s (%d verificaciones)\n", color_verde(), color_reset(), ruta_asm, total);
    else
        printf("%sFALLA%s %s\n%s", color_rojo(), color_reset(), ruta_asm, fallas_txt);
    return fallas == 0;
}

/* Muestra las primeras líneas de errores del ensamblador, con sangría. */
static void mostrar_errores_resumen(const char *errores, int max)
{
    const char *fin;
    int n = 0;

    while (*errores && n < max) {
        fin = strchr(errores, '\n');
        if (!fin)
            fin = errores + strlen(errores);
        printf("    %.*s\n", (int)(fin - errores), errores);
        errores = *fin ? fin + 1 : fin;
        n++;
    }
    if (*errores)
        printf("    ...\n");
}

/* syscall 5 sin interfaz: lee de programa.entrada (pruebas) o de la entrada estándar. */
static void leer_entero(CPU *cpu, const Opciones *op)
{
    char linea[64], eco[32];
    long long v = 0;

    if (op->modo == MODO_TRAZA)
        printf("  ENTRADA (syscall 5): ");
    fflush(stdout);
    if (op->entrada && fgets(linea, sizeof linea, op->entrada))
        leer_numero(recortar(linea), &v);
    if (op->modo == MODO_TRAZA && op->entrada != stdin)
        printf("%lld\n", v);
    cpu->reg[2] = (uint32_t)v;
    cpu->pide_entero = 0;
    snprintf(eco, sizeof eco, "%lld\n", v);
    consola_agregar(cpu, eco);
}

/* Ejecuta sin interfaz. Devuelve el número de pasos; *limite indica si se agotaron. */
static int ejecutar_sin_interfaz(const ISA *isa, const Programa *prog, CPU *cpu,
                                 const Opciones *op, int *limite)
{
    static CPU antes;
    Instruccion in;
    EstadoPaso r;
    int pasos = 0;

    *limite = 0;
    for (;;) {
        if (pasos >= op->max_pasos && !programa_terminado(prog, cpu)) {
            *limite = 1;
            break;
        }
        antes = *cpu;
        r = ejecutar_paso(isa, prog, cpu, &in);
        if (r == PASO_FIN)
            break;
        pasos++;
        if (cpu->pide_entero)
            leer_entero(cpu, op);
        if (op->modo == MODO_TRAZA)
            mostrar_paso(pasos, &antes, cpu, &in, prog, op->tabla);
        if (r == PASO_EXCEPCION) {
            if (op->modo != MODO_PRUEBA)
                mostrar_error("EXCEPCIÓN en PC = 0x%03X: %s", antes.pc, cpu->excepcion);
            break;
        }
    }
    if (*limite && op->modo != MODO_PRUEBA)
        mostrar_error("Se alcanzó el límite de %d instrucciones (¿ciclo infinito?). Use -n para cambiarlo.",
                      op->max_pasos);
    return pasos;
}

int main(int argc, char **argv)
{
    static ISA isa;
    static CPU cpu, inicial;
    static Programa prog;
    Opciones op;
    int pasos, limite = 0;

    terminal_preparar();
    if (!leer_opciones(argc, argv, &op)) {
        uso(argv[0]);
        return 2;
    }
    configurar_color(op.color);

    if (!cargar_isa(op.isa, &isa))
        return 2;

    if (op.modo == MODO_INTERACTIVO) {
        pasos = tui_ejecutar(&isa, op.programa, op.max_pasos, &prog, &inicial, &cpu);
        if (pasos < 0)
            return 0;                   /* salió sin cargar ningún programa */
        if (cpu.excepcion[0])
            mostrar_error("EXCEPCIÓN: %s", cpu.excepcion);
        mostrar_final(&cpu, &inicial, &prog, pasos);
        return cpu.excepcion[0] ? 1 : 0;
    }

    op.entrada = stdin;
    if (op.modo == MODO_PRUEBA) {                   /* entradas de syscall 5: programa.entrada */
        char ruta[512], *punto;
        snprintf(ruta, sizeof ruta, "%s", op.programa);
        if ((punto = strrchr(ruta, '.')) != NULL && strcmp(punto, ".asm") == 0)
            *punto = '\0';
        strncat(ruta, ".entrada", sizeof ruta - strlen(ruta) - 1);
        op.entrada = fopen(ruta, "r");
    }

    if (!cargar_programa(&isa, op.programa, &prog, &cpu)) {
        if (op.modo == MODO_PRUEBA) {
            printf("%sFALLA%s %s (no ensambla)\n", color_rojo(), color_reset(), op.programa);
            mostrar_errores_resumen(prog.errores, 3);
            return 1;
        }
        fputs(prog.errores, stderr);
        fprintf(stderr, "No se pudo ensamblar '%s'.\n", op.programa);
        return 2;
    }
    inicial = cpu;

    if (op.modo == MODO_TRAZA) {
        mostrar_arquitectura(&prog);
        mostrar_programa(&cpu, &prog);
        mostrar_titulo("Ejecución");
    }
    pasos = ejecutar_sin_interfaz(&isa, &prog, &cpu, &op, &limite);

    if (op.modo == MODO_PRUEBA)
        return verificar(op.programa, &cpu, limite) ? 0 : 1;

    mostrar_final(&cpu, &inicial, &prog, pasos);
    return (cpu.excepcion[0] || limite) ? 1 : 0;
}
