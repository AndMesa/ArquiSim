#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
probar_modificaciones.py -- Comprueba en el simulador las "modificaciones en vivo" de
GUIA_SUSTENTACION.md. Para cada una: aplica el cambio a una copia de proyecto.asm, la
corre con varias combinaciones de botones y compara con lo que debería verse.

Uso:  python probar_modificaciones.py            prueba todas
      python probar_modificaciones.py --guardar  además deja cada versión en modificaciones/
"""
import os
import sys
import tempfile

AQUI = os.path.dirname(os.path.abspath(__file__))
ENTREGA = os.path.dirname(AQUI)
SIM = os.path.dirname(ENTREGA)
sys.path.insert(0, AQUI)
sys.path.insert(0, ENTREGA)
import referencia as ref  # noqa: E402
from probar import compilar_harness, correr  # noqa: E402


def codigo(linea):
    """La instrucción de una línea: sin comentario, sin etiqueta y con espacios simples ('' si no hay)."""
    c = " ".join(linea.split("#")[0].split())
    if ":" in c:
        c = c.split(":", 1)[1].strip()
    return c


def cambiar(lineas, viejo, nuevo):
    """Reemplaza la secuencia de instrucciones 'viejo' (se ignoran comentarios) por 'nuevo'."""
    idx = [i for i, l in enumerate(lineas) if codigo(l)]
    for k in range(len(idx) - len(viejo) + 1):
        if [codigo(lineas[i]) for i in idx[k:k + len(viejo)]] == [" ".join(v.split()) for v in viejo]:
            return lineas[:idx[k]] + nuevo + lineas[idx[k + len(viejo) - 1] + 1:]
    raise SystemExit("no se encontró: " + " / ".join(viejo))


# ----------------------------------------------------------------------------
# Cada modificación: lista de (instrucciones viejas, instrucciones nuevas)
# y la función que dice qué debe verse: esperado(imagen, botones) -> (video, rgb_final)
# ----------------------------------------------------------------------------
def rgb_2x2(f):
    return lambda img, b: [ref.a_rgb(f(img, x // 2, y // 2, b)) for y in range(32) for x in range(32)]


def normal(img, x, y, b):
    return ref.aplicar_filtros(img[y][x], b)


def video16(izq, der):
    def f(img, b):
        v = bytearray(512)
        for y in range(16):
            for x in range(16):
                v[32 * y + x] = ref.a_16colores(izq(img, x, y, b))
                v[32 * y + 16 + x] = ref.a_16colores(der(img, x, y, b))
        return bytes(v)
    return f


def original(img, x, y, b):
    return img[y][x]


def swap_rg(p):
    r, g, bl = ref.canales(p)
    return ref.armar(g, r, bl)


def filtros_con(f3=None, orden="123"):
    def f(img, x, y, b):
        p = img[y][x]
        for paso in orden:
            if paso == "1" and b & 1:
                p = ref.negativo(p)
            if paso == "2" and b & 2:
                p = ref.grises(p)
            if paso == "3" and b & 4:
                p = f3(p) if f3 else ref.filtro3(p, b)
        return p
    return f


def rgb_1x1(img, b):
    final = [0] * 1024
    for y in range(16):
        for x in range(16):
            final[32 * y + x] = ref.a_rgb(ref.aplicar_filtros(img[y][x], b))
    return final


MODIFICACIONES = [
    ("1_original_a_la_derecha",
     [(["jal A_16COLORES", "sb $v0, 0x410($s3)"],        # primero la segunda escritura (la única con 0x410)
       ["        jal  A_16COLORES", "        sb   $v0, 0x400($s3)        # CAMBIO: la filtrada ahora va a la izquierda"]),
      (["sb $v0, 0x400($s3)"], ["        sb   $v0, 0x410($s3)        # CAMBIO: la original ahora va a la derecha (x + 16)"])],
     video16(normal, original), rgb_2x2(normal)),

    ("2_espejo_horizontal",
     [(["jal A_16COLORES", "sb $v0, 0x410($s3)"],
       ["        jal  A_16COLORES",
        "        xori $t1, $s1, 15           # CAMBIO: $t1 = 15 - x (invertir los 4 bits de x)",
        "        sll  $t0, $s0, 5            # CAMBIO: 32*y",
        "        add  $t0, $t0, $t1          # CAMBIO: 32*y + (15 - x)",
        "        sb   $v0, 0x410($t0)        # CAMBIO: mitad derecha, columna espejada"]),
      (["srl $t1, $s1, 1"],
       ["        srl  $t1, $s1, 1",
        "        xori $t1, $t1, 15           # CAMBIO: leer la columna 15 - x/2 de la imagen"])],
     video16(original, lambda img, x, y, b: normal(img, 15 - x, y, b)),
     rgb_2x2(lambda img, x, y, b: normal(img, 15 - x, y, b))),

    ("3a_filtro3_intercambia_rojo_y_verde",
     [(["move $a0, $v0", "jal BRILLO", "andi $t0, $a1, 0x18", "add $t0, $t0, $v0", "lbu $v0, PALETAS($t0)"],
       ["        andi $t0, $v0, 0xE0         # CAMBIO: R (bits 7-5)",
        "        srl  $t0, $t0, 3            # CAMBIO: R baja al lugar de G (bits 4-2)",
        "        andi $t1, $v0, 0x1C         # CAMBIO: G (bits 4-2)",
        "        sll  $t1, $t1, 3            # CAMBIO: G sube al lugar de R (bits 7-5)",
        "        andi $v0, $v0, 0x03         # CAMBIO: B se queda donde está",
        "        or   $v0, $v0, $t0          # CAMBIO: juntar",
        "        or   $v0, $v0, $t1          # CAMBIO: juntar"])],
     video16(original, filtros_con(swap_rg)), rgb_2x2(filtros_con(swap_rg))),

    ("3b_filtro3_solo_canal_rojo",
     [(["move $a0, $v0", "jal BRILLO", "andi $t0, $a1, 0x18", "add $t0, $t0, $v0", "lbu $v0, PALETAS($t0)"],
       ["        andi $v0, $v0, 0xE0         # CAMBIO: dejar solo el rojo (borra G y B)"])],
     video16(original, filtros_con(lambda p: p & 0xE0)), rgb_2x2(filtros_con(lambda p: p & 0xE0))),

    ("4_rgb_sin_ampliar",
     [(["sw $zero, 0x604($zero)", "li $s0, 0", "li $s1, 0", "srl $t0, $s0, 1", "sll $t0, $t0, 4",
        "srl $t1, $s1, 1", "add $t0, $t0, $t1"],
       ["RGB:    li   $s0, 0                 # y = 0",
        "R_FILA: sll  $t0, $s0, 16           # CAMBIO: posición = (y << 16) | 0: inicio de la fila y",
        "        sw   $t0, 0x604($zero)      # CAMBIO: se escribe al empezar CADA fila (16 veces)",
        "        li   $s1, 0                 # x = 0",
        "R_COL:  sll  $t0, $s0, 4            # CAMBIO: 16*y (ya no se divide por 2)",
        "        add  $t0, $t0, $s1          # CAMBIO: 16*y + x"]),
      (["slti $t0, $s1, 32"], ["        slti $t0, $s1, 16           # CAMBIO: solo 16 columnas"]),
      (["slti $t0, $s0, 32"], ["        slti $t0, $s0, 16           # CAMBIO: solo 16 filas"])],
     video16(original, normal), rgb_1x1),

    ("5_orden_grises_negativo_mapa",
     [(["andi $t0, $a1, 0x01", "beq $t0, $zero, F_GRIS", "xori $v0, $v0, 0xFF",
        "andi $t0, $a1, 0x02", "beq $t0, $zero, F_MAPA", "move $a0, $v0", "jal BRILLO",
        "sll $t0, $v0, 5", "sll $t1, $v0, 2", "or $t0, $t0, $t1", "srl $t1, $v0, 1", "or $v0, $t0, $t1"],
       ["        andi $t0, $a1, 0x02         # CAMBIO: primero el filtro 2 (grises)",
        "        beq  $t0, $zero, F_NEG      # CAMBIO: si está apagado sigue con el negativo",
        "        move $a0, $v0",
        "        jal  BRILLO",
        "        sll  $t0, $v0, 5",
        "        sll  $t1, $v0, 2",
        "        or   $t0, $t0, $t1",
        "        srl  $t1, $v0, 1",
        "        or   $v0, $t0, $t1",
        "F_NEG:  andi $t0, $a1, 0x01         # CAMBIO: después el filtro 1 (negativo)",
        "        beq  $t0, $zero, F_MAPA     # CAMBIO: si está apagado sigue con el mapa",
        "        xori $v0, $v0, 0xFF"])],
     video16(original, filtros_con(orden="213")), rgb_2x2(filtros_con(orden="213"))),
    ("2b_espejo_vertical",
     [(["jal A_16COLORES", "sb $v0, 0x410($s3)"],
       ["        jal  A_16COLORES",
        "        xori $t1, $s0, 15           # CAMBIO: $t1 = 15 - y",
        "        sll  $t0, $t1, 5            # CAMBIO: 32*(15 - y)",
        "        add  $t0, $t0, $s1          # CAMBIO: + x",
        "        sb   $v0, 0x410($t0)        # CAMBIO: mitad derecha, fila espejada"]),
      (["srl $t0, $s0, 1"],
       ["R_COL:  srl  $t0, $s0, 1",
        "        xori $t0, $t0, 15           # CAMBIO: leer la fila 15 - y/2 de la imagen"])],
     video16(original, lambda img, x, y, b: normal(img, x, 15 - y, b)),
     rgb_2x2(lambda img, x, y, b: normal(img, x, 15 - y, b))),

    ("6a_boton_6_activa_el_negativo",
     [(["andi $t0, $a1, 0x01"], ["        andi $t0, $a1, 0x40         # CAMBIO: botón 6 en vez del 0"])],
     video16(original, lambda img, x, y, b: normal(img, x, y, (b & 0xFE) | ((b >> 6) & 1))),
     rgb_2x2(lambda img, x, y, b: normal(img, x, y, (b & 0xFE) | ((b >> 6) & 1)))),

    ("6c_solo_mitad_de_arriba",
     [(["slti $t0, $s0, 16"], ["        slti $t0, $s0, 8            # CAMBIO: solo 8 filas"])],
     lambda img, b: video16(original, normal)(img, b)[:256] + bytes(256), rgb_2x2(normal)),

    ("6d_paleta_con_botones_5_y_6",
     [(["andi $t0, $a1, 0x18"],
       ["        andi $t0, $a1, 0x60         # CAMBIO: botones 5 y 6 (valen 32 y 64)",
        "        srl  $t0, $t0, 2            # CAMBIO: /4 para que valga 0, 8, 16 o 24"])],
     video16(original, lambda img, x, y, b: normal(img, x, y, (b & 0x87) | ((b & 0x60) >> 2))),
     rgb_2x2(lambda img, x, y, b: normal(img, x, y, (b & 0x87) | ((b & 0x60) >> 2)))),
]


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    exe = compilar_harness(SIM)
    with open(os.path.join(ENTREGA, "proyecto.asm"), encoding="utf-8") as f:
        base = f.read().split("\n")
    imagen = ref.leer_imagen(os.path.join(ENTREGA, "proyecto.asm"))
    guardar = "--guardar" in sys.argv
    combos = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x0C, 0x14, 0x1F,
              0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x8C, 0x94, 0x9F,
              0x24, 0x44, 0x64, 0x41, 0x47, 0xA4, 0xC4, 0xE4, 0xC1, 0xC7]
    todo_bien = True
    for nombre, cambios, f_video, f_rgb in MODIFICACIONES:
        lineas = list(base)
        for viejo, nuevo in cambios:
            lineas = cambiar(lineas, viejo, nuevo)
        if guardar:
            os.makedirs(os.path.join(ENTREGA, "modificaciones"), exist_ok=True)
            ruta = os.path.join(ENTREGA, "modificaciones", nombre + ".asm")
            f = open(ruta, "w", encoding="utf-8", newline="\n")
        else:
            f = tempfile.NamedTemporaryFile("w", suffix=".asm", delete=False, encoding="utf-8", newline="\n")
            ruta = f.name
        f.write("\n".join(lineas))
        f.close()
        fallas, n = [], 0
        for b in combos:
            d = correr(exe, SIM, ruta, b, llamadas=False)
            n = d["num_instr"]
            if d["excepcion"] or d["limite"]:
                fallas.append(f"0x{b:02X}: {d['excepcion'] or 'límite de instrucciones'}")
            elif b & 0x80 and d["rgb_final"] != f_rgb(imagen, b):
                fallas.append(f"0x{b:02X}: la pantalla RGB no coincide")
            elif not b & 0x80 and bytes.fromhex(d["video"]) != f_video(imagen, b):
                fallas.append(f"0x{b:02X}: la pantalla de 16 colores no coincide")
        if not guardar:
            os.unlink(ruta)
        todo_bien = todo_bien and not fallas
        print(f"{'PASA ' if not fallas else 'FALLA'} {nombre:40s} {n:3d} instrucciones  {'; '.join(fallas[:3])}")
    return 0 if todo_bien else 1


if __name__ == "__main__":
    sys.exit(main())
