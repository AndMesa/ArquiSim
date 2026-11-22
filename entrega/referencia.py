#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
referencia.py -- Oráculo del proyecto final (Filtros de imagen en ArquiSim).

Implementa en Python, tal como los define el enunciado, los filtros y las dos
conversiones de color, y con eso calcula lo que DEBE quedar en el simulador:

  * el contenido de la pantalla de 16 colores (memoria 0x400-0x5FF), y
  * la secuencia de colores que el programa debe escribir en 0x608 (pantalla RGB),

para cualquier combinación de botones. Sirve para probar proyecto.asm
(herramientas/probar.py) y filtros.cpp.

Uso:
  python referencia.py                    verifica la Tabla 4 y resume las 12 combinaciones
  python referencia.py --volcado 0x81     imprime lo esperado para esos botones
  python referencia.py --json salida.json guarda lo esperado para las 12 combinaciones
  python referencia.py --todas ...        usa las 256 combinaciones en lugar de las 12
  python referencia.py --conmutan         ¿negativo->grises == grises->negativo? (256 valores)
  python referencia.py --asm ruta.asm     lee la imagen de otro archivo (por defecto proyecto.asm)
"""
import argparse
import hashlib
import json
import os
import re
import sys

# ----------------------------------------------------------------------------
# Constantes del enunciado
# ----------------------------------------------------------------------------
IMAGEN_BASE = 0x200          # 16 x 16 bytes RGB332
VIDEO_BASE = 0x400           # pantalla de 16 colores: 32 x 16 bytes
LADO = 16

BOTON_NEGATIVO = 0x01        # bit 0: filtro 1
BOTON_GRISES = 0x02          # bit 1: filtro 2
BOTON_FILTRO3 = 0x04         # bit 2: filtro 3 (mapa de color)
BOTONES_PALETA = 0x18        # bits 3 y 4: cuál de las 4 paletas usa el filtro 3
BOTON_RGB = 0x80             # bit 7: 0 = pantalla de 16 colores, 1 = pantalla RGB

# Combinaciones mínimas que pide el enunciado (con y sin el bit 7)
COMBINACIONES = [0x00, 0x01, 0x02, 0x03, 0x04, 0x07,
                 0x80, 0x81, 0x82, 0x83, 0x84, 0x87]

# Filtro 3: "mapa de color". El brillo Y (0-7) del píxel es el índice de una
# tabla de 8 colores RGB332. Hay 4 tablas; los botones 3 y 4 eligen cuál.
PALETAS = [
    # Y:  0     1     2     3     4     5     6     7
    [0x00, 0x02, 0x43, 0xA1, 0xE0, 0xF0, 0xFC, 0xFF],   # 0: térmica (negro-azul-morado-rojo-naranja-amarillo-blanco)
    [0x00, 0x24, 0x44, 0x68, 0x8D, 0xB1, 0xD1, 0xF6],   # 1: sepia (tonos café)
    [0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF],   # 2: umbral blanco y negro (Y >= 4 es blanco)
    [0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C],   # 3: visión nocturna (solo canal verde: G = Y)
]
NOMBRES_PALETA = ["térmica", "sepia", "umbral B/N", "visión nocturna"]

# Tabla 4 del enunciado: píxel (x, y) -> valores esperados
TABLA_4 = [
    # (x, y), RGB332, 16col, RGB,      neg,  neg16, gris, gris16
    ((0, 0),  0xFF, 15, 0xE0E0C0, 0x00, 0,  0xFF, 15),
    ((5, 1),  0xE0, 9,  0xE00000, 0x1F, 14, 0x49, 8),
    ((3, 11), 0x00, 0,  0x000000, 0xFF, 15, 0x00, 0),
    ((9, 10), 0xB1, 3,  0xA08040, 0x4E, 12, 0x92, 7),
]


# ----------------------------------------------------------------------------
# Píxel RGB332 = RRRGGGBB
# ----------------------------------------------------------------------------
def canales(p):
    """Devuelve (R, G, B): R y G de 0 a 7, B de 0 a 3."""
    return (p >> 5) & 7, (p >> 2) & 7, p & 3


def armar(r, g, b):
    return (r << 5) | (g << 2) | b


# ----------------------------------------------------------------------------
# Filtros (reciben un píxel RGB332 y devuelven otro)
# ----------------------------------------------------------------------------
def negativo(p):
    """Filtro 1: R' = 7 - R, G' = 7 - G, B' = 3 - B."""
    r, g, b = canales(p)
    return armar(7 - r, 7 - g, 3 - b)


def brillo(p):
    """Y = (R + 2G + 2B + 2) >> 2, de 0 a 7."""
    r, g, b = canales(p)
    return (r + 2 * g + 2 * b + 2) >> 2


def grises(p):
    """Filtro 2: R' = Y, G' = Y, B' = Y >> 1."""
    y = brillo(p)
    return armar(y, y, y >> 1)


def filtro3(p, botones=0):
    """Filtro 3 (mapa de color): el brillo es el índice de la paleta elegida con los botones 3 y 4."""
    paleta = (botones & BOTONES_PALETA) >> 3
    return PALETAS[paleta][brillo(p)]


def aplicar_filtros(p, botones):
    """Aplica los filtros activos en orden 1 -> 2 -> 3."""
    if botones & BOTON_NEGATIVO:
        p = negativo(p)
    if botones & BOTON_GRISES:
        p = grises(p)
    if botones & BOTON_FILTRO3:
        p = filtro3(p, botones)
    return p


# ----------------------------------------------------------------------------
# Conversiones de color
# ----------------------------------------------------------------------------
def a_16colores(p):
    """bit0 <- bit7 (R alto), bit1 <- bit4 (G alto), bit2 <- bit1 (B alto), bit3 <- bit6 OR bit3."""
    rojo = (p >> 7) & 1
    verde = (p >> 4) & 1
    azul = (p >> 1) & 1
    brillante = ((p >> 6) | (p >> 3)) & 1
    return rojo | (verde << 1) | (azul << 2) | (brillante << 3)


def a_rgb(p):
    """color = (R << 21) | (G << 13) | (B << 6)  ->  0x00RRGGBB."""
    r, g, b = canales(p)
    return (r << 21) | (g << 13) | (b << 6)


# ----------------------------------------------------------------------------
# La imagen: se lee del bloque IMAGEN de proyecto.asm
# ----------------------------------------------------------------------------
def leer_imagen(ruta_asm):
    """Devuelve la imagen como lista de 16 filas de 16 bytes, leída de las líneas .byte
    que están entre '# >>> IMAGEN' y '# <<< IMAGEN'."""
    datos = []
    dentro = False
    with open(ruta_asm, encoding="utf-8") as f:
        for linea in f:
            if linea.startswith("# >>> IMAGEN"):
                dentro = True
                continue
            if linea.startswith("# <<< IMAGEN"):
                break
            codigo = linea.split("#")[0].strip()
            if dentro and codigo.startswith(".byte"):
                datos += [int(v, 0) for v in codigo[5:].split(",") if v.strip()]
    if len(datos) != LADO * LADO:
        raise SystemExit(f"{ruta_asm}: se esperaban 256 bytes de imagen y hay {len(datos)}")
    return [datos[LADO * y:LADO * (y + 1)] for y in range(LADO)]


# ----------------------------------------------------------------------------
# Lo que debe quedar en el simulador
# ----------------------------------------------------------------------------
def pantalla_16(imagen, botones):
    """512 bytes de 0x400-0x5FF: original en x 0-15, filtrada en x 16-31."""
    video = bytearray(32 * 16)
    for y in range(LADO):
        for x in range(LADO):
            p = imagen[y][x]
            video[32 * y + x] = a_16colores(p)
            video[32 * y + 16 + x] = a_16colores(aplicar_filtros(p, botones))
    return bytes(video)


def secuencia_rgb(imagen, botones):
    """Los 1024 colores que se escriben en 0x608, en orden: la imagen filtrada ampliada 2 x 2.
    El píxel (x, y) de la pantalla es el píxel (x // 2, y // 2) de la imagen."""
    return [a_rgb(aplicar_filtros(imagen[y // 2][x // 2], botones))
            for y in range(32) for x in range(32)]


def esperado(imagen, botones):
    """Devuelve (video, rgb): con el bit 7 en 0 se dibuja en 0x400 y no se escribe en 0x608;
    con el bit 7 en 1 la memoria de video queda en ceros y se escriben 1024 colores."""
    if botones & BOTON_RGB:
        return bytes(32 * 16), secuencia_rgb(imagen, botones)
    return pantalla_16(imagen, botones), []


def volcado(imagen, botones):
    """Texto canónico (lo mismo que imprime filtros.cpp con --volcado)."""
    video, rgb = esperado(imagen, botones)
    lineas = [f"botones=0x{botones:02X}", "video:"]
    for y in range(16):
        lineas.append(video[32 * y:32 * (y + 1)].hex().upper())
    lineas.append(f"rgb: {len(rgb)}")
    for y in range(len(rgb) // 32):
        lineas.append(" ".join(f"{c:06X}" for c in rgb[32 * y:32 * (y + 1)]))
    return "\n".join(lineas) + "\n"


# ----------------------------------------------------------------------------
# Verificaciones
# ----------------------------------------------------------------------------
def verificar_tabla_4(imagen=None):
    """Compara cada función con la Tabla 4. Si la imagen es la de ejemplo, también
    revisa que el píxel esté en esa posición. Devuelve la cantidad de fallas."""
    fallas = 0

    def revisar(nombre, obtenido, debe):
        nonlocal fallas
        if obtenido != debe:
            fallas += 1
            print(f"  FALLA {nombre}: obtenido 0x{obtenido:X}, esperado 0x{debe:X}")

    for (x, y), p, c16, rgb, neg, neg16, gris, gris16 in TABLA_4:
        revisar(f"a_16colores(0x{p:02X})", a_16colores(p), c16)
        revisar(f"a_rgb(0x{p:02X})", a_rgb(p), rgb)
        revisar(f"negativo(0x{p:02X})", negativo(p), neg)
        revisar(f"a_16colores(negativo(0x{p:02X}))", a_16colores(negativo(p)), neg16)
        revisar(f"grises(0x{p:02X})", grises(p), gris)
        revisar(f"a_16colores(grises(0x{p:02X}))", a_16colores(grises(p)), gris16)
    return fallas


def es_imagen_de_ejemplo(imagen):
    return all(imagen[y][x] == p for (x, y), p, *_ in TABLA_4)


def conmutan():
    """¿negativo->grises da lo mismo que grises->negativo? Prueba los 256 píxeles."""
    distintos = [p for p in range(256) if grises(negativo(p)) != negativo(grises(p))]
    return distintos


def huella(video, rgb):
    """Resumen corto (para comparar a simple vista dos resultados)."""
    h = hashlib.sha1(video + b"".join(c.to_bytes(4, "big") for c in rgb))
    return h.hexdigest()[:10]


def main():
    aqui = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description="Oráculo del proyecto final de ArquiSim")
    ap.add_argument("--asm", default=os.path.join(aqui, "proyecto.asm"),
                    help="archivo del que se lee la imagen (por defecto proyecto.asm)")
    ap.add_argument("--volcado", metavar="BOTONES", help="imprime lo esperado para esos botones (p. ej. 0x81)")
    ap.add_argument("--json", metavar="ARCHIVO", help="guarda lo esperado en un archivo JSON")
    ap.add_argument("--todas", action="store_true", help="las 256 combinaciones de botones, no solo las 12")
    ap.add_argument("--conmutan", action="store_true", help="prueba negativo->grises contra grises->negativo")
    args = ap.parse_args()

    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")

    if args.conmutan:
        d = conmutan()
        print(f"grises(negativo(p)) == negativo(grises(p)) en {256 - len(d)} de 256 píxeles")
        for p in d[:10]:
            print(f"  0x{p:02X}: neg->gris 0x{grises(negativo(p)):02X}, gris->neg 0x{negativo(grises(p)):02X}")
        return 1 if d else 0

    imagen = leer_imagen(args.asm)

    if args.volcado:
        sys.stdout.write(volcado(imagen, int(args.volcado, 0)))
        return 0

    combos = list(range(256)) if args.todas else COMBINACIONES

    if args.json:
        salida = {}
        for b in combos:
            video, rgb = esperado(imagen, b)
            salida[f"0x{b:02X}"] = {"video": video.hex().upper(), "rgb_escrituras": rgb}
        with open(args.json, "w", encoding="utf-8") as f:
            json.dump(salida, f)
        print(f"{args.json}: {len(combos)} combinaciones")
        return 0

    fallas = verificar_tabla_4(imagen)
    print(f"Tabla 4: {'OK (24 valores)' if fallas == 0 else str(fallas) + ' fallas'}")
    print(f"Imagen de {args.asm}: {'es la de ejemplo' if es_imagen_de_ejemplo(imagen) else 'imagen propia'}")
    d = conmutan()
    print(f"negativo->grises == grises->negativo: {'sí, en los 256 píxeles' if not d else f'no ({len(d)} distintos)'}")
    print()
    print("botones  pantalla   bytes video != 0   escrituras 0x608   huella")
    for b in combos:
        video, rgb = esperado(imagen, b)
        print(f"  0x{b:02X}   {'RGB      ' if b & BOTON_RGB else '16 colores'}   "
              f"{sum(1 for v in video if v):6d}          {len(rgb):6d}          {huella(video, rgb)}")
    return 1 if fallas else 0


if __name__ == "__main__":
    sys.exit(main())
