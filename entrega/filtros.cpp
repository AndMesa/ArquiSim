// filtros.cpp -- El mismo algoritmo de proyecto.asm, escrito en C++.
//
// La imagen es un arreglo de 16 x 16 bytes (RGB332) y la pantalla de 16 colores un
// arreglo de 16 x 32 bytes, como pide la pregunta 9 del enunciado. La pantalla RGB
// (que en ArquiSim vive dentro del dispositivo) es un arreglo de 32 x 32 palabras.
//
// Compilar:   g++ -std=c++11 -O2 -o filtros filtros.cpp
// Usar:       ./filtros 0x81             dibuja el resultado en la terminal (con colores)
//             ./filtros 0x81 --volcado   imprime el mismo texto que  python referencia.py --volcado 0x81
//
// El argumento son los botones (lo que en ArquiSim se escribe en mem[0x600]).

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// >>> IMAGEN (la misma de proyecto.asm)
const uint8_t IMAGEN[16][16] = {
    {0xFF, 0xFF, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xE0, 0xE4, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE4, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xE9, 0xE5, 0xC4, 0xA0, 0xA0, 0xA0, 0xE0, 0xE4, 0xE4, 0xE0, 0xE0, 0xE9, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xF2, 0xE0, 0xC4, 0x85, 0x44, 0xA4, 0xE4, 0xE4, 0xE4, 0xE4, 0xE0, 0xF2, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xF2, 0xE0, 0x65, 0x44, 0x65, 0x65, 0xE0, 0xE4, 0xE4, 0xE4, 0xE0, 0xF2, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xF2, 0xE0, 0x89, 0x45, 0x24, 0x69, 0x84, 0xE4, 0xE4, 0xE4, 0xE0, 0xF6, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xF2, 0xC4, 0x49, 0x49, 0x45, 0x89, 0x69, 0xA4, 0xC4, 0xC0, 0xE0, 0xF6, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xF2, 0xE0, 0xA4, 0x65, 0x65, 0x84, 0xA5, 0x69, 0x89, 0x85, 0xE0, 0xF6, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xF2, 0xE0, 0xE4, 0xA5, 0x84, 0xE0, 0xA8, 0x88, 0x88, 0xA8, 0xE0, 0xF6, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xF2, 0xE0, 0xE4, 0xA4, 0x84, 0xC0, 0x89, 0x6D, 0x6D, 0x68, 0x84, 0xD6, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xF6, 0xE4, 0xA4, 0xE4, 0x65, 0x44, 0x8D, 0xB1, 0xD5, 0x71, 0x91, 0xDA, 0xFF, 0xFF},
    {0xFF, 0xFF, 0x92, 0x00, 0x20, 0x24, 0x69, 0xD1, 0xD5, 0x8D, 0xA8, 0x68, 0x95, 0xDA, 0xFF, 0xFF},
    {0xFF, 0xFF, 0x92, 0x00, 0x20, 0x24, 0xAD, 0xCC, 0xAD, 0xAD, 0x84, 0x90, 0xAC, 0xD6, 0xFF, 0xFF},
    {0xFF, 0xFF, 0x6D, 0x00, 0x40, 0x00, 0x24, 0x68, 0x68, 0x88, 0xAC, 0x88, 0x88, 0xB1, 0xFF, 0xFF},
    {0xFF, 0xFF, 0x24, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xFF, 0xFF},
};
// <<< IMAGEN

// >>> ALGORITMO (esta parte es la que se compara con las 105 instrucciones del ensamblador)
const uint8_t PALETAS[4][8] = {
    {0x00, 0x02, 0x43, 0xA1, 0xE0, 0xF0, 0xFC, 0xFF},   // 0: térmica
    {0x00, 0x24, 0x44, 0x68, 0x8D, 0xB1, 0xD1, 0xF6},   // 1: sepia
    {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF},   // 2: umbral blanco/negro
    {0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C},   // 3: visión nocturna
};

uint8_t  pantalla[16][32];        // pantalla de 16 colores: en ArquiSim es la memoria 0x400-0x5FF
uint32_t pantallaRGB[32][32];     // pantalla RGB: en ArquiSim vive dentro del dispositivo

// Y = (R + 2G + 2B + 2) / 4, de 0 a 7
uint8_t brillo(uint8_t p) {
    int R = p >> 5, G = (p >> 2) & 7, B = p & 3;
    return (R + 2 * G + 2 * B + 2) >> 2;
}

// Aplica los filtros activos en orden 1 -> 2 -> 3
uint8_t aplicarFiltros(uint8_t p, uint8_t botones) {
    if (botones & 0x01)                                   // filtro 1: negativo
        p = p ^ 0xFF;
    if (botones & 0x02) {                                 // filtro 2: escala de grises
        uint8_t Y = brillo(p);
        p = (Y << 5) | (Y << 2) | (Y >> 1);
    }
    if (botones & 0x04)                                   // filtro 3: mapa de color
        p = PALETAS[(botones >> 3) & 3][brillo(p)];
    return p;
}

// Píxel RGB332 -> color 0-15 de la pantalla de 16 colores
uint8_t a16Colores(uint8_t p) {
    return ((p >> 7) & 1) | ((p >> 3) & 2) | ((p << 1) & 4) | (((p >> 3) | p) & 8);
}

// Píxel RGB332 -> color 0x00RRGGBB de la pantalla RGB
uint32_t aRGB(uint8_t p) {
    return ((p & 0xE0u) << 16) | ((p & 0x1Cu) << 11) | ((p & 0x03u) << 6);
}

// Botón 7 = 0: original a la izquierda, filtrada a la derecha
void dibujar16Colores(uint8_t botones) {
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            pantalla[y][x] = a16Colores(IMAGEN[y][x]);
            pantalla[y][x + 16] = a16Colores(aplicarFiltros(IMAGEN[y][x], botones));
        }
}

// Botón 7 = 1: solo la filtrada, ampliada 2 x 2
void dibujarRGB(uint8_t botones) {
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++)
            pantallaRGB[y][x] = aRGB(aplicarFiltros(IMAGEN[y / 2][x / 2], botones));
}
// <<< ALGORITMO

// ---------------------------------------------------------------------------------
// Lo que sigue no existe en el ensamblador: en ArquiSim la pantalla la muestra el simulador.
// ---------------------------------------------------------------------------------

// Texto canónico, idéntico al de referencia.py --volcado
void volcar(uint8_t botones, bool rgb) {
    printf("botones=0x%02X\nvideo:\n", botones);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 32; x++)
            printf("%02X", pantalla[y][x]);
        printf("\n");
    }
    printf("rgb: %d\n", rgb ? 1024 : 0);
    for (int y = 0; rgb && y < 32; y++)
        for (int x = 0; x < 32; x++)
            printf("%06X%s", (unsigned)pantallaRGB[y][x], x < 31 ? " " : "\n");
}

// Vista en la terminal: 2 espacios con color de fondo por píxel
void mostrar(bool rgb) {
    static const int PALETA16[16][3] = {
        {0, 0, 0}, {170, 0, 0}, {0, 170, 0}, {170, 170, 0}, {0, 0, 170}, {170, 0, 170}, {0, 170, 170}, {170, 170, 170},
        {85, 85, 85}, {255, 85, 85}, {85, 255, 85}, {255, 255, 85}, {85, 85, 255}, {255, 85, 255}, {85, 255, 255}, {255, 255, 255}};
    for (int y = 0; y < (rgb ? 32 : 16); y++) {
        for (int x = 0; x < 32; x++) {
            if (rgb) {
                uint32_t c = pantallaRGB[y][x];
                printf("\033[48;2;%u;%u;%um  ", (unsigned)(c >> 16) & 255, (unsigned)(c >> 8) & 255, (unsigned)c & 255);
            } else {
                const int *c = PALETA16[pantalla[y][x]];
                printf("\033[48;2;%d;%d;%dm  ", c[0], c[1], c[2]);
            }
        }
        printf("\033[0m\n");
    }
}

int main(int argc, char **argv) {
    uint8_t botones = argc > 1 ? (uint8_t)strtol(argv[1], nullptr, 0) : 0;   // lo que sería mem[0x600]
    bool rgb = botones & 0x80;                                               // botón 7: cuál pantalla

    if (rgb)
        dibujarRGB(botones);
    else
        dibujar16Colores(botones);

    if (argc > 2 && strcmp(argv[2], "--volcado") == 0)
        volcar(botones, rgb);
    else
        mostrar(rgb);
    return 0;
}
