/*
 * tui.c -- Interfaz de pantalla completa en ASCII. La pantalla se redibuja en cada paso:
 *
 *   +--------------------------------------------------------------------------+
 *   | ArquiSim    programa.asm   paso 3   PC = 0x00C                           |
 *   +-- PROGRAMA (.text) ---------------------+-- BANCO DE REGISTROS ----------+
 *   | PC> 0x00C  0110482A  LOOP: slt ...       | $zero 00000000  $s0  00000200 |
 *   +-- ÚLTIMA INSTRUCCIÓN (IR) --------------+ ...                            |
 *   +-- CAMBIOS EN ESTE PASO -----------------+-- MEMORIA RAM -----------------+
 *   +-- MAPA DE MEMORIA ----------------------+ 0x200 = 00 00 00 01  A         |
 *   +-----------------------------------------+--------------------------------+
 *   | estado                                                                   |
 *   | comandos                                                                 |
 *   +--------------------------------------------------------------------------+
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tui.h"
#include "ciclo.h"
#include "mostrar.h"
#include "terminal.h"
#include "sistema.h"

#define ANCHO_DER     46        /* columna derecha: registros y memoria  */
#define ANCHO_IZQ_MIN 58
#define ANCHO_IZQ_MAX 92
#define ALTO_MIN      31        /* filas del cuerpo                      */
#define ALTO_MAX      80
#define FILAS_FIJAS   10        /* bordes, encabezado, estado, separador, 3 de comandos, prompt */
#define FIJAS_IZQ     16        /* separadores + IR + cambios + mapa     */
#define FIJAS_DER     26        /* 4 separadores + 16 registros + 4 internos + 2 de botones */
#define MAX_LINEA     2048
#define MAX_FILAS     (ALTO_MAX + 4)
#define ANIMACION_MS  400
#define HIST_MAX      5000      /* estados guardados para retroceder con b      */
#define HIST_DESCARTE 1000      /* al llenarse, se olvidan los más antiguos     */
#define REFRESCO_MS   40        /* en tiempo real: redibujar a lo sumo 25 veces/s */

/* ------------------------------------------------------------------ */
/* Paneles: listas de líneas que luego se dibujan lado a lado          */
/* ------------------------------------------------------------------ */

typedef struct {
    char t[MAX_LINEA];
    int  sep;           /* 1 = línea separadora con título */
} Linea;

typedef struct {
    Linea l[MAX_FILAS];
    int   n;
} Panel;

static void agregar(Panel *p, int sep, const char *formato, ...)
{
    va_list ap;

    if (p->n >= MAX_FILAS)
        return;
    va_start(ap, formato);
    vsnprintf(p->l[p->n].t, MAX_LINEA, formato, ap);
    va_end(ap);
    p->l[p->n].sep = sep;
    p->n++;
}

#define LINEA(p, ...)         agregar(p, 0, __VA_ARGS__)
#define SEPARADOR(p, titulo)  agregar(p, 1, "%s", titulo)

/* Ancho visible: ignora los códigos de color y cuenta caracteres UTF-8. */
static int ancho_visible(const char *s)
{
    int n = 0;

    while (*s) {
        if (*s == '\033') {
            while (*s && *s != 'm')
                s++;
            if (*s)
                s++;
            continue;
        }
        if (((unsigned char)*s & 0xC0) != 0x80)
            n++;
        s++;
    }
    return n;
}

/* Escribe s recortado o rellenado con espacios hasta 'ancho' caracteres visibles. */
static void emitir(const char *s, int ancho)
{
    int n = 0, hubo_color = 0;
    const char *fin;

    while (*s) {
        if (*s == '\033') {
            for (fin = s; *fin && *fin != 'm'; fin++)
                ;
            if (*fin)
                fin++;
            fwrite(s, 1, (size_t)(fin - s), stdout);
            hubo_color = 1;
            s = fin;
            continue;
        }
        if (((unsigned char)*s & 0xC0) != 0x80) {
            if (n == ancho)
                break;
            n++;
        }
        putchar(*s++);
    }
    if (hubo_color)
        fputs(color_reset(), stdout);
    for (; n < ancho; n++)
        putchar(' ');
}

/* Caracteres de los bordes: dibujo de cajas (Unicode) o ASCII simple (--ascii). */
typedef struct {
    const char *h, *v;                      /* línea horizontal y vertical      */
    const char *sup_izq, *sup_der;          /* esquinas superiores              */
    const char *inf_izq, *inf_der;          /* esquinas inferiores              */
    const char *t_izq, *t_der;              /* uniones en T a izquierda/derecha */
    const char *t_arr, *t_abj, *cruz;       /* uniones en T arriba/abajo, cruz  */
    const char *tit_ini, *tit_fin;          /* marco del título de una ventana  */
} Bordes;

static const Bordes BORDES_UNICODE = { "─", "│", "┌", "┐", "└", "┘", "├", "┤", "┬", "┴", "┼", "┤ ", " ├" };
static const Bordes BORDES_ASCII   = { "-", "|", "+", "+", "+", "+", "+", "+", "+", "+", "+", "- ", " " };
static const Bordes *B = &BORDES_UNICODE;

void tui_usar_ascii(int activar) { B = activar ? &BORDES_ASCII : &BORDES_UNICODE; }

static void repetir(const char *s, int n)
{
    while (n-- > 0)
        fputs(s, stdout);
}

static void emitir_celda(const Linea *l, int ancho)
{
    char buf[MAX_LINEA];
    int n;

    if (!l->sep) {
        putchar(' ');
        emitir(l->t, ancho - 1);
        return;
    }
    /* título de ventana:  ─┤ TÍTULO ├──────── */
    snprintf(buf, sizeof buf, "%s%s%s%s%s%s", B->h, B->tit_ini, color_titulo(), l->t, color_reset(), B->tit_fin);
    n = ancho_visible(buf);
    emitir(buf, n < ancho ? n : ancho);
    repetir(B->h, ancho - n);
}

/* Línea horizontal completa, con una unión opcional en la columna union_col. */
static void linea_horizontal(const char *izq, const char *der, int ancho_total,
                             int union_col, const char *union_ch)
{
    fputs(izq, stdout);
    if (union_col >= 0) {
        repetir(B->h, union_col);
        fputs(union_ch, stdout);
        repetir(B->h, ancho_total - union_col - 1);
    } else {
        repetir(B->h, ancho_total);
    }
    fputs(der, stdout);
    fputs("\033[K\n", stdout);
}

static void linea_titulo(const char *titulo, int ancho_total)
{
    Linea l;

    l.sep = 1;
    snprintf(l.t, sizeof l.t, "%s", titulo);
    fputs(B->sup_izq, stdout);
    emitir_celda(&l, ancho_total);
    fputs(B->sup_der, stdout);
    fputs("\033[K\n", stdout);
}

static void fila_completa(const char *texto, int ancho_total)
{
    fputs(B->v, stdout);
    putchar(' ');
    emitir(texto, ancho_total - 1);
    fputs(B->v, stdout);
    fputs("\033[K\n", stdout);
}

/* ------------------------------------------------------------------ */
/* Estado de la sesión                                                 */
/* ------------------------------------------------------------------ */

typedef struct {
    CPU         cpu;
    Instruccion in;     /* instrucción que llevó a este estado */
} Estado;

typedef struct {
    const ISA      *isa;
    Programa       *p;          /* programa cargado (memoria del llamador)        */
    CPU            *inicial;    /* estado inicial del programa cargado             */
    int             max_pasos;
    Estado         *hist;       /* hist[0] = estado inicial; hist[k] = después de k pasos */
    int             cap;
    int             k;
    uint32_t        mem_inicio; /* primera dirección visible en la ventana de memoria */
    char            mensaje[160];
    int             ancho_izq, alto, columnas, filas;
    int             aire;           /* 1 = línea en blanco bajo cada título (si hay espacio) */
    int             ver_pantalla;   /* panel izquierdo: 0 programa, 1 pantalla, 2 pantalla RGB */
    int             pantalla_auto;  /* ya se cambió automáticamente a la pantalla   */
    int             rgb_auto;       /* ya se cambió automáticamente a la RGB        */
    int             consola_auto;   /* ya se cambió automáticamente a la consola    */
    unsigned char   bp[MAX_INSTR_PROGRAMA];   /* puntos de parada, por instrucción  */
    int             base;           /* pasos olvidados del historial: paso = base + k   */
    CPU             cero;           /* estado del paso 0 (si ya se olvidó del historial) */
    int             corriendo;      /* ejecución en tiempo real (comando a)             */
    int             ms;             /* milisegundos por instrucción en tiempo real      */
    Historial       historial;      /* comandos anteriores (flecha arriba / abajo)      */
} Sesion;

static const char *nombre_programa(const Sesion *s)
{
    const char *n = strrchr(s->p->ruta, '/');
#ifdef _WIN32
    const char *w = strrchr(s->p->ruta, '\\');
    if (w && (!n || w > n))
        n = w;
#endif
    return n ? n + 1 : s->p->ruta;
}

static void calcular_tamano(Sesion *s)
{
    int columnas, filas;

    terminal_tamano(&columnas, &filas);
    s->columnas = columnas;
    s->filas = filas;
    s->ancho_izq = columnas - ANCHO_DER - 3;
    if (s->ancho_izq < ANCHO_IZQ_MIN) s->ancho_izq = ANCHO_IZQ_MIN;
    if (s->ancho_izq > ANCHO_IZQ_MAX) s->ancho_izq = ANCHO_IZQ_MAX;
    s->alto = filas - FILAS_FIJAS;
    if (s->alto < ALTO_MIN) s->alto = ALTO_MIN;
    if (s->alto > ALTO_MAX) s->alto = ALTO_MAX;
    s->aire = s->alto - 2 >= ALTO_MIN + 8;
    if (s->aire)
        s->alto -= 2;
}

/* El panel de memoria se divide en dos ventanas: datos (arriba) y pila (abajo). */
static int es_video(uint32_t dir) { return dir >= VIDEO_BASE && dir < BOTONES_DIR; }

static int filas_memoria(const Sesion *s) { return s->alto - FIJAS_DER - 9 * s->aire; }
static int filas_pila(const Sesion *s)    { int f = filas_memoria(s) / 3; return f < 2 ? 2 : f; }
static int filas_datos(const Sesion *s)   { return filas_memoria(s) - filas_pila(s); }

static uint32_t inicio_pila(const Sesion *s, const CPU *cpu)
{
    uint32_t n = 4u * (uint32_t)filas_pila(s), sp = cpu->reg[REG_SP] & ~3u;
    uint32_t ini = sp >= 4 ? sp - 4 : 0;        /* una palabra libre debajo de $sp */

    if (ini > RAM_BYTES - n)
        ini = RAM_BYTES - n;
    return ini;
}

static void ajustar_memoria(Sesion *s)
{
    uint32_t max = MEM_BYTES - 4u * (uint32_t)filas_datos(s);

    s->mem_inicio &= ~3u;
    if (s->mem_inicio > max)
        s->mem_inicio = max;
}

/* ------------------------------------------------------------------ */
/* Panel izquierdo                                                     */
/* ------------------------------------------------------------------ */

static int indice_instr(const Programa *p, uint32_t dir)
{
    int i;

    if (dir == p->text_fin)
        return p->num_instr;
    for (i = 0; i < p->num_instr; i++)
        if (p->fuente[i].dir == dir)
            return i;
    return -1;
}

static void panel_programa(Panel *P, const Programa *p, const CPU *cpu, const CPU *antes, int filas,
                           const unsigned char *bp)
{
    int total = p->num_instr + 1;   /* + marca de fin */
    int actual = indice_instr(p, cpu->pc);
    int ultima = antes ? indice_instr(p, antes->pc) : -1;
    int inicio, i, ancho_et = 0, len;
    const char *et, *estilo, *marca;
    char etiqueta[MAX_NOMBRE + 2];

    for (i = 0; i < p->num_instr; i++)
        if ((et = etiqueta_en(p, p->fuente[i].dir)) != NULL && (len = (int)strlen(et) + 1) > ancho_et)
            ancho_et = len;

    inicio = (actual >= 0 ? actual : 0) - filas / 3;
    if (inicio > total - filas) inicio = total - filas;
    if (inicio < 0) inicio = 0;

    for (i = inicio; i < inicio + filas; i++) {
        if (i >= total) {
            LINEA(P, "");
            continue;
        }
        marca  = (i == actual) ? "PC>" : "   ";
        estilo = (i == actual) ? color_cambio() : (i == ultima) ? color_titulo() : "";
        if (i == p->num_instr) {
            LINEA(P, "%s%s   0x%03X  --------  %*s (fin del programa)%s", estilo, marca,
                  p->text_fin, ancho_et, "", color_reset());
            continue;
        }
        et = etiqueta_en(p, p->fuente[i].dir);
        snprintf(etiqueta, sizeof etiqueta, "%s%s", et ? et : "", et ? ":" : "");
        LINEA(P, "%s%s %s%c%s%s 0x%03X  %08X  %-*s %s%s", estilo, marca,
              bp[p->fuente[i].dir / 4] ? color_rojo() : "", bp[p->fuente[i].dir / 4] ? 'o' : ' ',
              color_reset(), estilo, p->fuente[i].dir,
              palabra_en(cpu, p->fuente[i].dir), ancho_et, etiqueta, p->fuente[i].texto, color_reset());
    }
}

static void panel_ir(Panel *P, const Programa *p, const CPU *antes, const Instruccion *in)
{
    char campos[3][MAX_CAMPOS];
    const LineaFuente *lf;
    int n, i;

    if (!antes) {
        LINEA(P, "%s(todavía no se ha ejecutado ninguna instrucción)%s", color_tenue(), color_reset());
        LINEA(P, "");
        LINEA(P, "");
        LINEA(P, "");
        return;
    }
    lf = buscar_fuente(p, antes->pc);
    if (in->def)
        LINEA(P, "IR = 0x%08X   formato %c   %s%s%s", in->binario, "RIJ"[in->def->formato],
              color_titulo(), lf ? lf->texto : "?", color_reset());
    else
        LINEA(P, "IR = 0x%08X", in->binario);
    n = formatear_campos(in, antes->pc, campos);
    for (i = 0; i < 3; i++)
        LINEA(P, "%s", i < n ? campos[i] : "");
}

static void panel_cambios(Panel *P, const CPU *antes, const CPU *cpu, int filas)
{
    char cambios[NUM_REGISTROS + 2 + MEM_BYTES / 4][96];
    int n = 0, r, i;
    uint32_t dir, a, d;

    if (!antes) {
        LINEA(P, "%s(ninguno)%s", color_tenue(), color_reset());
        for (i = 1; i < filas; i++)
            LINEA(P, "");
        return;
    }
    for (r = 0; r < NUM_REGISTROS; r++)
        if (antes->reg[r] != cpu->reg[r])
            snprintf(cambios[n++], sizeof cambios[0], "%-9s 0x%08X -> %s0x%08X%s  (%d)",
                     NOMBRES_REG[r], antes->reg[r], color_cambio(), cpu->reg[r], color_reset(),
                     (int32_t)cpu->reg[r]);
    if (antes->banderas != cpu->banderas) {
        char b[96];
        formatear_banderas(cpu, antes, b, sizeof b);
        snprintf(cambios[n++], sizeof cambios[0], "%-9s %s", "FLAGS", b);
    }
    if (antes->hi != cpu->hi)
        snprintf(cambios[n++], sizeof cambios[0], "%-9s 0x%08X -> %s0x%08X%s  (%d)", "HI",
                 antes->hi, color_cambio(), cpu->hi, color_reset(), (int32_t)cpu->hi);
    if (antes->lo != cpu->lo)
        snprintf(cambios[n++], sizeof cambios[0], "%-9s 0x%08X -> %s0x%08X%s  (%d)", "LO",
                 antes->lo, color_cambio(), cpu->lo, color_reset(), (int32_t)cpu->lo);
    if (antes->acc != cpu->acc)
        snprintf(cambios[n++], sizeof cambios[0], "%-9s 0x%08X -> %s0x%08X%s  (%d)", "ACC",
                 antes->acc, color_cambio(), cpu->acc, color_reset(), (int32_t)cpu->acc);
    snprintf(cambios[n++], sizeof cambios[0], "%-9s 0x%03X      -> %s0x%03X%s%s", "PC",
             antes->pc, color_cambio(), cpu->pc, color_reset(),
             cpu->pc != antes->pc + 4 ? "       (salto)" : "");
    for (dir = 0; dir < MEM_BYTES; dir += 4) {
        a = palabra_en(antes, dir);
        d = palabra_en(cpu, dir);
        if (a != d)
            snprintf(cambios[n++], sizeof cambios[0], "MEM[0x%03X] 0x%08X -> %s0x%08X%s  (%d)",
                     dir, a, color_cambio(), d, color_reset(), (int32_t)d);
    }
    for (i = 0; i < filas; i++) {
        if (i == filas - 1 && n > filas)
            LINEA(P, "%s... y %d cambios más%s", color_tenue(), n - filas + 1, color_reset());
        else
            LINEA(P, "%s", i < n ? cambios[i] : "");
    }
}

/* Barra ASCII de la RAM: cada carácter representa un bloque de bytes. */
static char tipo_bloque(const Programa *p, const CPU *cpu, uint32_t ini, uint32_t fin)
{
    if (ini >= BOTONES_DIR)                        return 'e';
    if (ini >= VIDEO_BASE)                         return 'v';
    if (ini < p->text_fin)                         return '#';
    if (fin > DATA_BASE && ini < p->data_fin)      return '=';
    if (fin > cpu->reg[REG_SP] && ini < PILA_INICIAL && cpu->reg[REG_SP] < PILA_INICIAL)
                                                   return 's';
    return '.';
}

static void panel_mapa(Panel *P, const Programa *p, const CPU *cpu, int ancho)
{
    char barra[MAX_LINEA];
    int bloques = (ancho - 12 >= 64) ? 64 : 32;
    uint32_t tam = RAM_BYTES / (uint32_t)bloques, t = p->text_fin - TEXT_BASE, d = p->data_fin - DATA_BASE;
    uint32_t pila = cpu->reg[REG_SP] < PILA_INICIAL ? PILA_INICIAL - cpu->reg[REG_SP] : 0;
    size_t k;
    int i;
    char ch, previo = 0;
    const char *estilo;

    k = (size_t)snprintf(barra, sizeof barra, "RAM [");
    for (i = 0; i < bloques; i++) {
        ch = tipo_bloque(p, cpu, (uint32_t)i * tam, (uint32_t)(i + 1) * tam);
        if (ch != previo) {
            estilo = ch == '#' ? color_negrita() : ch == '=' ? color_verde() :
                     ch == 's' ? "" : color_tenue();
            k += (size_t)snprintf(barra + k, sizeof barra - k, "%s%s", color_reset(), estilo);
            previo = ch;
        }
        barra[k++] = ch;
    }
    snprintf(barra + k, sizeof barra - k, "%s] %u B/car", color_reset(), tam);
    LINEA(P, "%s", barra);
    snprintf(barra, sizeof barra, d ? "0x%03X-0x%03X" : "(vacío)", DATA_BASE, p->data_fin - 1);
    LINEA(P, "     # .text 0x%03X-0x%03X  = .data %s  s pila  v video  e E/S",
          TEXT_BASE, t ? p->text_fin - 1 : TEXT_BASE, barra);
    LINEA(P, "PC, IR, ACC 32 b | registros 128 B | RAM %u B | video 0x%03X | E/S 0x%03X",
          RAM_BYTES, VIDEO_BASE, BOTONES_DIR);
    LINEA(P, "Programa: %u B de %u B (%.1f %%)   .text %u B   .data %u B   pila %u B",
          t + d, RAM_BYTES, 100.0 * (t + d) / RAM_BYTES, t, d, pila);
}

/* ------------------------------------------------------------------ */
/* Panel derecho                                                       */
/* ------------------------------------------------------------------ */

static void celda_registro(char *buf, size_t tam, const CPU *cpu, const CPU *antes, int r)
{
    int cambio = antes && antes->reg[r] != cpu->reg[r];
    const char *estilo = cambio ? color_cambio() : cpu->reg[r] == 0 ? color_tenue() : "";

    /* la dirección de 5 bits del registro: la misma que aparece en los campos rs, rt, rd */
    snprintf(buf, tam, "%s%c%s%c%c%c%c%c%s%s %-5s %08X%s", estilo, cambio ? '*' : ' ', color_tenue(),
             '0' + ((r >> 4) & 1), '0' + ((r >> 3) & 1), '0' + ((r >> 2) & 1), '0' + ((r >> 1) & 1),
             '0' + (r & 1), color_reset(), estilo, NOMBRES_REG[r], cpu->reg[r], color_reset());
}

static void fila_memoria(Panel *P, const Sesion *s, const CPU *cpu, const CPU *antes, uint32_t dir)
{
    uint32_t w = palabra_en(cpu, dir);
    int cambio = antes && palabra_en(antes, dir) != w;
    const char *et = etiqueta_en(s->p, dir);
    const char *estilo = cambio ? color_cambio() : (w == 0 ? color_tenue() : "");
    char notas[64] = "";

    if (dir == cpu->reg[REG_SP] && cpu->reg[REG_SP] < PILA_INICIAL)
        snprintf(notas, sizeof notas, "<-$sp ");
    else if (cpu->reg[REG_SP] == PILA_INICIAL && dir == PILA_INICIAL - 4)
        snprintf(notas, sizeof notas, "<-push");
    if (et)
        snprintf(notas + strlen(notas), sizeof notas - strlen(notas), "%s:", et);
    LINEA(P, "%s%c0x%03X %c %02X %02X %02X %02X %10d %s%s", estilo, cambio ? '*' : ' ', dir,
          tipo_bloque(s->p, cpu, dir, dir + 4), cpu->ram[dir], cpu->ram[dir + 1], cpu->ram[dir + 2],
          cpu->ram[dir + 3], (int32_t)w, notas, color_reset());
}

/* Botones (entrada mapeada en memoria en 0x600): bits 7..0 de la palabra. */
static void panel_botones(Panel *P, const Sesion *s, const CPU *cpu, const CPU *antes)
{
    uint32_t v = palabra_en(cpu, BOTONES_DIR) & 0xFF;
    uint32_t a = antes ? palabra_en(antes, BOTONES_DIR) & 0xFF : v;
    const char *encendido = (B == &BORDES_ASCII) ? "[*]" : "[●]";
    char estados[512];
    size_t k = 0;
    int i, on;

    SEPARADOR(P, "BOTONES (0x600)  [bt n]");
    if (s->aire)
        LINEA(P, "");
    LINEA(P, "%s  7   6   5   4   3   2   1   0%s", color_tenue(), color_reset());
    for (i = 7; i >= 0; i--) {
        on = (v >> i) & 1;
        k += (size_t)snprintf(estados + k, sizeof estados - k, " %s%s%s",
                              on ? color_cambio() : (((a >> i) & 1) ? "" : color_tenue()),
                              on ? encendido : "[ ]", color_reset());
    }
    snprintf(estados + k, sizeof estados - k, "  = 0x%02X", v);
    LINEA(P, "%s", estados);
    if (s->aire)
        LINEA(P, "");
}

static void panel_derecho(Panel *P, const Sesion *s, const CPU *cpu, const CPU *antes)
{
    char a[128], b[128], titulo[64];
    int r, i, filas = filas_datos(s);
    uint32_t dir;

    SEPARADOR(P, "REGISTROS: banco 32x32 bits e internos");
    if (s->aire)
        LINEA(P, "");
    for (r = 0; r < 16; r++) {
        celda_registro(a, sizeof a, cpu, antes, r);
        celda_registro(b, sizeof b, cpu, antes, r + 16);
        LINEA(P, "%s  %s", a, b);
    }
    if (s->aire)
        LINEA(P, "");                   /* separar el banco de los registros internos */
    /* registros internos: no tienen dirección, el programador no los nombra */
    LINEA(P, " PC   %08X   IR %08X   %sACC  %08X%s", cpu->pc, cpu->ir,
          antes && antes->acc != cpu->acc ? color_cambio() : "", cpu->acc, color_reset());
    LINEA(P, " A    %08X   B  %08X   TEMP %08X", cpu->a, cpu->b, cpu->temp);
    LINEA(P, " %sHI   %08X%s   %sLO %08X%s", antes && antes->hi != cpu->hi ? color_cambio() : "",
          cpu->hi, color_reset(), antes && antes->lo != cpu->lo ? color_cambio() : "", cpu->lo, color_reset());
    formatear_banderas(cpu, antes, a, sizeof a);
    LINEA(P, " FLAGS  %s", a);
    if (s->aire)
        LINEA(P, "");

    snprintf(titulo, sizeof titulo, "MEMORIA desde 0x%03X  [m dir]", s->mem_inicio);
    SEPARADOR(P, titulo);
    if (s->aire)
        LINEA(P, "");
    for (i = 0, dir = s->mem_inicio; i < filas && dir < MEM_BYTES; i++, dir += 4)
        fila_memoria(P, s, cpu, antes, dir);
    for (; i < filas; i++)
        LINEA(P, "");
    if (s->aire)
        LINEA(P, "");

    snprintf(titulo, sizeof titulo, cpu->reg[REG_SP] == PILA_INICIAL ? "PILA vacía ($sp = 0x%03X)" :
             "PILA ($sp = 0x%03X)", cpu->reg[REG_SP]);
    SEPARADOR(P, titulo);
    if (s->aire)
        LINEA(P, "");
    for (i = 0, dir = inicio_pila(s, cpu); i < filas_pila(s); i++, dir += 4)
        fila_memoria(P, s, cpu, antes, dir);
    if (s->aire)
        LINEA(P, "");

    panel_botones(P, s, cpu, antes);
}

/* Pantalla mapeada en memoria: regla de columnas, 16 filas de píxeles y la paleta. */
static void panel_pantalla(Panel *P, const CPU *cpu, int filas, int ancho, int rgb)
{
    static const char *const nombres = "0 negro 1 rojo 2 verde 3 amarillo 4 azul 5 magenta 6 cian 7 blanco 8-F brillantes";
    char fila[MAX_LINEA - 16], regla[128], paleta[512];
    int px = (ancho - 1 >= 3 + 2 * VIDEO_ANCHO) ? 2 : 1;    /* caracteres por píxel */
    /* la pantalla RGB (32 filas) usa medio bloque: 2 filas de píxeles por línea */
    int dobles = rgb && color_reset()[0] && B != &BORDES_ASCII;
    int alto = rgb ? (dobles ? RGB_ALTO / 2 : RGB_ALTO) : VIDEO_ALTO;
    int visibles = filas - 2, y, x, c, usadas = 0;
    size_t k;

    if (visibles > alto)
        visibles = alto;
    if (visibles < alto)
        visibles--;                     /* una fila para el aviso */

    memset(regla, ' ', sizeof regla);
    for (x = 0; x < VIDEO_ANCHO; x += (px == 2 ? 4 : 8))
        regla[3 + snprintf(regla + 3 + px * x, 4, "%d", x) + px * x] = ' ';
    regla[3 + px * VIDEO_ANCHO] = '\0';
    LINEA(P, "%s%s%s", color_tenue(), regla, color_reset());
    usadas++;

    for (y = 0; y < visibles; y++, usadas++) {
        if (dobles)
            formatear_dos_filas_rgb(cpu, 2 * y, px, fila, sizeof fila);
        else if (rgb)
            formatear_fila_rgb(cpu, y, px, fila, sizeof fila);
        else
            formatear_fila_pantalla(cpu, y, px, fila, sizeof fila);
        LINEA(P, "%s%2d%s %s", color_tenue(), dobles ? 2 * y : y, color_reset(), fila);
    }
    if (visibles < alto) {
        LINEA(P, "%s(se ven %d de %d filas: agrande la terminal)%s", color_cambio(),
              visibles, alto, color_reset());
        usadas++;
    }

    if (rgb) {
        uint32_t pos = palabra_en(cpu, RGB_POS_DIR);
        LINEA(P, "POSICION x %u  y %u      COLOR 0x%06X      (sw a 0x604 y 0x608)",
              pos & 0xFFFF, pos >> 16, palabra_en(cpu, RGB_COLOR_DIR) & 0xFFFFFF);
    } else if (color_reset()[0]) {
        k = (size_t)snprintf(paleta, sizeof paleta, "%scolor:%s", color_tenue(), color_reset());
        for (c = 0; c < 16; c++)
            k += (size_t)snprintf(paleta + k, sizeof paleta - k, " \033[%dm%s\033[0m%X",
                                  c < 8 ? 40 + c : 100 + c - 8, px == 2 ? "  " : " ", c);
        LINEA(P, "%s", paleta);
    } else {
        LINEA(P, "%s", nombres);
    }
    for (usadas++; usadas < filas; usadas++)
        LINEA(P, "");
}

/* Consola: las últimas líneas de la salida de los syscall, partidas al ancho. */
static void panel_consola(Panel *P, const CPU *cpu, int filas, int ancho)
{
    static char lineas[200][MAX_LINEA];
    int n = 0, i, col = 0, max = ancho - 3;
    const char *t;

    lineas[0][0] = '\0';
    for (t = cpu->consola; *t && n < 200; t++) {
        if (*t == '\n' || col >= max) {
            lineas[n][col] = '\0';
            if (++n >= 200)
                break;
            col = 0;
            if (*t == '\n')
                continue;
        }
        if (*t == '\t') {
            do lineas[n][col++] = ' '; while (col % 4 && col < max);
        } else {
            lineas[n][col++] = *t;
        }
    }
    if (n < 200)
        lineas[n][col] = '\0';
    n = n < 200 ? n + 1 : 200;
    if (cpu->consola[0] == '\0') {
        LINEA(P, "%s(sin salida: use syscall 1, 4 u 11)%s", color_tenue(), color_reset());
        i = 1;
    } else {
        for (i = n > filas ? n - filas : 0; i < n; i++)
            LINEA(P, "%s", lineas[i]);
        i = n < filas ? n : filas;
    }
    for (; i < filas; i++)
        LINEA(P, "");
}

/* ------------------------------------------------------------------ */
/* Dibujo de la pantalla completa                                      */
/* ------------------------------------------------------------------ */

/* Devuelve 0 si la terminal es demasiado pequeña (solo muestra un aviso). */
static int dibujar(Sesion *s)
{
    static Panel izq, der;
    const CPU *cpu = &s->hist[s->k].cpu;
    const CPU *antes = s->k > 0 ? &s->hist[s->k - 1].cpu : NULL;
    const Instruccion *in = &s->hist[s->k].in;
    int ancho_total, i, filas_prog, necesarias, libre, m_sup, m_inf, filas_cambios;
    char texto[MAX_LINEA], estadisticas[128] = "";
    static const Linea vacia = { "", 0 };
    const Linea *a, *b;

    calcular_tamano(s);
    ajustar_memoria(s);
    ancho_total = s->ancho_izq + 1 + ANCHO_DER;

    if (s->columnas < ancho_total + 2 || s->filas < ALTO_MIN + FILAS_FIJAS) {
        printf("\033[H\033[2J%sArquiSim%s\n\n"
               "La ventana mide %d x %d caracteres y el simulador necesita al menos %d x %d.\n\n"
               "  - Agrande o maximice la ventana de la terminal, o\n"
               "  - reduzca el tamaño de la letra (Cmd - o Ctrl -),\n"
               "y luego presione Enter.\n\n"
               "También puede usar el modo sin interfaz:  ./sim --traza %s\n\n> ",
               color_negrita(), color_reset(), s->columnas, s->filas,
               ancho_total + 2, ALTO_MIN + FILAS_FIJAS, nombre_programa(s));
        fflush(stdout);
        return 0;
    }
    /* El programa (o la pantalla) usa solo las filas que necesita; el resto se
     * reparte entre los paneles de información: una línea en blanco arriba y
     * abajo de cada uno y hasta 4 filas más para CAMBIOS. */
    necesarias = s->ver_pantalla == 3 ? 12 : s->ver_pantalla ? VIDEO_ALTO + 2 : s->p->num_instr + 1;
    if (necesarias < 6)
        necesarias = 6;
    /* primero el contenido; los márgenes solo si sobra espacio */
    libre = s->alto - FIJAS_IZQ - necesarias;
    m_sup = s->aire && libre >= 4;      /* línea en blanco bajo cada título    */
    libre -= 4 * m_sup;
    m_inf = libre >= 4;                 /* y al final de cada ventana          */
    libre -= 4 * m_inf;
    filas_cambios = 4 + (libre > 4 ? 4 : libre > 0 ? libre : 0);
    filas_prog = s->alto - FIJAS_IZQ - 4 * m_sup - 4 * m_inf - (filas_cambios - 4);

    izq.n = der.n = 0;
#define MARGEN(n) do { int _i; for (_i = 0; _i < (n); _i++) LINEA(&izq, ""); } while (0)
    if (s->ver_pantalla == 3) {
        SEPARADOR(&izq, "CONSOLA (syscall)  [p] cambiar vista");
        MARGEN(m_sup);
        panel_consola(&izq, cpu, filas_prog, s->ancho_izq);
        MARGEN(m_inf);
    } else if (s->ver_pantalla) {
        if (s->ver_pantalla == 1)
            snprintf(texto, sizeof texto, "PANTALLA %dx%d (0x%03X-0x%03X)  [p] cambiar vista",
                     VIDEO_ANCHO, VIDEO_ALTO, VIDEO_BASE, BOTONES_DIR - 1);
        else
            snprintf(texto, sizeof texto, "PANTALLA RGB %dx%d (registros 0x%03X y 0x%03X)  [p] cambiar vista",
                     RGB_ANCHO, RGB_ALTO, RGB_POS_DIR, RGB_COLOR_DIR);
        SEPARADOR(&izq, texto);
        MARGEN(m_sup);
        panel_pantalla(&izq, cpu, filas_prog, s->ancho_izq, s->ver_pantalla == 2);
        MARGEN(m_inf);
    } else {
        snprintf(texto, sizeof texto, "PROGRAMA (.text: %d instrucciones, %u B)  [p] cambiar vista",
                 s->p->num_instr, s->p->text_fin - TEXT_BASE);
        SEPARADOR(&izq, texto);
        MARGEN(m_sup);
        panel_programa(&izq, s->p, cpu, antes, filas_prog, s->bp);
        MARGEN(m_inf);
    }
    SEPARADOR(&izq, "ÚLTIMA INSTRUCCIÓN EJECUTADA (IR)");
    MARGEN(m_sup);
    panel_ir(&izq, s->p, antes, in);
    MARGEN(m_inf);
    SEPARADOR(&izq, "CAMBIOS EN ESTE PASO");
    MARGEN(m_sup);
    panel_cambios(&izq, antes, cpu, filas_cambios);
    MARGEN(m_inf);
    SEPARADOR(&izq, "MAPA DE MEMORIA");
    MARGEN(m_sup);
    panel_mapa(&izq, s->p, cpu, s->ancho_izq);
    MARGEN(m_inf);
#undef MARGEN
    panel_derecho(&der, s, cpu, antes);

    fputs("\033[H", stdout);
    linea_titulo("ArquiSim", ancho_total);
    if (s->aire)
        fila_completa("", ancho_total);
    formatear_estadisticas(cpu, estadisticas, sizeof estadisticas);
    snprintf(texto, sizeof texto, "programa: %s%s%s   paso: %d   PC = 0x%03X   %s%s%s",
             color_negrita(), nombre_programa(s), color_reset(), s->base + s->k, cpu->pc,
             color_tenue(), estadisticas, color_reset());
    fila_completa(texto, ancho_total);
    if (s->aire)
        fila_completa("", ancho_total);

    for (i = 0; i < s->alto; i++) {
        a = i < izq.n ? &izq.l[i] : &vacia;
        b = i < der.n ? &der.l[i] : &vacia;
        fputs(a->sep ? B->t_izq : B->v, stdout);
        emitir_celda(a, s->ancho_izq);
        if (a->sep && b->sep)
            fputs(i == 0 ? B->t_arr : B->cruz, stdout);     /* la primera fila cuelga del encabezado */
        else if (a->sep)
            fputs(B->t_der, stdout);
        else if (b->sep)
            fputs(B->t_izq, stdout);
        else
            fputs(B->v, stdout);
        emitir_celda(b, ANCHO_DER);
        fputs(b->sep ? B->t_der : B->v, stdout);
        fputs("\033[K\n", stdout);
    }
    linea_horizontal(B->t_izq, B->t_der, ancho_total, s->ancho_izq, B->t_abj);

    /* línea de estado */
    if (cpu->excepcion[0])
        snprintf(texto, sizeof texto, "%sEXCEPCIÓN en PC = 0x%03X: %s%s", color_rojo(),
                 antes ? antes->pc : cpu->pc, cpu->excepcion, color_reset());
    else if (programa_terminado(s->p, cpu))
        snprintf(texto, sizeof texto, "%sFIN DEL PROGRAMA%s: %d instrucciones ejecutadas%s",
                 color_verde(), cpu->terminado ? " (syscall 10)" : "", s->base + s->k, color_reset());
    else if (s->corriendo)
        snprintf(texto, sizeof texto, "%sEN EJECUCIÓN%s (%d ms por instrucción)   teclas:  0-7 botones   "
                 "+ más rápido   - más lento   espacio pausa", color_cambio(), color_reset(), s->ms);
    else if (s->base + s->k >= s->max_pasos)
        snprintf(texto, sizeof texto, "%sLÍMITE de %d instrucciones (¿ciclo infinito?)%s",
                 color_rojo(), s->max_pasos, color_reset());
    else if (s->base + s->k == 0)
        snprintf(texto, sizeof texto, "LISTO: presione Enter para ejecutar la primera instrucción");
    else
        snprintf(texto, sizeof texto, "EJECUTANDO: la siguiente instrucción está en PC = 0x%03X", cpu->pc);
    if (s->mensaje[0])
        snprintf(texto + strlen(texto), sizeof texto - strlen(texto), "   %s| %s%s",
                 color_cambio(), s->mensaje, color_reset());
    fila_completa(texto, ancho_total);
    linea_horizontal(B->t_izq, B->t_der, ancho_total, -1, NULL);
    snprintf(texto, sizeof texto, "%s[Enter] paso  [b] atrás  [c] continuar  [a 100] tiempo real  "
             "[bp LOOP] punto de parada  [0] reiniciar  [q] salir%s", color_tenue(), color_reset());
    fila_completa(texto, ancho_total);
    snprintf(texto, sizeof texto, "%s[p] programa/pantallas  [m 0x200] ver memoria  "
             "[$t0 = 5]  [acc = 3]  [mem[0x200] = 7] asignar%s", color_tenue(), color_reset());
    fila_completa(texto, ancho_total);
    snprintf(texto, sizeof texto, "%s[l] cargar programa  [e] editar  [r] recargar del disco  [bt 3] botón  "
             "[t] colores (%s)%s", color_tenue(), nombre_tema(), color_reset());
    fila_completa(texto, ancho_total);
    linea_horizontal(B->inf_izq, B->inf_der, ancho_total, -1, NULL);
    fputs("\033[J> ", stdout);
    fflush(stdout);
    return 1;
}

/* ------------------------------------------------------------------ */
/* Ejecución                                                           */
/* ------------------------------------------------------------------ */

static void pedir_entero(Sesion *s);

/* Ejecuta una instrucción y guarda el nuevo estado. Devuelve 1 si avanzó. */
static int avanzar(Sesion *s, int sin_limite)
{
    const CPU *cpu = &s->hist[s->k].cpu;
    Estado *nuevo;
    uint32_t dir, pila;
    int filas;

    if (cpu->excepcion[0]) {
        snprintf(s->mensaje, sizeof s->mensaje, "procesador detenido: use [b] o [0]");
        return 0;
    }
    if (programa_terminado(s->p, cpu)) {
        snprintf(s->mensaje, sizeof s->mensaje, "el programa terminó: use [b], [0] o [q]");
        return 0;
    }
    if (!sin_limite && s->base + s->k >= s->max_pasos)
        return 0;
    if (s->k + 1 >= HIST_MAX) {         /* historial lleno: olvidar los pasos más antiguos */
        if (s->base == 0)
            s->cero = s->hist[0].cpu;
        memmove(s->hist, s->hist + HIST_DESCARTE, (size_t)(s->k + 1 - HIST_DESCARTE) * sizeof *s->hist);
        s->k -= HIST_DESCARTE;
        s->base += HIST_DESCARTE;
    }
    if (s->k + 1 >= s->cap) {
        nuevo = realloc(s->hist, (size_t)s->cap * 2 * sizeof *s->hist);
        if (!nuevo) {
            snprintf(s->mensaje, sizeof s->mensaje, "sin memoria para el historial");
            return 0;
        }
        s->hist = nuevo;
        s->cap *= 2;
    }
    s->hist[s->k + 1].cpu = s->hist[s->k].cpu;
    ejecutar_paso(s->isa, s->p, &s->hist[s->k + 1].cpu, &s->hist[s->k + 1].in);
    s->k++;
    if (strcmp(s->hist[s->k - 1].cpu.consola, s->hist[s->k].cpu.consola) != 0 && !s->consola_auto) {
        s->ver_pantalla = 3;            /* primera salida por consola: mostrarla */
        s->consola_auto = 1;
    }
    if (s->hist[s->k].cpu.pide_entero)
        pedir_entero(s);

    /* Si se escribió en memoria fuera de lo visible, mover la ventana de datos.
     * La primera escritura en la pantalla cambia la vista automáticamente. */
    filas = filas_datos(s);
    pila = inicio_pila(s, &s->hist[s->k].cpu);
    for (dir = 0; dir < MEM_BYTES; dir += 4) {
        if (palabra_en(&s->hist[s->k - 1].cpu, dir) == palabra_en(&s->hist[s->k].cpu, dir))
            continue;
        if (es_video(dir)) {            /* la pantalla se ve en su propio panel */
            if (!s->pantalla_auto) {
                s->ver_pantalla = 1;
                s->pantalla_auto = 1;
            }
            continue;
        }
        if (dir == RGB_POS_DIR || dir == RGB_COLOR_DIR) {   /* pantalla RGB */
            if (!s->rgb_auto) {
                s->ver_pantalla = 2;
                s->rgb_auto = 1;
            }
            continue;
        }
        if (dir >= pila && dir < pila + 4u * (uint32_t)filas_pila(s))
            continue;                   /* ya se ve en la ventana de la pila */
        if (dir < s->mem_inicio || dir >= s->mem_inicio + 4u * (uint32_t)filas)
            s->mem_inicio = dir >= 4u * (uint32_t)(filas / 2) ? dir - 4u * (uint32_t)(filas / 2) : 0;
        break;
    }
    return 1;
}

static char *recortar(char *t)
{
    char *fin;

    while (*t == ' ' || *t == '\t')
        t++;
    fin = t + strlen(t);
    while (fin > t && (fin[-1] == '\n' || fin[-1] == '\r' || fin[-1] == ' ' || fin[-1] == '\t'))
        *--fin = '\0';
    return t;
}

/* Elimina los espacios: permite escribir "mem [ 0x604 ]" o "rgb[3, 2]". */
static void quitar_espacios(char *t)
{
    char *d = t;

    for (; *t; t++)
        if (*t != ' ' && *t != '\t')
            *d++ = *t;
    *d = '\0';
}

/* syscall 5: pedir un entero al usuario y dejarlo en $v0. */
static void pedir_entero(Sesion *s)
{
    CPU *cpu = &s->hist[s->k].cpu;
    char linea[64], eco[32];
    long long v = 0;
    int ok;

    s->ver_pantalla = 3;
    for (;;) {
        snprintf(s->mensaje, sizeof s->mensaje, "syscall 5: escriba un número entero y presione Enter");
        dibujar(s);
        if (s->corriendo)
            terminal_teclas_directas(0);
        ok = terminal_leer_linea(linea, sizeof linea, &s->historial);
        if (s->corriendo)
            terminal_teclas_directas(1);
        if (!ok || leer_numero(recortar(linea), &v))
            break;
    }
    cpu->reg[2] = (uint32_t)v;
    cpu->pide_entero = 0;
    snprintf(eco, sizeof eco, "%lld\n", v);
    consola_agregar(cpu, eco);
    s->mensaje[0] = '\0';
}

/* Dirección: número o etiqueta del programa. */
static int leer_direccion(const Programa *p, const char *texto, long long *dir)
{
    int i;

    for (i = 0; i < p->num_etiquetas; i++)
        if (strcmp(p->etiquetas[i].nombre, texto) == 0) {
            *dir = p->etiquetas[i].dir;
            return 1;
        }
    return leer_numero(texto, dir);
}

/* "$t0 = 42" o "mem[0x200] = 42": modifica el estado actual de la máquina. */
static void asignar(Sesion *s, char *cmd)
{
    CPU *cpu = &s->hist[s->k].cpu;
    char *igual = strchr(cmd, '='), *lhs, *rhs, *cierre;
    long long valor, dir;
    int r, filas;

    *igual = '\0';
    lhs = recortar(cmd);
    quitar_espacios(lhs);                /* "mem [ 0x604 ]" = "mem[0x604]" */
    rhs = recortar(igual + 1);
    if (!leer_numero(rhs, &valor) || valor < -2147483648LL) {
        snprintf(s->mensaje, sizeof s->mensaje, "valor inválido '%s'", rhs);
        return;
    }

    if (strlen(lhs) == 1 && strchr("zncvZNCV", lhs[0])) {
        uint32_t bit = strchr("zZ", lhs[0]) ? BANDERA_Z : strchr("nN", lhs[0]) ? BANDERA_N :
                       strchr("cC", lhs[0]) ? BANDERA_C : BANDERA_V;
        cpu->banderas = valor ? (cpu->banderas | bit) : (cpu->banderas & ~bit);
        snprintf(s->mensaje, sizeof s->mensaje, "bandera %c = %d", lhs[0], valor != 0);
        return;
    }
    if (strcmp(lhs, "hi") == 0 || strcmp(lhs, "lo") == 0) {
        *(lhs[0] == 'h' ? &cpu->hi : &cpu->lo) = (uint32_t)valor;
        snprintf(s->mensaje, sizeof s->mensaje, "%s = 0x%08X", lhs, (uint32_t)valor);
        return;
    }
    if (strcmp(lhs, "acc") == 0 || strcmp(lhs, "ACC") == 0) {
        cpu->acc = (uint32_t)valor;
        snprintf(s->mensaje, sizeof s->mensaje, "ACC = 0x%08X", cpu->acc);
        return;
    }
    if ((r = numero_registro(lhs)) >= 0) {
        if (r == 0) {
            snprintf(s->mensaje, sizeof s->mensaje, "$zero siempre vale 0");
            return;
        }
        cpu->reg[r] = (uint32_t)valor;
        snprintf(s->mensaje, sizeof s->mensaje, "%s = 0x%08X", NOMBRES_REG[r], cpu->reg[r]);
        return;
    }

    cierre = strrchr(lhs, ']');
    if (strncmp(lhs, "mem[", 4) != 0 || !cierre || cierre[1] != '\0') {
        snprintf(s->mensaje, sizeof s->mensaje, "use  $t0 = 42  o  mem[0x200] = 42");
        return;
    }
    *cierre = '\0';
    if (!leer_direccion(s->p, recortar(lhs + 4), &dir) || dir < 0 || dir > MEM_BYTES - 4 || dir % 4) {
        snprintf(s->mensaje, sizeof s->mensaje, "dirección inválida (múltiplo de 4, 0x000-0x%03X)",
                 MEM_BYTES - 4);
        return;
    }
    if ((uint32_t)dir == BOTONES_DIR) {
        cambiar_botones(cpu, (uint32_t)valor);
    } else if ((uint32_t)dir == RGB_COLOR_DIR) {
        poner_palabra(cpu, (uint32_t)dir, (uint32_t)valor);
        rgb_escribir_color(cpu, (uint32_t)valor);      /* el dispositivo dibuja */
        s->ver_pantalla = 2;
    }
    else
        poner_palabra(cpu, (uint32_t)dir, (uint32_t)valor);   /* a mano: no cuenta como escritura */
    snprintf(s->mensaje, sizeof s->mensaje, "MEM[0x%03X] = 0x%08X", (unsigned)dir, (uint32_t)valor);

    /* Lo que se escribe a mano siempre queda visible en la ventana MEMORIA;
     * si es un píxel, además se muestra la pantalla. */
    filas = filas_datos(s);
    if (es_video((uint32_t)dir))
        s->ver_pantalla = 1;
    if ((uint32_t)dir < s->mem_inicio || (uint32_t)dir >= s->mem_inicio + 4u * (uint32_t)filas)
        s->mem_inicio = (uint32_t)dir;
}

static void reiniciar(Sesion *s)
{
    if (s->base > 0) {                  /* el paso 0 ya no está en el historial */
        s->hist[0].cpu = s->cero;
        memset(&s->hist[0].in, 0, sizeof s->hist[0].in);
        s->base = 0;
    }
    s->k = 0;
    s->mem_inicio = DATA_BASE;
    s->ver_pantalla = s->pantalla_auto = s->rgb_auto = s->consola_auto = 0;
}

/* Ensambla 'ruta'. Si funciona reemplaza el programa actual; si no, deja los
 * errores en 'errores' y el programa actual sigue cargado. */
static int en_parada(Sesion *s)
{
    uint32_t pc = s->hist[s->k].cpu.pc;

    if (pc >= s->p->text_fin || pc % 4 || !s->bp[pc / 4])
        return 0;
    snprintf(s->mensaje, sizeof s->mensaje, "punto de parada en 0x%03X", pc);
    return 1;
}

/* "bp LOOP" o "bp 0x00C" activa o desactiva un punto de parada; "bp" los borra todos. */
static void punto_de_parada(Sesion *s, char *arg)
{
    long long dir;
    int i, n = 0;

    if (*arg == '\0') {
        for (i = 0; i < (int)MAX_INSTR_PROGRAMA; i++)
            n += s->bp[i];
        memset(s->bp, 0, sizeof s->bp);
        snprintf(s->mensaje, sizeof s->mensaje, "se borraron %d puntos de parada", n);
        return;
    }
    if (!leer_direccion(s->p, arg, &dir) || dir < 0 || dir >= (long long)s->p->text_fin || dir % 4) {
        snprintf(s->mensaje, sizeof s->mensaje, "'%s' no es una instrucción del programa", arg);
        return;
    }
    s->bp[dir / 4] = !s->bp[dir / 4];
    snprintf(s->mensaje, sizeof s->mensaje, "punto de parada en 0x%03X %s", (unsigned)dir,
             s->bp[dir / 4] ? "activado (c y a se detienen allí)" : "desactivado");
}

/* "bt 3" cambia el estado del botón 3; "bt" los apaga todos. */
static void boton(Sesion *s, char *arg)
{
    CPU *cpu = &s->hist[s->k].cpu;
    uint32_t v = palabra_en(cpu, BOTONES_DIR) & 0xFF;
    long long n;

    if (*arg == '\0') {
        cambiar_botones(cpu, 0);
        snprintf(s->mensaje, sizeof s->mensaje, "botones apagados");
        return;
    }
    if (!leer_numero(arg, &n) || n < 0 || n > 7) {
        snprintf(s->mensaje, sizeof s->mensaje, "use bt 0 ... bt 7");
        return;
    }
    v ^= 1u << n;
    cambiar_botones(cpu, v);
    snprintf(s->mensaje, sizeof s->mensaje, "botón %lld %s (0x600 = 0x%02X)", n,
             (v >> n) & 1 ? "activado" : "desactivado", v);
}

static int cargar(Sesion *s, const char *ruta, char *errores, size_t tam)
{
    static Programa nuevo;
    static CPU cpu;
    char copia[MAX_RUTA];

    snprintf(copia, sizeof copia, "%s", ruta);
    if (!cargar_programa(s->isa, copia, &nuevo, &cpu)) {
        snprintf(errores, tam, "%s", nuevo.errores);
        return 0;
    }
    if (strcmp(s->p->ruta, nuevo.ruta) != 0)
        memset(s->bp, 0, sizeof s->bp);     /* otro programa: borrar puntos de parada */
    *s->p = nuevo;
    *s->inicial = cpu;
    s->base = 0;
    s->hist[0].cpu = cpu;
    memset(&s->hist[0].in, 0, sizeof s->hist[0].in);
    reiniciar(s);
    snprintf(s->mensaje, sizeof s->mensaje, "cargado %s (%d instrucciones)",
             nombre_programa(s), s->p->num_instr);
    errores[0] = '\0';
    return 1;
}

static void mostrar_errores(const char *errores)
{
    char linea[16];

    printf("\033[H\033[2J%sNo se pudo cargar el programa:%s\n\n%s\n"
           "El programa anterior sigue cargado. Presione Enter para continuar.",
           color_rojo(), color_reset(), errores);
    fflush(stdout);
    if (!fgets(linea, sizeof linea, stdin))
        return;
}

#ifdef _WIN32
#define EDITOR_DEFECTO "notepad"
#else
#define EDITOR_DEFECTO "nano"
#endif

/* Abre el programa en un editor de texto y lo recarga al cerrarlo.
 * Usa la variable de entorno VISUAL o EDITOR (p. ej. EDITOR="code --wait"). */
static void editar(Sesion *s, char *errores, size_t tam)
{
    const char *editor = getenv("VISUAL");
    char orden[MAX_RUTA + 256];
    int r;

    if (!editor || !*editor)
        editor = getenv("EDITOR");
    if (!editor || !*editor)
        editor = EDITOR_DEFECTO;
    snprintf(orden, sizeof orden, "%s \"%s\"", editor, s->p->ruta);

    terminal_pantalla_alterna(0);
    printf("Editando %s con '%s'. Guarde y cierre el editor para volver al simulador...\n",
           s->p->ruta, editor);
    fflush(stdout);
    r = system(orden);
    terminal_pantalla_alterna(1);

    if (r != 0) {
        snprintf(s->mensaje, sizeof s->mensaje, "no se pudo abrir '%s' (defina la variable EDITOR)", editor);
        return;
    }
    if (!cargar(s, s->p->ruta, errores, tam))
        mostrar_errores(errores);
    else
        snprintf(s->mensaje, sizeof s->mensaje, "programa editado y recargado (%d instrucciones)",
                 s->p->num_instr);
}

/* Pantalla para elegir un programa. Devuelve 1 si cargó uno, 0 si se canceló
 * y -1 si el usuario quiere salir. */
static int elegir_programa(Sesion *s, const char *error_previo)
{
    static char nombres[64][MAX_NOMBRE_ARCHIVO];
    char linea[MAX_RUTA], ruta[MAX_RUTA], errores[MAX_ERRORES], cwd[512];
    const char *dir;
    int n, i, columnas, hay_programa = s->p->ruta[0] != '\0';
    long long v;
    char *cmd;

    snprintf(errores, sizeof errores, "%s", error_previo ? error_previo : "");
    for (;;) {
        dir = "programas";
        n = terminal_listar_asm(dir, nombres, 64);
        if (n == 0) {
            dir = ".";
            n = terminal_listar_asm(dir, nombres, 64);
        }
        columnas = n > 12 ? 2 : 1;

        printf("\033[H\033[2J%sArquiSim: cargar programa%s\n\n", color_negrita(), color_reset());
        if (n == 0)
            printf("No hay archivos .asm en programas/ ni en el directorio actual.\n");
        else
            printf("Programas en %s/\n\n", dir);
        terminal_directorio(cwd, sizeof cwd);
        printf("%s(directorio actual: %s)%s\n\n", color_tenue(), cwd, color_reset());
        for (i = 0; i < n; i++)
            printf("  %s%2d)%s %-34s%s", color_titulo(), i + 1, color_reset(), nombres[i],
                   (i % columnas == columnas - 1 || i == n - 1) ? "\n" : "");
        if (errores[0])
            printf("\n%sNo se pudo cargar:%s\n%s", color_rojo(), color_reset(), errores);
        printf("\nEscriba el número o la ruta de un archivo .asm  (%sq = salir):\n> ",
               hay_programa ? "Enter = volver, " : "");
        fflush(stdout);

        if (!terminal_leer_linea(linea, sizeof linea, &s->historial))
            return -1;
        cmd = recortar(linea);
        if (strcmp(cmd, "q") == 0)
            return -1;
        if (cmd[0] == '\0') {
            if (hay_programa)
                return 0;
            continue;
        }
        if (leer_numero(cmd, &v) && v >= 1 && v <= n)
            snprintf(ruta, sizeof ruta, "%s/%s", dir, nombres[v - 1]);
        else
            snprintf(ruta, sizeof ruta, "%s", cmd);
        if (cargar(s, ruta, errores, sizeof errores))
            return 1;
    }
}

/* Ejecución en tiempo real: una instrucción cada 'ms' milisegundos, con el teclado
 * en modo directo (sin Enter). Las teclas 0-7 cambian los botones mientras el
 * programa corre; + y - cambian la velocidad; espacio, Enter o q pausan. */
static void correr(Sesion *s, int ms)
{
    long ahora, proximo = terminal_ms(), dibujo = 0;
    int tecla, seguir = 1;
    uint32_t botones;

    s->corriendo = 1;
    s->ms = ms;
    terminal_animando(1);               /* Ctrl+C también pausa */
    terminal_teclas_directas(1);
    while (seguir && !terminal_interrumpido) {
        while ((tecla = terminal_tecla()) >= 0) {
            if (tecla >= '0' && tecla <= '7') {
                botones = palabra_en(&s->hist[s->k].cpu, BOTONES_DIR) ^ (1u << (tecla - '0'));
                cambiar_botones(&s->hist[s->k].cpu, botones);
                snprintf(s->mensaje, sizeof s->mensaje, "botón %c %s", tecla,
                         (botones >> (tecla - '0')) & 1 ? "activado" : "desactivado");
                dibujo = 0;
            } else if (tecla == '+') {
                s->ms = s->ms > 1 ? s->ms / 2 : 1;
            } else if (tecla == '-') {
                s->ms = s->ms < 2000 ? s->ms * 2 : 2000;
            } else if (tecla == ' ' || tecla == '\n' || tecla == '\r' || tecla == 'q' || tecla == 27) {
                seguir = 0;
            }
        }
        if (!seguir)
            break;
        ahora = terminal_ms();
        if (ahora >= proximo) {
            if (!avanzar(s, 1) || en_parada(s))
                break;
            proximo = ahora + s->ms + (long)s->hist[s->k].cpu.espera_ms;   /* syscall 32 */
        }
        if (ahora - dibujo >= REFRESCO_MS || s->ms >= REFRESCO_MS) {
            dibujar(s);
            dibujo = ahora;
        }
        terminal_dormir_ms(s->ms < 2 ? 0 : 1);
    }
    terminal_teclas_directas(0);
    terminal_animando(0);
    s->corriendo = 0;
}

int tui_ejecutar(const ISA *isa, const char *ruta, int max_pasos,
                 Programa *p, CPU *inicial, CPU *final)
{
    static char buffer_salida[1 << 16];
    static char errores[MAX_ERRORES];
    Sesion s;
    char linea[MAX_RUTA], *cmd;
    long long valor;
    int ms, pasos, cabe, salir = 0;

    memset(&s, 0, sizeof s);
    s.isa = isa;
    s.p = p;
    s.inicial = inicial;
    s.max_pasos = max_pasos;
    s.cap = 64;
    s.hist = malloc((size_t)s.cap * sizeof *s.hist);
    if (!s.hist)
        return -1;
    p->ruta[0] = '\0';

    setvbuf(stdout, buffer_salida, _IOFBF, sizeof buffer_salida);   /* dibujar sin parpadeo */
    terminal_capturar_ctrl_c();
    terminal_pantalla_alterna(1);

    if (!(ruta && cargar(&s, ruta, errores, sizeof errores)) &&
        elegir_programa(&s, ruta ? errores : NULL) < 0)
        salir = 1;

    while (!salir) {
        cabe = dibujar(&s);
        s.mensaje[0] = '\0';
        if (!terminal_leer_linea(linea, sizeof linea, &s.historial))
            break;
        cmd = recortar(linea);
        if (!cabe && strcmp(cmd, "q") != 0)
            continue;                   /* solo redibujar tras cambiar el tamaño */

        if (cmd[0] == '\0' || strcmp(cmd, "s") == 0) {
            avanzar(&s, 0);
        } else if (strcmp(cmd, "q") == 0) {
            break;
        } else if (strcmp(cmd, "b") == 0) {
            if (s.k > 0)
                s.k--;
            else if (s.base > 0)
                snprintf(s.mensaje, sizeof s.mensaje, "solo se guardan los últimos %d pasos", HIST_MAX);
            else
                snprintf(s.mensaje, sizeof s.mensaje, "ya está en el estado inicial");
        } else if (strcmp(cmd, "p") == 0) {
            s.ver_pantalla = (s.ver_pantalla + 1) % 4;    /* programa, pantalla, RGB, consola */
        } else if (strcmp(cmd, "0") == 0) {
            reiniciar(&s);
        } else if (strcmp(cmd, "c") == 0) {
            s.mensaje[0] = '\0';
            while (avanzar(&s, 0) && !en_parada(&s))
                ;
        } else if (cmd[0] == 'a' && (cmd[1] == '\0' || cmd[1] == ' ')) {
            ms = ANIMACION_MS;
            if (leer_numero(recortar(cmd + 1), &valor) && valor > 0 && valor < 10000)
                ms = (int)valor;
            s.mensaje[0] = '\0';
            correr(&s, ms);
        } else if (strcmp(cmd, "l") == 0) {
            if (elegir_programa(&s, NULL) < 0)
                break;
        } else if (cmd[0] == 'l' && cmd[1] == ' ') {
            if (!cargar(&s, recortar(cmd + 1), errores, sizeof errores))
                mostrar_errores(errores);
        } else if (strncmp(cmd, "bt", 2) == 0 && (cmd[2] == '\0' || cmd[2] == ' ')) {
            boton(&s, recortar(cmd + 2));
        } else if (strncmp(cmd, "bp", 2) == 0 && (cmd[2] == '\0' || cmd[2] == ' ')) {
            punto_de_parada(&s, recortar(cmd + 2));
        } else if (strcmp(cmd, "t") == 0) {
            siguiente_tema();
            snprintf(s.mensaje, sizeof s.mensaje, "tema de colores: %s  (t = siguiente)", nombre_tema());
        } else if (strcmp(cmd, "e") == 0) {
            editar(&s, errores, sizeof errores);
        } else if (strcmp(cmd, "r") == 0) {
            if (!cargar(&s, s.p->ruta, errores, sizeof errores))
                mostrar_errores(errores);
        } else if (strchr(cmd, '=')) {
            asignar(&s, cmd);
        } else if (cmd[0] == 'm' && (cmd[1] == '\0' || cmd[1] == ' ')) {
            if (cmd[1] == '\0')
                s.mem_inicio = DATA_BASE;
            else if (leer_direccion(s.p, recortar(cmd + 1), &valor) && valor >= 0 && valor < MEM_BYTES)
                s.mem_inicio = (uint32_t)valor;
            else
                snprintf(s.mensaje, sizeof s.mensaje, "dirección inválida (use p. ej. m 0x200)");
        } else {
            snprintf(s.mensaje, sizeof s.mensaje, "comando desconocido '%s'", cmd);
        }
    }

    terminal_pantalla_alterna(0);
    pasos = p->ruta[0] ? s.base + s.k : -1;
    if (pasos >= 0)
        *final = s.hist[s.k].cpu;
    free(s.hist);
    return pasos;
}
