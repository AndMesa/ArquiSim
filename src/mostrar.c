/*
 * mostrar.c -- Presentación del estado de la máquina en la terminal.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mostrar.h"

static int color = 1;

void configurar_color(int activar) { color = activar; }

static const char *c(const char *codigo) { return color ? codigo : ""; }

const char *color_reset(void) { return c("\033[0m"); }
const char *color_rojo(void)  { return c("\033[1;31m"); }
const char *color_verde(void) { return c("\033[1;32m"); }
const char *color_negrita(void)  { return c("\033[1m"); }

/* Temas: colores de los títulos, de lo que cambió y de los valores en cero. */
typedef struct {
    const char *nombre;
    const char *titulo;
    const char *cambio;
    const char *tenue;
} Tema;

static const Tema temas[] = {
    { "sobrio",   "\033[1m",    "\033[1;32m", "\033[2m" },   /* negrilla + verde          */
    { "clasico",  "\033[1;36m", "\033[1;33m", "\033[2m" },   /* cyan + amarillo            */
    { "azul",     "\033[1;34m", "\033[1;32m", "\033[2m" },   /* azul + verde               */
    { "inverso",  "\033[1m",    "\033[7m",    "\033[2m" },   /* sin colores: video inverso */
    { "sin-gris", "\033[1m",    "\033[1;32m", ""        },   /* como sobrio, sin atenuar   */
};
#define NUM_TEMAS ((int)(sizeof temas / sizeof temas[0]))
static int tema = 0;

const char *color_titulo(void) { return c(temas[tema].titulo); }   /* títulos           */
const char *color_cambio(void) { return c(temas[tema].cambio); }   /* lo que cambió     */
const char *color_tenue(void)  { return c(temas[tema].tenue); }    /* ceros y ayudas    */

const char *nombre_tema(void) { return temas[tema].nombre; }

void siguiente_tema(void) { tema = (tema + 1) % NUM_TEMAS; }

int elegir_tema(const char *nombre)
{
    int i;

    for (i = 0; i < NUM_TEMAS; i++)
        if (strcmp(temas[i].nombre, nombre) == 0) {
            tema = i;
            return 1;
        }
    return 0;
}

const char *lista_temas(void) { return "sobrio, clasico, azul, inverso, sin-gris"; }
#define negrita  color_negrita

void mostrar_titulo(const char *titulo)
{
    printf("\n%s== %s ==%s\n", color_titulo(), titulo, color_reset());
}

void mostrar_error(const char *formato, ...)
{
    va_list ap;

    printf("%s", color_rojo());
    va_start(ap, formato);
    vprintf(formato, ap);
    va_end(ap);
    printf("%s\n", color_reset());
}

#define palabra palabra_en

static void mostrar_estadisticas(const CPU *cpu);
static int color_256(uint32_t c);

/* ------------------------------------------------------------------ */

static void segmento(uint32_t inicio, uint32_t tam, const char *nombre, const char *detalle)
{
    if (tam == 0)
        printf("  0x%03X           %-6s %5u B   (vacío)\n", inicio, nombre, 0u);
    else
        printf("  0x%03X - 0x%03X   %-6s %5u B   %s\n", inicio, inicio + tam - 1, nombre, tam, detalle);
}

void mostrar_arquitectura(const Programa *p)
{
    uint32_t t = p->text_fin - TEXT_BASE, d = p->data_fin - DATA_BASE;
    char detalle[48];

    mostrar_titulo("ArquiSim: recursos de la máquina");
    printf("  PC                   32 bits\n");
    printf("  IR                   32 bits\n");
    printf("  Banco de registros   %d registros x 32 bits = %d bytes\n", NUM_REGISTROS, NUM_REGISTROS * 4);
    printf("  ACC                  32 bits (acumulador, fuera del banco)\n");
    printf("  RAM (unificada)      %u bytes = %u palabras de 32 bits, big-endian\n",
           RAM_BYTES, RAM_BYTES / 4);
    printf("  Pantalla (E/S)       %d x %d píxeles, 1 byte por píxel = %d bytes\n",
           VIDEO_ANCHO, VIDEO_ALTO, VIDEO_BYTES);
    printf("  Botones (E/S)        8 botones en la palabra 0x%03X (1 = activado)\n", BOTONES_DIR);

    mostrar_titulo("Mapa de memoria");
    snprintf(detalle, sizeof detalle, "%d instrucciones", p->num_instr);
    segmento(TEXT_BASE, t, ".text", detalle);
    if (TEXT_BASE + t < DATA_BASE)
        segmento(TEXT_BASE + t, DATA_BASE - TEXT_BASE - t, "libre", "");
    snprintf(detalle, sizeof detalle, "%u palabras", d / 4);
    segmento(DATA_BASE, d, ".data", detalle);
    printf("  0x%03X - 0x%03X   %-6s %5u B   libre; la pila crece hacia abajo desde $sp = 0x%03X\n",
           DATA_BASE + d, RAM_BYTES - 1, "pila", RAM_BYTES - DATA_BASE - d, PILA_INICIAL);
    printf("  0x%03X - 0x%03X   %-6s %5u B   memoria de video: píxel (x, y) en 0x%03X + 32*y + x\n",
           VIDEO_BASE, BOTONES_DIR - 1, "video", VIDEO_BYTES, VIDEO_BASE);
    printf("  0x%03X - 0x%03X   %-6s %5u B   botones, temporizador, vector, causa, pantalla RGB\n",
           BOTONES_DIR, MEM_BYTES - 1, "E/S", MEM_BYTES - BOTONES_DIR);
    printf("  El programa ocupa %u B de %u B de RAM (%.1f %%)\n",
           t + d, RAM_BYTES, 100.0 * (t + d) / RAM_BYTES);
}

void mostrar_programa(const CPU *cpu, const Programa *p)
{
    int i;
    uint32_t dir;
    const char *et;
    char etiqueta[MAX_NOMBRE + 2];

    mostrar_titulo("Programa cargado en memoria");
    printf("  %sDir     Código máquina   Línea  Instrucción%s\n", color_tenue(), color_reset());
    for (i = 0; i < p->num_instr; i++) {
        const LineaFuente *lf = &p->fuente[i];
        et = etiqueta_en(p, lf->dir);
        snprintf(etiqueta, sizeof etiqueta, "%s%s", et ? et : "", et ? ":" : "");
        printf("  0x%03X   0x%08X     %5d  %-9s %s\n", lf->dir, palabra(cpu, lf->dir),
               lf->linea, etiqueta, lf->texto);
    }
    if (p->data_fin > DATA_BASE) {
        printf("  %s.data%s\n", color_tenue(), color_reset());
        for (dir = DATA_BASE; dir < p->data_fin; dir += 4) {
            et = etiqueta_en(p, dir);
            printf("  0x%03X   0x%08X            %s%s\n", dir, palabra(cpu, dir),
                   et ? et : "", et ? ":" : "");
        }
    }
}

/* ------------------------------------------------------------------ */

typedef struct {
    int         bits;
    const char *nombre;
    char        valor[32];
} Campo;

int formatear_campos(const Instruccion *in, uint32_t pc, char lineas[3][MAX_CAMPOS])
{
    Campo cs[6];
    int n = 0, i, b, ancho[6], pos = 32;
    size_t k;
    const char *nombres_r[] = { "op", "rs", "rt", "rd", "shamt", "funct" };
    const int bits_r[] = { 6, 5, 5, 5, 5, 6 };

    if (!in->def) {
        snprintf(lineas[0], MAX_CAMPOS, "(el opcode %u no está definido en el ISA)", in->op);
        return 1;
    }

    switch (in->def->formato) {
    case FMT_R:
        for (i = 0; i < 6; i++) {
            cs[i].bits = bits_r[i];
            cs[i].nombre = nombres_r[i];
        }
        snprintf(cs[0].valor, sizeof cs[0].valor, "0x%02X", in->op);
        snprintf(cs[1].valor, sizeof cs[1].valor, "%s", NOMBRES_REG[in->rs]);
        snprintf(cs[2].valor, sizeof cs[2].valor, "%s", NOMBRES_REG[in->rt]);
        snprintf(cs[3].valor, sizeof cs[3].valor, "%s", NOMBRES_REG[in->rd]);
        snprintf(cs[4].valor, sizeof cs[4].valor, "%u", in->shamt);
        snprintf(cs[5].valor, sizeof cs[5].valor, "0x%02X", in->funct);
        n = 6;
        break;
    case FMT_I:
        cs[0].bits = 6;  cs[0].nombre = "op";
        cs[1].bits = 5;  cs[1].nombre = "rs";
        cs[2].bits = 5;  cs[2].nombre = "rt";
        cs[3].bits = 16; cs[3].nombre = "imm";
        snprintf(cs[0].valor, sizeof cs[0].valor, "0x%02X", in->op);
        snprintf(cs[1].valor, sizeof cs[1].valor, "%s", NOMBRES_REG[in->rs]);
        snprintf(cs[2].valor, sizeof cs[2].valor, "%s", NOMBRES_REG[in->rt]);
        if (strstr(in->def->operandos, "label"))
            snprintf(cs[3].valor, sizeof cs[3].valor, "%d -> destino 0x%03X",
                     in->simm, (uint32_t)((int32_t)pc + 4 + in->simm * 4));
        else if (in->simm > -1000 && in->simm < 1000)
            snprintf(cs[3].valor, sizeof cs[3].valor, "%d", in->simm);
        else
            snprintf(cs[3].valor, sizeof cs[3].valor, "0x%04X", in->imm);
        n = 4;
        break;
    case FMT_J:
        cs[0].bits = 6;  cs[0].nombre = "op";
        cs[1].bits = 26; cs[1].nombre = "direccion";
        snprintf(cs[0].valor, sizeof cs[0].valor, "0x%02X", in->op);
        snprintf(cs[1].valor, sizeof cs[1].valor, "0x%07X -> destino 0x%03X",
                 in->direccion, ((pc + 4) & 0xF0000000u) | (in->direccion << 2));
        n = 2;
        break;
    }

    for (i = 0; i < n; i++) {
        ancho[i] = cs[i].bits;
        if ((int)strlen(cs[i].nombre) > ancho[i]) ancho[i] = (int)strlen(cs[i].nombre);
        if ((int)strlen(cs[i].valor) > ancho[i])  ancho[i] = (int)strlen(cs[i].valor);
    }

    /* línea 1: los bits; línea 2: nombre del campo; línea 3: valor */
    k = 0;
    for (i = 0; i < n; i++) {
        pos -= cs[i].bits;
        for (b = cs[i].bits - 1; b >= 0; b--)
            lineas[0][k++] = (in->binario >> (pos + b)) & 1 ? '1' : '0';
        for (b = cs[i].bits; b <= ancho[i]; b++)
            lineas[0][k++] = ' ';
    }
    lineas[0][k] = '\0';
    k = (size_t)snprintf(lineas[1], MAX_CAMPOS, "%s", color_tenue());
    for (i = 0; i < n; i++)
        k += (size_t)snprintf(lineas[1] + k, MAX_CAMPOS - k, "%-*s ", ancho[i], cs[i].nombre);
    snprintf(lineas[1] + k, MAX_CAMPOS - k, "%s", color_reset());
    k = 0;
    for (i = 0; i < n; i++)
        k += (size_t)snprintf(lineas[2] + k, MAX_CAMPOS - k, "%-*s ", ancho[i], cs[i].valor);
    return 3;
}

static void mostrar_campos(const Instruccion *in, uint32_t pc)
{
    char lineas[3][MAX_CAMPOS];
    int i, n = formatear_campos(in, pc, lineas);

    for (i = 0; i < n; i++)
        printf("       %s\n", lineas[i]);
}

static void mostrar_cambios(const CPU *antes, const CPU *despues)
{
    int r;
    uint32_t dir, a, d;

    for (r = 0; r < NUM_REGISTROS; r++) {
        if (antes->reg[r] != despues->reg[r])
            printf("  %-5s  0x%08X -> %s0x%08X%s  (%d)\n", NOMBRES_REG[r], antes->reg[r],
                   color_cambio(), despues->reg[r], color_reset(), (int32_t)despues->reg[r]);
    }
    if (strcmp(antes->consola, despues->consola) != 0) {
        size_t la = strlen(antes->consola), ld = strlen(despues->consola);
        const char *nuevo = (ld > la && strncmp(antes->consola, despues->consola, la) == 0)
                            ? despues->consola + la : despues->consola;
        printf("  CONSOLA  %s\"", color_cambio());
        for (; *nuevo; nuevo++)
            fputs(*nuevo == '\n' ? "\\n" : (char[2]){ *nuevo, 0 }, stdout);
        printf("\"%s\n", color_reset());
    }
    if (despues->int_atendida)
        printf("  %sINTERRUPCIÓN%s (%s%s): EPC = 0x%03X, salto a la ISR en 0x%03X\n", color_cambio(),
               color_reset(), despues->int_atendida & INT_BOTONES ? "botones " : "",
               despues->int_atendida & INT_TEMPORIZADOR ? "temporizador" : "", despues->epc, despues->pc);
    if (antes->banderas != despues->banderas) {
        char b[128];
        formatear_banderas(despues, antes, b, sizeof b);
        printf("  %-5s  %s\n", "FLAGS", b);
    }
    if (antes->hi != despues->hi)
        printf("  %-5s  0x%08X -> %s0x%08X%s  (%d)\n", "HI", antes->hi, color_cambio(), despues->hi,
               color_reset(), (int32_t)despues->hi);
    if (antes->lo != despues->lo)
        printf("  %-5s  0x%08X -> %s0x%08X%s  (%d)\n", "LO", antes->lo, color_cambio(), despues->lo,
               color_reset(), (int32_t)despues->lo);
    if (antes->acc != despues->acc)
        printf("  %-5s  0x%08X -> %s0x%08X%s  (%d)\n", "ACC", antes->acc, color_cambio(), despues->acc,
               color_reset(), (int32_t)despues->acc);
    printf("  %-5s  0x%03X      -> %s0x%03X%s", "PC", antes->pc, color_cambio(), despues->pc, color_reset());
    if (despues->pc != antes->pc + 4)
        printf("       %s(salto)%s", negrita(), color_reset());
    printf("\n");
    for (dir = 0; dir < MEM_BYTES; dir += 4) {
        a = palabra(antes, dir);
        d = palabra(despues, dir);
        if (a != d)
            printf("  MEM[0x%03X]  0x%08X -> %s0x%08X%s  (%d)\n", dir, a, color_cambio(), d,
                   color_reset(), (int32_t)d);
    }
}

void mostrar_paso(int paso, const CPU *antes, const CPU *despues,
                  const Instruccion *in, const Programa *p, int con_tabla)
{
    const LineaFuente *lf = buscar_fuente(p, antes->pc);

    printf("%s------------------------------------------------------------------------%s\n",
           color_tenue(), color_reset());
    printf("%sPaso %d%s   PC = 0x%03X   %s%s%s", negrita(), paso, color_reset(), antes->pc,
           color_titulo(), lf ? lf->texto : "?", color_reset());
    if (lf)
        printf("   %s(línea %d)%s", color_tenue(), lf->linea, color_reset());
    printf("\n");
    printf("  IR = 0x%08X", in->binario);
    if (in->def)
        printf("   formato %c", "RIJ"[in->def->formato]);
    printf("\n");
    mostrar_campos(in, antes->pc);
    printf("  A = 0x%08X   B = 0x%08X   TEMP = 0x%08X\n", despues->a, despues->b, despues->temp);
    mostrar_cambios(antes, despues);
    if (con_tabla)
        mostrar_registros(despues, antes);
}

void mostrar_registros(const CPU *cpu, const CPU *ref)
{
    int fila, col, r;
    const char *estilo;

    printf("  %sBanco de registros (32 x 32 bits)%s\n", color_tenue(), color_reset());
    for (fila = 0; fila < 8; fila++) {
        printf(" ");
        for (col = 0; col < 4; col++) {
            r = col * 8 + fila;
            if (ref && ref->reg[r] != cpu->reg[r])
                estilo = color_cambio();
            else if (cpu->reg[r] == 0)
                estilo = color_tenue();
            else
                estilo = "";
            printf(" %s%-5s 0x%08X%s  ", estilo, NOMBRES_REG[r], cpu->reg[r], color_reset());
        }
        printf("\n");
    }
    char b[128];

    formatear_banderas(cpu, ref, b, sizeof b);
    printf("  PC 0x%03X   IR 0x%08X   %sACC 0x%08X%s   FLAGS %s\n", cpu->pc, cpu->ir,
           ref && ref->acc != cpu->acc ? color_cambio() : "", cpu->acc, color_reset(), b);
    printf("  A 0x%08X   B 0x%08X   TEMP 0x%08X   HI 0x%08X   LO 0x%08X   IE %u   EPC 0x%03X\n",
           cpu->a, cpu->b, cpu->temp, cpu->hi, cpu->lo, cpu->ie, cpu->epc);
}

static void encabezado_memoria(void)
{
    printf("  %sDir     Bytes (big-endian)   Palabra          Decimal%s\n", color_tenue(), color_reset());
}

static void fila_memoria(const CPU *cpu, const Programa *p, uint32_t dir)
{
    uint32_t w = palabra(cpu, dir);
    const char *et = etiqueta_en(p, dir);

    printf("  0x%03X   %02X %02X %02X %02X          0x%08X  %11d  %s%s\n", dir,
           cpu->ram[dir], cpu->ram[dir + 1], cpu->ram[dir + 2], cpu->ram[dir + 3],
           w, (int32_t)w, et ? et : "", et ? ":" : "");
}

void mostrar_memoria(const CPU *cpu, const Programa *p, uint32_t dir, int palabras)
{
    int i;

    encabezado_memoria();
    for (i = 0, dir &= ~3u; i < palabras && dir < MEM_BYTES; i++, dir += 4)
        fila_memoria(cpu, p, dir);
}

void mostrar_final(const CPU *cpu, const CPU *inicial, const Programa *p, int pasos)
{
    char titulo[64];
    uint32_t dir;
    int cambios = 0;

    snprintf(titulo, sizeof titulo, "Estado final (%d instrucciones ejecutadas)", pasos);
    mostrar_titulo(titulo);
    mostrar_registros(cpu, inicial);

    mostrar_titulo("Memoria modificada por el programa");
    for (dir = 0; dir < MEM_BYTES; dir += 4) {
        if (palabra(cpu, dir) != palabra(inicial, dir)) {
            if (cambios++ == 0)
                encabezado_memoria();
            fila_memoria(cpu, p, dir);
        }
    }
    if (cambios == 0)
        printf("  (ninguna)\n");

    mostrar_estadisticas(cpu);

    if (cpu->consola[0]) {
        mostrar_titulo("Consola");
        printf("%s%s", cpu->consola, cpu->consola[strlen(cpu->consola) - 1] == '\n' ? "" : "\n");
    }

    if (rgb_usada(cpu)) {
        char fila[MAX_FILA_PANTALLA * 4];
        int y;

        mostrar_titulo("Pantalla RGB (registros 0x610 POSICION, 0x614 COLOR)");
        for (y = 0; y < RGB_ALTO; y += color ? 2 : 1) {
            if (color)
                formatear_dos_filas_rgb(cpu, y, 2, fila, sizeof fila);
            else
                formatear_fila_rgb(cpu, y, 2, fila, sizeof fila);
            printf("  %2d %s\n", y, fila);
        }
    }

    if (pantalla_usada(cpu)) {
        char fila[MAX_FILA_PANTALLA];
        int y;

        mostrar_titulo("Pantalla (0x400 - 0x5FF)");
        for (y = 0; y < VIDEO_ALTO; y++) {
            formatear_fila_pantalla(cpu, y, 2, fila, sizeof fila);
            printf("  %2d %s\n", y, fila);
        }
    }
}

void formatear_banderas(const CPU *cpu, const CPU *ref, char *buf, size_t tam)
{
    static const char nombres[] = "ZNCV";
    static const uint32_t bits[] = { BANDERA_Z, BANDERA_N, BANDERA_C, BANDERA_V };
    size_t k = 0;
    int i, on, cambio;

    buf[0] = '\0';
    for (i = 0; i < 4 && k < tam; i++) {
        on = (cpu->banderas & bits[i]) != 0;
        cambio = ref && ((ref->banderas ^ cpu->banderas) & bits[i]);
        k += (size_t)snprintf(buf + k, tam - k, "%s%s%c %d%s", i ? "  " : "",
                              cambio ? color_cambio() : on ? "" : color_tenue(),
                              nombres[i], on, color_reset());
    }
}

void formatear_estadisticas(const CPU *cpu, char *buf, size_t tam)
{
    snprintf(buf, tam, "R %u  I %u  J %u | lect. %u  escr. %u | saltos %u | int. %u",
             cpu->est.instr[FMT_R], cpu->est.instr[FMT_I], cpu->est.instr[FMT_J],
             cpu->est.lecturas, cpu->est.escrituras, cpu->est.saltos, cpu->est.interrupciones);
}

static void mostrar_estadisticas(const CPU *cpu)
{
    const Estadisticas *e = &cpu->est;
    uint32_t total = e->instr[0] + e->instr[1] + e->instr[2];
    double t = total ? (double)total : 1.0;

    mostrar_titulo("Estadísticas");
    printf("  Instrucciones ejecutadas   %u   (R %u = %.0f %%,  I %u = %.0f %%,  J %u = %.0f %%)\n", total,
           e->instr[0], 100.0 * e->instr[0] / t, e->instr[1], 100.0 * e->instr[1] / t,
           e->instr[2], 100.0 * e->instr[2] / t);
    printf("  Accesos a datos            %u lecturas, %u escrituras\n", e->lecturas, e->escrituras);
    printf("  Saltos tomados             %u\n", e->saltos);
    printf("  Interrupciones atendidas   %u\n", e->interrupciones);
}

int rgb_usada(const CPU *cpu)
{
    int x, y;

    for (y = 0; y < RGB_ALTO; y++)
        for (x = 0; x < RGB_ANCHO; x++)
            if (cpu->rgb[y][x])
                return 1;
    return 0;
}

/* Color de 24 bits: truecolor si la terminal lo anuncia (COLORTERM); si no, el
 * color más cercano de la paleta de 256 (cubo 6 x 6 x 6). Sin color: '#' o '.'. */
void formatear_fila_rgb(const CPU *cpu, int y, int px, char *buf, size_t tam)
{
    const char *ct = getenv("COLORTERM");
    int truecolor = ct && (strstr(ct, "truecolor") || strstr(ct, "24bit"));
    uint32_t c, r, g, b, previo = 0xFFFFFFFF;
    size_t k = 0;
    int x;

    buf[0] = '\0';
    for (x = 0; x < RGB_ANCHO && k + 32 < tam; x++) {
        c = cpu->rgb[y][x];
        if (!color) {
            k += (size_t)snprintf(buf + k, tam - k, "%s", c ? (px == 2 ? "##" : "#") : (px == 2 ? ". " : "."));
            continue;
        }
        if (c != previo) {
            r = c >> 16;
            g = (c >> 8) & 0xFF;
            b = c & 0xFF;
            if (truecolor)
                k += (size_t)snprintf(buf + k, tam - k, "\033[48;2;%u;%u;%um", r, g, b);
            else
                k += (size_t)snprintf(buf + k, tam - k, "\033[48;5;%dm", color_256(c));
            previo = c;
        }
        k += (size_t)snprintf(buf + k, tam - k, "%s", px == 2 ? "  " : " ");
    }
    if (color)
        snprintf(buf + k, tam - k, "\033[0m");
}

static int color_256(uint32_t c)
{
    return 16 + 36 * (int)(((c >> 16) * 5 + 127) / 255) + 6 * (int)((((c >> 8) & 0xFF) * 5 + 127) / 255) +
           (int)(((c & 0xFF) * 5 + 127) / 255);
}

void formatear_dos_filas_rgb(const CPU *cpu, int y, int px, char *buf, size_t tam)
{
    const char *ct = getenv("COLORTERM");
    int truecolor = ct && (strstr(ct, "truecolor") || strstr(ct, "24bit"));
    uint32_t arriba, abajo, previo_a = 0xFFFFFFFF, previo_b = 0xFFFFFFFF;
    size_t k = 0;
    int x;

    buf[0] = '\0';
    for (x = 0; x < RGB_ANCHO && k + 64 < tam; x++) {
        arriba = cpu->rgb[y][x];
        abajo = y + 1 < RGB_ALTO ? cpu->rgb[y + 1][x] : 0;
        if (arriba != previo_a || abajo != previo_b) {
            if (truecolor)
                k += (size_t)snprintf(buf + k, tam - k, "\033[38;2;%u;%u;%u;48;2;%u;%u;%um",
                                      arriba >> 16, (arriba >> 8) & 0xFF, arriba & 0xFF,
                                      abajo >> 16, (abajo >> 8) & 0xFF, abajo & 0xFF);
            else
                k += (size_t)snprintf(buf + k, tam - k, "\033[38;5;%d;48;5;%dm", color_256(arriba), color_256(abajo));
            previo_a = arriba;
            previo_b = abajo;
        }
        k += (size_t)snprintf(buf + k, tam - k, "%s", px == 2 ? "▀▀" : "▀");
    }
    snprintf(buf + k, tam - k, "\033[0m");
}

int pantalla_usada(const CPU *cpu)
{
    int i;

    for (i = 0; i < VIDEO_BYTES; i++)
        if (cpu->ram[VIDEO_BASE + i])
            return 1;
    return 0;
}

/* Cada píxel ocupa px caracteres (1 o 2). Con color se pinta el fondo con el color ANSI;
 * sin color se escribe el número de color en hexadecimal ('.' para negro). */
void formatear_fila_pantalla(const CPU *cpu, int y, int px, char *buf, size_t tam)
{
    const uint8_t *fila = &cpu->ram[VIDEO_BASE + (uint32_t)(y * VIDEO_ANCHO)];
    size_t k = 0;
    int x, c, previo = -1;

    buf[0] = '\0';
    for (x = 0; x < VIDEO_ANCHO && k + 16 < tam; x++) {
        c = fila[x] & 0x0F;
        if (color) {
            if (c != previo)
                k += (size_t)snprintf(buf + k, tam - k, "\033[%dm", c < 8 ? 40 + c : 100 + c - 8);
            k += (size_t)snprintf(buf + k, tam - k, "%s", px == 2 ? "  " : " ");
            previo = c;
        } else if (c == 0) {
            k += (size_t)snprintf(buf + k, tam - k, "%s", px == 2 ? ". " : ".");
        } else {
            k += (size_t)snprintf(buf + k, tam - k, px == 2 ? "%X%X" : "%X", c, c);
        }
    }
    if (color)
        snprintf(buf + k, tam - k, "\033[0m");
}
