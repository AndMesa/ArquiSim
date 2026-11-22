#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
poner_imagen.py -- Cambia la imagen del proyecto en un solo paso.

  python poner_imagen.py foto.bmp        (también .ppm; .jpg y .png si está instalado Pillow)
  python poner_imagen.py -e foto.bmp     estira en lugar de recortar el centro

Hace lo que describe la sección "Preparar la imagen" del enunciado:
  1. Compila img_a_arquisim (si hace falta) y convierte la imagen (muestra la vista previa -v).
  2. Reemplaza el bloque entre "# >>> IMAGEN" y "# <<< IMAGEN" de proyecto.asm.
  3. Pone la misma imagen en el arreglo IMAGEN de filtros.cpp.
  4. Copia el archivo original y el .bmp a entrega/imagen/ (son entregables).
  5. Corre las 256 pruebas. Si alguna falla, deja proyecto.asm y filtros.cpp como estaban.
  6. Copia proyecto.asm a programas/ del simulador y actualiza experimentos/ y modificaciones/.

La última línea que imprime es "RESULTADO: LISTO ..." o "RESULTADO: FALLO ...".
"""
import os
import re
import shutil
import subprocess
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
ENTREGA = os.path.dirname(AQUI)
SIM = os.path.dirname(ENTREGA)


def reemplazar_bloque(ruta, inicio, fin, nuevo):
    with open(ruta, encoding="utf-8") as f:
        lineas = f.read().split("\n")
    a = next(i for i, l in enumerate(lineas) if l.startswith(inicio))
    b = next(i for i, l in enumerate(lineas) if l.startswith(fin))
    lineas[a:b + 1] = nuevo
    with open(ruta, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lineas))


def main():
    args = [a for a in sys.argv[1:] if a != "-e"]
    estirar = "-e" in sys.argv[1:]
    if len(args) != 1:
        raise SystemExit(__doc__)
    origen = os.path.abspath(args[0])
    carpeta = os.path.join(ENTREGA, "imagen")
    os.makedirs(carpeta, exist_ok=True)
    nombre, ext = os.path.splitext(os.path.basename(origen))

    # el archivo original es un entregable: se copia tal cual
    copia = os.path.join(carpeta, os.path.basename(origen))
    if os.path.abspath(copia) != origen:
        shutil.copyfile(origen, copia)

    # el conversor solo lee BMP de 24/32 bits o PPM: convertir si hace falta
    bmp = copia
    if ext.lower() not in (".bmp", ".ppm"):
        try:
            from PIL import Image
        except ImportError:
            raise SystemExit("Para JPG/PNG instale Pillow (pip install pillow) o guarde la imagen como BMP de 24 bits.")
        bmp = os.path.join(carpeta, nombre + ".bmp")
        Image.open(origen).convert("RGB").save(bmp, "BMP")
        print(f"convertida a BMP de 24 bits: {bmp}")

    exe = os.path.join(AQUI, "img_a_arquisim.exe" if os.name == "nt" else "img_a_arquisim")
    if not os.path.exists(exe):
        subprocess.run(["gcc", "-O2", "-o", exe, os.path.join(AQUI, "img_a_arquisim.c")], check=True)

    cmd = [exe, "-v"] + (["-e"] if estirar else []) + [os.path.basename(bmp)]
    r = subprocess.run(cmd, cwd=carpeta, stdout=subprocess.PIPE)       # la vista previa (stderr) sale a la terminal
    if r.returncode != 0:
        raise SystemExit("img_a_arquisim falló")
    salida = r.stdout.decode("utf-8", errors="replace").replace("\r\n", "\n").rstrip("\n").split("\n")

    # respaldo en memoria: si las pruebas fallan, se dejan los dos archivos como estaban
    respaldo = {}
    for nombre_archivo in ("proyecto.asm", "filtros.cpp"):
        with open(os.path.join(ENTREGA, nombre_archivo), encoding="utf-8") as f:
            respaldo[nombre_archivo] = f.read()

    marca = f"# >>> IMAGEN (generado con: ./img_a_arquisim {'-e ' if estirar else ''}{os.path.basename(bmp)})"
    reemplazar_bloque(os.path.join(ENTREGA, "proyecto.asm"), "# >>> IMAGEN", "# <<< IMAGEN",
                      [marca] + [l.rstrip() for l in salida] + ["# <<< IMAGEN"])

    datos = [int(v, 16) for l in salida if l.strip().startswith(".byte")
             for v in re.findall(r"0x[0-9A-Fa-f]{2}", l.split("#")[0])]
    if len(datos) != 256:
        raise SystemExit(f"el conversor produjo {len(datos)} bytes y se esperaban 256")
    filas = ["    {" + ", ".join(f"0x{v:02X}" for v in datos[16 * y:16 * y + 16]) + "}," for y in range(16)]
    reemplazar_bloque(os.path.join(ENTREGA, "filtros.cpp"), "// >>> IMAGEN", "// <<< IMAGEN",
                      [f"// >>> IMAGEN (la misma de proyecto.asm: {os.path.basename(bmp)})",
                       "const uint8_t IMAGEN[16][16] = {"] + filas + ["};", "// <<< IMAGEN"])

    print("\nImagen cambiada en proyecto.asm y filtros.cpp. Probando las 256 combinaciones...\n", flush=True)
    if subprocess.run([sys.executable, os.path.join(AQUI, "probar.py"), "--todas"]).returncode != 0:
        for nombre_archivo, texto in respaldo.items():
            with open(os.path.join(ENTREGA, nombre_archivo), "w", encoding="utf-8", newline="\n") as f:
                f.write(texto)
        print("\nRESULTADO: FALLO. Las pruebas no pasaron con la imagen nueva; proyecto.asm y filtros.cpp "
              "quedaron como estaban.", flush=True)
        return 1

    destino = os.path.join(SIM, "programas", "proyecto.asm")
    if os.path.isdir(os.path.dirname(destino)):
        shutil.copyfile(os.path.join(ENTREGA, "proyecto.asm"), destino)
        print(f"\ncopiado a {destino}", flush=True)

    # lo que depende de la imagen: los experimentos (pregunta 2) y las versiones de modificaciones/
    print("\nActualizando experimentos/ y modificaciones/...\n", flush=True)
    subprocess.run([sys.executable, os.path.join(AQUI, "experimentos.py")])
    mods = subprocess.run([sys.executable, os.path.join(AQUI, "probar_modificaciones.py"), "--guardar"]).returncode

    print(f"\nRESULTADO: LISTO. Imagen '{os.path.basename(bmp)}' puesta en proyecto.asm, filtros.cpp y "
          f"programas/proyecto.asm; original y .bmp en entrega/imagen/."
          + ("" if mods == 0 else " (OJO: alguna modificación de práctica falló.)"), flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
