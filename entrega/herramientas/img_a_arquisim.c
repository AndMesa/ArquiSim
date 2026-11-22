/*
 * img_a_arquisim -- convierte una imagen BMP o PPM en una sección .data para ArquiSim.
 *
 *   1. Lee la imagen: BMP sin compresión de 24 o 32 bits (Paint, Vista Previa,
 *      sips, GIMP...) o PPM binario (P6).
 *   2. Recorta el centro para dejarla cuadrada (o la estira con -e).
 *   3. La reduce a N x N píxeles (por defecto 16 x 16) promediando los píxeles
 *      originales que caen dentro de cada píxel nuevo.
 *   4. Guarda cada píxel en 1 byte con formato RGB332:
 *
 *          bit  7 6 5 | 4 3 2 | 1 0
 *               R R R | G G G | B B          (rojo 0-7, verde 0-7, azul 0-3)
 *
 *   5. Escribe en la salida estándar las líneas .byte, una fila por línea.
 *
 * Compilar:
 *     cc -O2 -o img_a_arquisim img_a_arquisim.c
 *
 * Usar:
 *     ./img_a_arquisim foto.bmp                   muestra el resultado en pantalla
 *     ./img_a_arquisim foto.bmp > imagen.asm      lo guarda en un archivo
 *     ./img_a_arquisim -v foto.bmp > imagen.asm   además dibuja una vista previa
 *
 * Opciones:
 *     -n N        tamaño de la imagen: N x N píxeles (1 a 16; por defecto 16)
 *     -e          estirar la imagen en lugar de recortar el centro
 *     -l NOMBRE   etiqueta de la imagen en .data (por defecto IMAGEN)
 *     -v          vista previa en colores en la terminal (va a stderr, así que no
 *                 se mezcla con el archivo generado)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LADO 16     /* 16 x 16 = 256 bytes: la mitad de la zona .data + pila */

typedef struct {
    int ancho, alto;
    unsigned char *rgb;     /* ancho * alto * 3 bytes, fila 0 arriba */
} Imagen;

static unsigned leer16(const unsigned char *p) { return p[0] | (p[1] << 8); }
static unsigned long leer32(const unsigned char *p)
{
    return p[0] | (p[1] << 8) | ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

/* BMP sin compresión (BI_RGB) de 24 o 32 bits, o 32 bits con máscaras (BI_BITFIELDS
 * en el orden habitual BGRA). Las filas vienen de abajo hacia arriba salvo que la
 * altura sea negativa, y cada fila se rellena hasta un múltiplo de 4 bytes. */
static const char *leer_bmp(FILE *f, Imagen *im)
{
    unsigned char cab[54];
    unsigned long inicio, compresion;
    long alto;
    int bits, bpp, abajo_arriba, relleno;

    if (fread(cab, 1, 54, f) != 54 || cab[0] != 'B' || cab[1] != 'M')
        return "no es un BMP válido";
    inicio     = leer32(cab + 10);
    im->ancho  = (int)leer32(cab + 18);
    alto       = (long)(int)leer32(cab + 22);
    bits       = (int)leer16(cab + 28);
    compresion = leer32(cab + 30);
    if (bits != 24 && bits != 32)
        return "el BMP debe ser de 24 o 32 bits (guárdelo como \"BMP de 24 bits\")";
    if (compresion != 0 && !(compresion == 3 && bits == 32))
        return "el BMP está comprimido (guárdelo sin compresión)";
    abajo_arriba = alto > 0;
    im->alto = (int)(alto > 0 ? alto : -alto);
    if (im->ancho <= 0 || im->alto <= 0 || im->ancho > 20000 || im->alto > 20000)
        return "tamaño de imagen inválido";

    bpp = bits / 8;
    relleno = (4 - (im->ancho * bpp) % 4) % 4;
    im->rgb = malloc((size_t)im->ancho * im->alto * 3);
    if (!im->rgb || fseek(f, (long)inicio, SEEK_SET) != 0)
        return "sin memoria o archivo incompleto";
    for (int i = 0; i < im->alto; i++) {
        int y = abajo_arriba ? im->alto - 1 - i : i;
        for (int x = 0; x < im->ancho; x++) {
            unsigned char p[4];
            unsigned char *d = im->rgb + 3 * ((size_t)y * im->ancho + x);
            if (fread(p, 1, (size_t)bpp, f) != (size_t)bpp)
                return "archivo BMP incompleto";
            d[0] = p[2]; d[1] = p[1]; d[2] = p[0];      /* BGR -> RGB */
        }
        fseek(f, relleno, SEEK_CUR);
    }
    return NULL;
}

/* PPM binario: "P6 ancho alto maximo" y luego los bytes RGB (comentarios con #). */
static int leer_numero_ppm(FILE *f)
{
    int c, n = 0;
    do {
        c = fgetc(f);
        if (c == '#')
            while (c != '\n' && c != EOF) c = fgetc(f);
    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    if (c < '0' || c > '9') return -1;
    while (c >= '0' && c <= '9') { n = n * 10 + (c - '0'); c = fgetc(f); }
    return n;
}

static const char *leer_ppm(FILE *f, Imagen *im)
{
    int maximo;
    size_t total;
    if (fgetc(f) != 'P' || fgetc(f) != '6')
        return "no es un PPM binario (P6)";
    im->ancho = leer_numero_ppm(f);
    im->alto  = leer_numero_ppm(f);
    maximo    = leer_numero_ppm(f);
    if (im->ancho <= 0 || im->alto <= 0 || maximo != 255)
        return "PPM inválido (se espera P6 con máximo 255)";
    total = (size_t)im->ancho * im->alto * 3;
    im->rgb = malloc(total);
    if (!im->rgb || fread(im->rgb, 1, total, f) != total)
        return "archivo PPM incompleto";
    return NULL;
}

static const char *leer_imagen(const char *ruta, Imagen *im)
{
    FILE *f = fopen(ruta, "rb");
    const char *error;
    int c1, c2;
    if (!f) return "no se pudo abrir el archivo";
    c1 = fgetc(f); c2 = fgetc(f);
    rewind(f);
    if (c1 == 'B' && c2 == 'M')      error = leer_bmp(f, im);
    else if (c1 == 'P' && c2 == '6') error = leer_ppm(f, im);
    else if (c1 == 0xFF && c2 == 0xD8)
        error = "es un JPG: conviértalo primero a BMP (vea \"Preparar la imagen\" en el enunciado)";
    else if (c1 == 0x89 && c2 == 'P')
        error = "es un PNG: conviértalo primero a BMP (vea \"Preparar la imagen\" en el enunciado)";
    else
        error = "formato desconocido: use BMP de 24 bits o PPM";
    fclose(f);
    return error;
}

/* Mismo cálculo que debe hacer el programa en ensamblador para pasar de
 * RGB332 a 0x00RRGGBB: cada canal se corre a los bits altos de su byte. */
static unsigned long rgb332_a_rgb(unsigned p)
{
    unsigned long r = (p >> 5) & 7, g = (p >> 2) & 7, b = p & 3;
    return (r << 21) | (g << 13) | (b << 6);
}

static void uso(const char *nombre)
{
    fprintf(stderr,
        "uso: %s [-n N] [-e] [-l NOMBRE] [-v] imagen.bmp > imagen.asm\n"
        "  -n N       tamaño N x N (1 a %d, por defecto %d)\n"
        "  -e         estirar en lugar de recortar el centro\n"
        "  -l NOMBRE  etiqueta en .data (por defecto IMAGEN)\n"
        "  -v         vista previa en la terminal\n",
        nombre, MAX_LADO, MAX_LADO);
}

int main(int argc, char **argv)
{
    const char *ruta = NULL, *etiqueta = "IMAGEN", *error;
    int lado = MAX_LADO, estirar = 0, vista = 0, i;
    int x0, y0, an_rec, al_rec;     /* rectángulo de la imagen original que se usa */
    unsigned char salida[MAX_LADO * MAX_LADO];
    Imagen im = {0, 0, NULL};

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc)
            lado = atoi(argv[++i]);
        else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc)
            etiqueta = argv[++i];
        else if (strcmp(argv[i], "-e") == 0)
            estirar = 1;
        else if (strcmp(argv[i], "-v") == 0)
            vista = 1;
        else if (argv[i][0] == '-' || ruta) {
            uso(argv[0]);
            return 1;
        } else
            ruta = argv[i];
    }
    if (!ruta || lado < 1 || lado > MAX_LADO) {
        uso(argv[0]);
        return 1;
    }
    error = leer_imagen(ruta, &im);
    if (error) {
        fprintf(stderr, "%s: %s\n", ruta, error);
        free(im.rgb);
        return 1;
    }

    /* Recorte: el cuadrado más grande centrado (o la imagen completa) */
    if (estirar || im.ancho == im.alto) {
        x0 = 0; y0 = 0; an_rec = im.ancho; al_rec = im.alto;
    } else if (im.ancho > im.alto) {
        an_rec = al_rec = im.alto; x0 = (im.ancho - im.alto) / 2; y0 = 0;
    } else {
        an_rec = al_rec = im.ancho; x0 = 0; y0 = (im.alto - im.ancho) / 2;
    }

    /* Reducción por promedio de área y cuantización a RGB332 */
    for (int y = 0; y < lado; y++) {
        for (int x = 0; x < lado; x++) {
            int ax = x0 + x * an_rec / lado, bx = x0 + (x + 1) * an_rec / lado;
            int ay = y0 + y * al_rec / lado, by = y0 + (y + 1) * al_rec / lado;
            double suma[3] = {0, 0, 0};
            long n = 0;
            unsigned r, g, b;
            if (bx <= ax) bx = ax + 1;           /* imagen más pequeña que N x N */
            if (by <= ay) by = ay + 1;
            for (int yy = ay; yy < by && yy < im.alto; yy++)
                for (int xx = ax; xx < bx && xx < im.ancho; xx++) {
                    const unsigned char *p = im.rgb + 3 * ((size_t)yy * im.ancho + xx);
                    suma[0] += p[0]; suma[1] += p[1]; suma[2] += p[2];
                    n++;
                }
            r = (unsigned)(suma[0] / n * 7 / 255 + 0.5);
            g = (unsigned)(suma[1] / n * 7 / 255 + 0.5);
            b = (unsigned)(suma[2] / n * 3 / 255 + 0.5);
            salida[y * lado + x] = (unsigned char)((r << 5) | (g << 2) | b);
        }
    }

    printf("# Imagen generada con img_a_arquisim a partir de '%s' (%d x %d)\n",
           ruta, im.ancho, im.alto);
    printf("# %d x %d píxeles, 1 byte por píxel en formato RGB332 (RRRGGGBB) = %d bytes\n",
           lado, lado, lado * lado);
    printf("# El píxel (x, y) está en la dirección %s + %d*y + x\n", etiqueta, lado);
    printf(".data\n%s:\n", etiqueta);
    for (int y = 0; y < lado; y++) {
        printf("        .byte ");
        for (int x = 0; x < lado; x++)
            printf("0x%02X%s", salida[y * lado + x], x < lado - 1 ? ", " : "");
        printf("    # fila %d\n", y);
    }

    if (vista) {       /* 2 espacios con color de fondo por píxel */
        fprintf(stderr, "Vista previa (%d x %d, RGB332 expandido como en ArquiSim):\n", lado, lado);
        for (int y = 0; y < lado; y++) {
            for (int x = 0; x < lado; x++) {
                unsigned long c = rgb332_a_rgb(salida[y * lado + x]);
                fprintf(stderr, "\033[48;2;%lu;%lu;%lum  ", (c >> 16) & 255, (c >> 8) & 255, c & 255);
            }
            fprintf(stderr, "\033[0m\n");
        }
    }
    free(im.rgb);
    return 0;
}
