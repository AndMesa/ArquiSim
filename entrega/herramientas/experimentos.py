#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
experimentos.py -- Genera y corre los programas pequeños que responden las preguntas de
reflexión con datos del simulador. Los deja en entrega/experimentos/ para poder abrirlos
también en la interfaz de ArquiSim y tomar las capturas.

  endian.asm    pregunta 2: lw $t0, 0($s0) con $s0 = IMAGEN (usa la imagen actual de proyecto.asm)
  botones.asm   pregunta 4: qué pasa al escribir en 0x600
  pisa_s1.asm   pregunta 8: A_16COLORES usa $s1 en lugar de $t0 (una función que daña un $s)

Uso:  python experimentos.py
"""
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
ENTREGA = os.path.dirname(AQUI)
SIM = os.path.dirname(ENTREGA)
sys.path.insert(0, AQUI)
from probar import compilar_harness, correr  # noqa: E402

ENDIAN = """
.text
# Pregunta 2: una palabra de la imagen leída con lw (ArquiSim es big-endian)
        la   $s0, IMAGEN            # $s0 = 0x200: dirección del píxel (0, 0)
        lw   $t0, 0($s0)            # $t0 = los 4 bytes 0x200..0x203 = píxeles (0,0) (1,0) (2,0) (3,0)
        lbu  $t1, 0($s0)            # $t1 = byte de la dirección MENOR (0x200): píxel (0, 0)
        lbu  $t2, 3($s0)            # $t2 = byte de la dirección MAYOR (0x203): píxel (3, 0)
"""

BOTONES = """# Pregunta 4: los botones (0x600) son de solo lectura
# En la interfaz: active antes algunos botones (por ejemplo  bt 0  y  bt 2) y ejecute paso a paso.
.text
        lw   $t0, 0x600($zero)      # $t0 = botones antes
        addi $t1, $zero, 0xFF       # valor que se intenta escribir
        sw   $t1, 0x600($zero)      # intento de escribir una palabra en los botones
        sb   $t1, 0x603($zero)      # intento de escribir solo el byte bajo
        lw   $t2, 0x600($zero)      # $t2 = botones después: igual que $t0
"""


def escribir(nombre, texto):
    carpeta = os.path.join(ENTREGA, "experimentos")
    os.makedirs(carpeta, exist_ok=True)
    ruta = os.path.join(carpeta, nombre)
    with open(ruta, "w", encoding="utf-8", newline="\n") as f:
        f.write(texto)
    return ruta


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    exe = compilar_harness(SIM)
    with open(os.path.join(ENTREGA, "proyecto.asm"), encoding="utf-8") as f:
        fuente = f.read()
    bloque = fuente[fuente.index("# >>> IMAGEN"):fuente.index("# <<< IMAGEN") + len("# <<< IMAGEN")]

    d = correr(exe, SIM, escribir("endian.asm", bloque + "\n" + ENDIAN), 0, llamadas=False)
    img = d["imagen"]
    print("Pregunta 2 (endian.asm)")
    print(f"  bytes en 0x200..0x203: {img[0:2]} {img[2:4]} {img[4:6]} {img[6:8]}")
    print(f"  lw  $t0, 0($s0) -> $t0 = 0x{d['reg'][8]:08X}")
    print(f"  lbu $t1, 0($s0) -> 0x{d['reg'][9]:02X}   lbu $t2, 3($s0) -> 0x{d['reg'][10]:02X}")

    ruta = escribir("botones.asm", BOTONES)
    print("\nPregunta 4 (botones.asm)")
    for b in (0x00, 0x05):
        d = correr(exe, SIM, ruta, b, llamadas=False)
        print(f"  botones = 0x{b:02X}: antes $t0 = 0x{d['reg'][8]:02X}, después $t2 = 0x{d['reg'][10]:02X}, "
              f"mem[0x600] final = 0x{d['botones_final']:02X}, excepción: '{d['excepcion']}', "
              f"escrituras contadas: {d['escrituras']}")

    a, b = fuente.index("\nA_16COLORES:"), fuente.index("\n# ====", fuente.index("\nA_16COLORES:"))
    malo = fuente[:a] + fuente[a:b].replace("$t0", "$s1") + fuente[b:]
    d = correr(exe, SIM, escribir("pisa_s1.asm", malo), 0, llamadas=False)
    video = bytes.fromhex(d["video"])
    pintados = [(i % 32, i // 32) for i in range(512) if video[i]]
    print("\nPregunta 8 (pisa_s1.asm): A_16COLORES usa $s1 como temporal")
    print(f"  instrucciones ejecutadas: {d['pasos']}  (límite alcanzado: {'sí' if d['limite'] else 'no'})")
    print(f"  $s0 (y) = {d['reg'][16]}, $s1 (x) = {d['reg'][17]}")
    print(f"  píxeles pintados (distintos de 0): {len(pintados)} -> {pintados}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
