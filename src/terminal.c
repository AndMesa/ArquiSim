/*
 * terminal.c -- Funciones dependientes del sistema operativo.
 */
#ifndef _WIN32
#define _DEFAULT_SOURCE
#define _DARWIN_C_SOURCE
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "terminal.h"

#ifdef _WIN32
#include <conio.h>
#include <direct.h>
#include <io.h>
#include <stdint.h>
#include <windows.h>
#else
#include <dirent.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#endif

volatile sig_atomic_t terminal_interrumpido = 0;
static volatile sig_atomic_t animando = 0;
static volatile sig_atomic_t alterna = 0;
static volatile sig_atomic_t directas = 0;
#ifndef _WIN32
static struct termios termios_original;
#endif

void terminal_preparar(void)
{
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD modo;
    SetConsoleOutputCP(CP_UTF8);
    if (GetConsoleMode(h, &modo))
        SetConsoleMode(h, modo | 0x0004 /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */);
#endif
}

int terminal_salida_es_tty(void)
{
#ifdef _WIN32
    return _isatty(_fileno(stdout));
#else
    return isatty(STDOUT_FILENO);
#endif
}

int terminal_interactiva(void)
{
#ifdef _WIN32
    return _isatty(_fileno(stdout)) && _isatty(_fileno(stdin));
#else
    return isatty(STDOUT_FILENO) && isatty(STDIN_FILENO);
#endif
}

void terminal_tamano(int *columnas, int *filas)
{
    *columnas = 120;
    *filas = 40;
#ifdef _WIN32
    {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
            *columnas = info.srWindow.Right - info.srWindow.Left + 1;
            *filas = info.srWindow.Bottom - info.srWindow.Top + 1;
        }
    }
#else
    {
        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0 && w.ws_row > 0) {
            *columnas = w.ws_col;
            *filas = w.ws_row;
        }
    }
#endif
}

void terminal_pantalla_alterna(int activar)
{
    fputs(activar ? "\033[?1049h\033[H\033[2J" : "\033[0m\033[?1049l", stdout);
    fflush(stdout);
    alterna = activar;
}

void terminal_dormir_ms(int ms)
{
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec t;
    t.tv_sec = ms / 1000;
    t.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&t, NULL);
#endif
}

static void al_interrumpir(int senal)
{
    (void)senal;
    if (animando) {                 /* Ctrl+C solo detiene la animación */
        terminal_interrumpido = 1;
        signal(SIGINT, al_interrumpir);
        return;
    }
#ifndef _WIN32
    if (directas)
        tcsetattr(STDIN_FILENO, TCSANOW, &termios_original);
#endif
    if (alterna) {                  /* restaurar la terminal antes de salir */
#ifdef _WIN32
        fputs("\033[0m\033[?1049l\n", stdout);
#else
        static const char restaurar[] = "\033[0m\033[?1049l\n";
        ssize_t n = write(STDOUT_FILENO, restaurar, sizeof restaurar - 1);
        (void)n;
#endif
    }
    _Exit(130);
}

void terminal_capturar_ctrl_c(void)
{
    signal(SIGINT, al_interrumpir);
}

void terminal_animando(int activo)
{
    terminal_interrumpido = 0;
    animando = activo;
}

static int termina_en_asm(const char *nombre)
{
    size_t n = strlen(nombre);
    return n > 4 && strcmp(nombre + n - 4, ".asm") == 0;
}

static int comparar(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

int terminal_listar_asm(const char *directorio, char nombres[][MAX_NOMBRE_ARCHIVO], int max)
{
    int n = 0;
#ifdef _WIN32
    struct _finddata_t info;
    intptr_t h;
    char patron[512];

    snprintf(patron, sizeof patron, "%s\\*.asm", directorio);
    h = _findfirst(patron, &info);
    if (h != -1) {
        do {
            if (n < max && termina_en_asm(info.name) && strlen(info.name) < MAX_NOMBRE_ARCHIVO)
                strcpy(nombres[n++], info.name);
        } while (_findnext(h, &info) == 0);
        _findclose(h);
    }
#else
    DIR *d = opendir(directorio);
    struct dirent *e;

    if (!d)
        return 0;
    while ((e = readdir(d)) != NULL)
        if (n < max && termina_en_asm(e->d_name) && strlen(e->d_name) < MAX_NOMBRE_ARCHIVO)
            strcpy(nombres[n++], e->d_name);
    closedir(d);
#endif
    qsort(nombres, (size_t)n, MAX_NOMBRE_ARCHIVO, comparar);
    return n;
}

/* bloqueante = 0: read() devuelve de inmediato; 1: espera al menos una tecla. */
static void modo_tecla(int activar, int bloqueante)
{
#ifndef _WIN32
    struct termios t;

    if (activar && !directas) {
        tcgetattr(STDIN_FILENO, &termios_original);
        t = termios_original;
        t.c_lflag &= ~(tcflag_t)(ICANON | ECHO);   /* sin esperar Enter y sin eco */
        t.c_cc[VMIN] = bloqueante ? 1 : 0;
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    } else if (!activar && directas) {
        tcsetattr(STDIN_FILENO, TCSANOW, &termios_original);
    }
#else
    (void)bloqueante;
#endif
    directas = activar;
}

void terminal_teclas_directas(int activar)
{
    modo_tecla(activar, 0);
}

int terminal_tecla(void)
{
#ifdef _WIN32
    return _kbhit() ? _getch() : -1;
#else
    unsigned char c;
    return read(STDIN_FILENO, &c, 1) == 1 ? c : -1;
#endif
}

long terminal_ms(void)
{
#ifdef _WIN32
    return (long)GetTickCount64();
#else
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (long)t.tv_sec * 1000L + t.tv_nsec / 1000000L;
#endif
}

void terminal_directorio(char *buf, size_t tam)
{
#ifdef _WIN32
    if (!_getcwd(buf, (int)tam))
#else
    if (!getcwd(buf, tam))
#endif
        snprintf(buf, tam, "?");
}

/* ------------------------------------------------------------------ */
/* Editor de línea con historial                                       */
/* ------------------------------------------------------------------ */

enum { TECLA_ARRIBA = 1000, TECLA_ABAJO, TECLA_OTRA };

static int leer_tecla_bloqueante(void)
{
#ifdef _WIN32
    int c = _getch();
    if (c == 0 || c == 0xE0) {              /* tecla especial: flechas */
        c = _getch();
        return c == 72 ? TECLA_ARRIBA : c == 80 ? TECLA_ABAJO : TECLA_OTRA;
    }
    return c == '\r' ? '\n' : c;
#else
    unsigned char c, sec[2];
    struct pollfd p;

    if (read(STDIN_FILENO, &c, 1) != 1)
        return -1;
    if (c != 27)
        return c;
    /* secuencia de escape: ESC [ A (arriba), ESC [ B (abajo)... o solo ESC */
    p.fd = STDIN_FILENO;
    p.events = POLLIN;
    if (poll(&p, 1, 50) <= 0 || read(STDIN_FILENO, &sec[0], 1) != 1)
        return TECLA_OTRA;
    if (poll(&p, 1, 50) <= 0 || read(STDIN_FILENO, &sec[1], 1) != 1)
        return TECLA_OTRA;
    if (sec[0] == '[' || sec[0] == 'O') {
        if (sec[1] == 'A') return TECLA_ARRIBA;
        if (sec[1] == 'B') return TECLA_ABAJO;
        while (sec[1] >= '0' && sec[1] <= '9' &&      /* p. ej. ESC [ 3 ~ */
               poll(&p, 1, 50) > 0 && read(STDIN_FILENO, &sec[1], 1) == 1)
            ;
    }
    return TECLA_OTRA;
#endif
}

static int caracteres(const char *s)
{
    int n = 0;
    for (; *s; s++)
        if (((unsigned char)*s & 0xC0) != 0x80)
            n++;
    return n;
}

static void reemplazar(char *buf, size_t tam, const char *nueva)
{
    int i, n = caracteres(buf);

    for (i = 0; i < n; i++)
        fputs("\b \b", stdout);
    snprintf(buf, tam, "%s", nueva);
    fputs(buf, stdout);
}

int terminal_leer_linea(char *buf, size_t tam, Historial *h)
{
    char actual[MAX_COMANDO] = "";
    size_t len = 0;
    int c, pos = h->n, ok = 1;

    buf[0] = '\0';
    modo_tecla(1, 1);
    for (;;) {
        fflush(stdout);
        c = leer_tecla_bloqueante();
        if (c < 0 || (c == 4 && len == 0)) {        /* fin de la entrada o Ctrl+D */
            ok = 0;
            break;
        }
        if (c == '\n' || c == '\r') {
            putchar('\n');
            break;
        }
        if (c == TECLA_ARRIBA && pos > 0) {
            if (pos == h->n)
                snprintf(actual, sizeof actual, "%s", buf);
            reemplazar(buf, tam, h->lineas[--pos]);
        } else if (c == TECLA_ABAJO && pos < h->n) {
            pos++;
            reemplazar(buf, tam, pos == h->n ? actual : h->lineas[pos]);
        } else if (c == 127 || c == 8) {            /* retroceso (borra un carácter UTF-8) */
            if (len > 0) {
                do
                    len--;
                while (len > 0 && ((unsigned char)buf[len] & 0xC0) == 0x80);
                buf[len] = '\0';
                fputs("\b \b", stdout);
            }
        } else if (c == 21) {                       /* Ctrl+U: borrar la línea */
            reemplazar(buf, tam, "");
        } else if (c >= 32 && c < 256 && c != 127 && len + 1 < tam) {
            buf[len++] = (char)c;
            buf[len] = '\0';
            putchar(c);
        }
        len = strlen(buf);
    }
    modo_tecla(0, 0);
    fflush(stdout);

    /* guardar en el historial (sin repetir el último) */
    if (ok && buf[0] && (h->n == 0 || strcmp(h->lineas[h->n - 1], buf) != 0)) {
        if (h->n == MAX_HISTORIAL) {
            memmove(h->lineas[0], h->lineas[1], (MAX_HISTORIAL - 1) * sizeof h->lineas[0]);
            h->n--;
        }
        snprintf(h->lineas[h->n++], MAX_COMANDO, "%s", buf);
    }
    return ok;
}
