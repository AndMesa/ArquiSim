#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
probar.py -- Corre proyecto.asm en ArquiSim (con harness.c) y lo compara con referencia.py.

Uso (desde cualquier carpeta):
  python probar.py                  las 12 combinaciones del enunciado
  python probar.py --todas          las 256 combinaciones de botones
  python probar.py --tabla          imprime la tabla de contadores (para el informe)
  python probar.py --asm otro.asm   prueba otro archivo
  python probar.py --etapa N        pruebas parciales usadas durante el desarrollo:
        1  solo la original en la mitad izquierda
        2  las dos mitades iguales (filtro identidad)
        3  botones + negativo + grises (pantalla de 16 colores)
        4  pantalla RGB sin ampliar (256 colores en 0x608)
        5  pantalla RGB ampliada 2 x 2, sin filtro 3
        6  programa completo (lo mismo que sin --etapa)

Qué revisa en cada combinación:
  - memoria de video 0x400-0x5FF y secuencia de colores escritos en 0x608
  - cada llamada a función: ($a0 -> $v0) contra la misma función del oráculo
  - que la imagen no se modificó, que los botones se leyeron UNA vez, que la posición
    0x604 se escribió una sola vez (solo en modo RGB) y que $sp volvió a 0x400
  - que no hubo excepción ni se llegó al límite de 100000 instrucciones
"""
import argparse
import json
import os
import subprocess
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
ENTREGA = os.path.dirname(AQUI)
sys.path.insert(0, ENTREGA)
import referencia as ref  # noqa: E402

FUENTES = ["ciclo.c", "cpu.c", "isa.c", "ensamblador.c", "sistema.c", "ejecutar.c"]


def compilar_harness(sim):
    exe = os.path.join(AQUI, "harness.exe" if os.name == "nt" else "harness")
    fuentes = [os.path.join(sim, "src", f) for f in FUENTES]
    origen = os.path.join(AQUI, "harness.c")
    if os.path.exists(exe) and all(os.path.getmtime(exe) >= os.path.getmtime(f) for f in fuentes + [origen]):
        return exe
    cmd = ["gcc", "-std=c99", "-O2", "-I" + os.path.join(sim, "src"), "-o", exe, origen] + fuentes
    subprocess.run(cmd, check=True, stderr=subprocess.DEVNULL)
    return exe


def correr(exe, sim, asm, botones, llamadas=True):
    cmd = [exe, os.path.join(sim, "isa.txt"), asm, str(botones), "100000"]
    if llamadas:
        cmd.append("--llamadas")
    r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8")
    if not r.stdout.strip():
        raise SystemExit(r.stderr)
    return json.loads(r.stdout)


def esperado_etapa(imagen, botones, etapa):
    """Lo que debe verse en cada etapa del desarrollo."""
    if etapa == 1:                       # solo la mitad izquierda
        video = bytearray(ref.pantalla_16(imagen, 0))
        for y in range(16):
            video[32 * y + 16:32 * y + 32] = bytes(16)
        return bytes(video), []
    if etapa == 2:                       # filtro identidad: los botones no importan
        return ref.pantalla_16(imagen, 0), []
    if etapa == 3:                       # negativo y grises en 16 colores
        return ref.pantalla_16(imagen, botones & 0x03), []
    if etapa == 4:                       # RGB sin ampliar: 256 colores seguidos
        if botones & ref.BOTON_RGB:
            return bytes(512), [ref.a_rgb(ref.aplicar_filtros(imagen[y][x], botones & 0x03))
                                for y in range(16) for x in range(16)]
        return ref.pantalla_16(imagen, botones & 0x03), []
    if etapa == 5:                       # RGB 2 x 2, todavía sin filtro 3
        return ref.esperado(imagen, botones & 0x83)
    return ref.esperado(imagen, botones)


ORACULO_FUNCION = {
    "A_16COLORES": lambda a0, b: ref.a_16colores(a0),
    "A_RGB": lambda a0, b: ref.a_rgb(a0),
    "BRILLO": lambda a0, b: ref.brillo(a0),
}


def revisar(d, imagen, botones, etapa):
    """Devuelve la lista de problemas encontrados en una corrida."""
    video, rgb = esperado_etapa(imagen, botones, etapa)
    plano = bytes(v for fila in imagen for v in fila)
    problemas = []

    def exigir(cond, texto):
        if not cond:
            problemas.append(texto)

    exigir(d["excepcion"] == "", f"excepción: {d['excepcion']}")
    exigir(d["limite"] == 0, "se alcanzó el límite de 100000 instrucciones")
    exigir(d["num_instr"] <= 128, f"{d['num_instr']} instrucciones: no cabe en .text")
    exigir(bytes.fromhex(d["imagen"]) == plano, "la imagen de .data fue modificada")
    exigir(d["escrituras_imagen"] == 0, "el programa escribió sobre la imagen")
    exigir(d["sp_final"] == 0x400, f"$sp terminó en 0x{d['sp_final']:X}")
    exigir(d["botones_final"] == botones, "los botones cambiaron")
    obt = bytes.fromhex(d["video"])
    if obt != video:
        dif = [i for i in range(512) if obt[i] != video[i]]
        i = dif[0]
        problemas.append(f"video: {len(dif)} bytes distintos; el primero en (x={i % 32}, y={i // 32}): "
                         f"obtenido {obt[i]}, esperado {video[i]}")
    if d["rgb_escrituras"] != rgb:
        n = min(len(rgb), len(d["rgb_escrituras"]))
        dif = [i for i in range(n) if d["rgb_escrituras"][i] != rgb[i]]
        txt = f"0x608: {len(d['rgb_escrituras'])} escrituras (esperadas {len(rgb)})"
        if dif:
            i = dif[0]
            txt += f"; la #{i} es 0x{d['rgb_escrituras'][i]:06X} y debía ser 0x{rgb[i]:06X}"
        problemas.append(txt)
    if etapa >= 3:
        exigir(d["lecturas_botones"] == 1, f"los botones se leyeron {d['lecturas_botones']} veces")
    if etapa >= 4:
        debe = 1 if botones & ref.BOTON_RGB else 0
        exigir(d["escrituras_pos"] == debe, f"0x604 se escribió {d['escrituras_pos']} veces")
    if etapa >= 5 and botones & ref.BOTON_RGB:
        exigir(d["rgb_final"] == rgb, "la imagen final de la pantalla RGB no coincide")
    if etapa >= 6:                       # cada función por separado, contra el oráculo
        for nombre, a0, v0 in d["llamadas"]:
            if nombre == "APLICAR_FILTROS":
                debe = ref.aplicar_filtros(a0, botones)
            elif nombre in ORACULO_FUNCION:
                debe = ORACULO_FUNCION[nombre](a0, botones)
            else:
                continue
            if v0 != debe:
                problemas.append(f"{nombre}(0x{a0:02X}) devolvió 0x{v0:X} y debía ser 0x{debe:X}")
                break
    return problemas


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--asm", default=os.path.join(ENTREGA, "proyecto.asm"))
    ap.add_argument("--sim", default=os.path.dirname(ENTREGA), help="carpeta del simulador (con src/ e isa.txt)")
    ap.add_argument("--etapa", type=int, default=6)
    ap.add_argument("--todas", action="store_true")
    ap.add_argument("--tabla", action="store_true", help="tabla de contadores en Markdown")
    ap.add_argument("--json", help="guarda los contadores de cada combinación")
    args = ap.parse_args()
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")

    exe = compilar_harness(args.sim)
    imagen = ref.leer_imagen(args.asm)
    combos = list(range(256)) if args.todas else ref.COMBINACIONES
    fallas = 0
    filas = []
    for b in combos:
        d = correr(exe, args.sim, args.asm, b)
        problemas = revisar(d, imagen, b, args.etapa)
        fallas += bool(problemas)
        filas.append(d)
        if problemas or not args.todas:
            print(f"0x{b:02X}  {'PASA ' if not problemas else 'FALLA'}  {d['pasos']:6d} instrucciones"
                  f"   R {d['R']:5d}  I {d['I']:5d}  J {d['J']:5d}   lect {d['lecturas']:5d}  escr {d['escrituras']:5d}"
                  f"  saltos {d['saltos']:5d}")
        for p in problemas[:4]:
            print("      " + p)
    d = filas[0]
    print(f"\n{len(combos) - fallas} de {len(combos)} combinaciones pasan (etapa {args.etapa}).  "
          f".text: {d['num_instr']} instrucciones = {d['text_fin']} bytes de 512.  "
          f"Peor caso: {max(f['pasos'] for f in filas)} instrucciones.")

    if args.tabla:
        print("\n| Botones | Instrucciones | R | % R | I | % I | J | % J | Lecturas | Escrituras | Saltos tomados |")
        print("|---|---|---|---|---|---|---|---|---|---|---|")
        for f in filas:
            n = f["pasos"]
            print(f"| 0x{f['botones']:02X} | {n} | {f['R']} | {100 * f['R'] / n:.1f} % | {f['I']} | {100 * f['I'] / n:.1f} % "
                  f"| {f['J']} | {100 * f['J'] / n:.1f} % | {f['lecturas']} | {f['escrituras']} | {f['saltos']} |")
    if args.json:
        claves = ["botones", "pasos", "R", "I", "J", "lecturas", "escrituras", "saltos", "num_instr",
                  "text_fin", "data_fin", "sp_min", "lecturas_botones", "escrituras_pos"]
        with open(args.json, "w", encoding="utf-8") as f:
            json.dump([{k: fila[k] for k in claves} for fila in filas], f, indent=1)
    return 1 if fallas else 0


if __name__ == "__main__":
    sys.exit(main())
