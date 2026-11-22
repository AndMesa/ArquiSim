# Datos para el informe — Proyecto final "Filtros de imagen en ArquiSim"

Arquitectura del Computador, Pontificia Universidad Javeriana, semestre 2026-3.

Este archivo es **autocontenido**: reúne todo lo que se midió y se decidió durante el desarrollo, para
redactar el informe técnico (PDF) sin tener que abrir el código. Todos los números salen de ejecutar
`proyecto.asm` en **nuestro** simulador ArquiSim (el del Taller 2, sin modificarlo). Las capturas de
pantalla se identifican por su nombre de archivo (`cap_….png`); cómo tomar cada una está en la sección 0.

> **Para quien redacte el informe:** no invente problemas ni resultados. Si un dato no está aquí, no
> existe. Lo único que falta está en la sección 12 ("Pendiente").

---

## 0. Instrucciones para Astra

Astra: usted va a tomar las capturas en este PC controlando la terminal y después va a redactar el
informe. Esta sección es su guion; el resto del archivo son los datos. Todas las secuencias de teclas
de esta sección se comprobaron en la interfaz real de este simulador: producen el panel y los números
que se indican.

### 0.1 Orden de trabajo

1. **Preparar** la terminal (0.2) y hacer la revisión previa (0.3). Si el usuario le entregó la imagen
   propia del grupo, **póngala primero** (0.3, paso 2): todas las capturas deben tomarse después.
2. **Tomar las 12 capturas obligatorias** (0.5), en el orden de la tabla, con el procedimiento de 0.4.
3. Tomar las **capturas complementarias** (0.6). Respaldan las preguntas de reflexión, que el enunciado
   pide responder "con datos tomados de ArquiSim (contadores, capturas, valores de registros y memoria)".
4. **Redactar el informe** en PDF con las secciones de 0.7, en ese orden.
5. Pasar la lista de verificación final (0.9).

### 0.2 Preparación (una sola vez)

El simulador `sim` de esta carpeta es un ejecutable de **Linux**: se corre dentro de WSL (Ubuntu), que ya
está instalado en este PC.

1. Abra **Windows Terminal** y **maximice la ventana**. La interfaz necesita como mínimo **107 columnas
   × 41 filas**. Si al abrir el simulador aparece el aviso "La ventana mide … y el simulador necesita al
   menos …", reduzca la letra con `Ctrl` + `-` y presione Enter.
2. Escriba estos tres comandos (uno por línea, Enter después de cada uno):

   ```
   wsl
   cd /mnt/c/Users/IHSOSU/Downloads/ArquiSim
   export COLORTERM=truecolor
   ```

   El tercero es necesario para que la pantalla RGB muestre los colores exactos de 24 bits (sin él los
   aproxima a 256 colores). Hay que repetirlo si cierra la terminal.
3. Las capturas se guardan en `C:\Users\IHSOSU\Downloads\ArquiSim\entrega\capturas\` (la carpeta ya
   existe), en formato PNG, con el nombre exacto que se indica para cada una.

Todos los comandos de este documento se escriben desde `/mnt/c/Users/IHSOSU/Downloads/ArquiSim` (la
carpeta del simulador). Ahí ya hay una copia del programa en `programas/proyecto.asm`.

### 0.3 Revisión previa (antes de la primera captura)

1. Compruebe que todo sigue pasando:

   ```
   python3 entrega/herramientas/probar.py
   ```

   Deben salir 12 líneas con `PASA` y al final "12 de 12 combinaciones pasan". Si alguna dice `FALLA`,
   **deténgase y avise al usuario**.
2. **La imagen.** Compruebe cuál tiene el programa:

   ```
   grep -n ">>> IMAGEN" entrega/proyecto.asm
   ```

   Si la línea menciona `ejemplo.bmp`, el programa todavía tiene la imagen de ejemplo del enunciado.

   **Si el usuario le entregó una imagen**, póngala ahora. Esto se hace en **PowerShell de Windows**, no en
   WSL (abra otra pestaña de Windows Terminal con PowerShell; ahí están Python con Pillow y `gcc`):

   ```
   cd C:\Users\IHSOSU\Downloads\ArquiSim\entrega
   python herramientas\poner_imagen.py "RUTA\COMPLETA\DE\LA\IMAGEN"
   ```

   Acepta BMP, JPG y PNG. El comando convierte la imagen a 16 × 16, muestra una **vista previa en
   colores** en la terminal, la pone en `proyecto.asm`, en `filtros.cpp` y en `programas/proyecto.asm`,
   copia el archivo original y el `.bmp` a `entrega\imagen\`, corre las 256 pruebas y actualiza
   `experimentos\` y `modificaciones\`. Tarda cerca de un minuto. Qué revisar:

   - La **última línea** debe ser `RESULTADO: LISTO. Imagen '…' puesta en …`. Si dice
     `RESULTADO: FALLO`, el script dejó los archivos como estaban: **deténgase y avise al usuario**.
   - Mire la **vista previa** (16 filas de bloques de color, justo después de "convertida a BMP…"). Si no
     se reconoce qué es, **dígaselo al usuario antes de seguir** y sugiérale otra imagen: a 16 × 16 solo
     sobreviven las formas grandes, así que funcionan íconos, logotipos, banderas o dibujos de colores
     planos, y no las fotografías. Si la imagen no es cuadrada el script recorta el centro; con
     `python herramientas\poner_imagen.py -e "RUTA"` la estira en lugar de recortarla.
   - Anote las líneas que el comando imprime bajo `Pregunta 2 (endian.asm)` y `Pregunta 8 (pisa_s1.asm)`:
     son los datos de esas preguntas **para la imagen nueva** (vea 0.8).

   El comando se ejecuta **una sola vez**, antes de la primera captura. Después vuelva a la pestaña de WSL
   y repita el paso 1 de esta sección (deben seguir saliendo 12 `PASA`).

   **Si el usuario no le entregó ninguna imagen**, pregúntele si quiere seguir con la de ejemplo. Si dice
   que sí, siga y déjelo anotado en el informe.

### 0.4 Procedimiento de una captura en la interfaz

Cada captura se toma en una ejecución **nueva** del simulador (así el panel correcto aparece solo):

1. Escriba el **comando** de la captura y Enter. Se abre la interfaz con el panel izquierdo
   `PROGRAMA (.text: 105 instrucciones, 420 B)` y la línea de estado `LISTO: presione Enter…`.
2. Escriba, una por una, las líneas de la columna **"Escribir en ArquiSim"**, con Enter después de cada
   una. Se escriben tal cual, en la línea de comandos de abajo (`>`). Donde dice `Enter × 12` es
   presionar Enter doce veces sin escribir nada.
3. Espere a que la línea de estado (penúltimo recuadro de abajo) muestre el texto indicado. `c` puede
   tardar uno o dos segundos.
4. **Verifique antes de capturar:** el título del panel izquierdo y los contadores de la barra superior
   deben coincidir con los indicados. Si no coinciden, no tome la captura: salga con `q` y repita.
5. Tome la captura de **toda la ventana de la terminal** (deben verse la barra superior con los
   contadores, el panel izquierdo, el banco de registros y la línea de estado) y guárdela con el nombre indicado.
6. Escriba `q` y Enter para salir. El simulador imprime el estado final; no hace falta capturarlo
   (salvo donde se pida). Puede limpiar con `clear`.

No use `p` salvo donde se indique: con `mem[0x600] = …` seguido de `c`, el simulador cambia **solo** al
panel de la pantalla en la que el programa dibujó. `p` rota los paneles en este orden: programa →
pantalla de 16 colores → pantalla RGB → consola → programa.

Cómo reconocer cada panel (título del recuadro izquierdo):

| Panel | Título exacto |
|---|---|
| Programa | `PROGRAMA (.text: 105 instrucciones, 420 B)  [p] cambiar vista` |
| Pantalla de 16 colores | `PANTALLA 32x16 (0x400-0x5FF)  [p] cambiar vista` |
| Pantalla RGB | `PANTALLA RGB 32x32 (registros 0x604 y 0x608)  [p] cambiar vista` |

### 0.5 Las 12 capturas obligatorias

Para las 12: **comando** `./sim -n 100000 programas/proyecto.asm`; **escribir en ArquiSim** las dos
líneas de la tabla; **estado al capturar** `FIN DEL PROGRAMA: N instrucciones ejecutadas` con el N de la
tabla; **PC = 0x1A4** en la barra superior.

| # | Archivo | Escribir en ArquiSim | Panel al capturar | N (instrucciones) | Barra superior: `R … I … J … \| lect. … escr. … \| saltos …` | Qué debe verse |
|---|---|---|---|---|---|---|
| 1 | `cap_0x00_16col.png` | `mem[0x600] = 0x00` · `c` | Pantalla de 16 colores | 14149 | R 7168 I 6212 J 769 · lect. 513 escr. 768 · saltos 2560 | las dos mitades iguales |
| 2 | `cap_0x01_16col.png` | `mem[0x600] = 0x01` · `c` | Pantalla de 16 colores | 14405 | R 7168 I 6468 J 769 · lect. 513 escr. 768 · saltos 2304 | original a la izquierda, negativo a la derecha |
| 3 | `cap_0x02_16col.png` | `mem[0x600] = 0x02` · `c` | Pantalla de 16 colores | 19013 | R 10752 I 7236 J 1025 · lect. 513 escr. 768 · saltos 2816 | derecha en grises |
| 4 | `cap_0x03_16col.png` | `mem[0x600] = 0x03` · `c` | Pantalla de 16 colores | 19269 | R 10752 I 7492 J 1025 · lect. 513 escr. 768 · saltos 2560 | derecha: negativo y luego grises |
| 5 | `cap_0x04_16col.png` | `mem[0x600] = 0x04` · `c` | Pantalla de 16 colores | 18501 | R 9728 I 7748 J 1025 · lect. 769 escr. 768 · saltos 2816 | derecha: filtro 3 (paleta térmica) |
| 6 | `cap_0x07_16col.png` | `mem[0x600] = 0x07` · `c` | Pantalla de 16 colores | 23621 | R 13312 I 9028 J 1281 · lect. 769 escr. 768 · saltos 2816 | derecha: los tres filtros |
| 7 | `cap_0x80_rgb.png` | `mem[0x600] = 0x80` · `c` | Pantalla RGB | 34950 | R 14336 I 18565 J 2049 · lect. 2049 escr. 2049 · saltos 8193 | imagen original ampliada 2 × 2 |
| 8 | `cap_0x81_rgb.png` | `mem[0x600] = 0x81` · `c` | Pantalla RGB | 35974 | R 14336 I 19589 J 2049 · lect. 2049 escr. 2049 · saltos 7169 | negativo |
| 9 | `cap_0x82_rgb.png` | `mem[0x600] = 0x82` · `c` | Pantalla RGB | 54406 | R 28672 I 22661 J 3073 · lect. 2049 escr. 2049 · saltos 9217 | grises |
| 10 | `cap_0x83_rgb.png` | `mem[0x600] = 0x83` · `c` | Pantalla RGB | 55430 | R 28672 I 23685 J 3073 · lect. 2049 escr. 2049 · saltos 8193 | negativo y grises |
| 11 | `cap_0x84_rgb.png` | `mem[0x600] = 0x84` · `c` | Pantalla RGB | 52358 | R 24576 I 24709 J 3073 · lect. 3073 escr. 2049 · saltos 9217 | filtro 3 (paleta térmica) |
| 12 | `cap_0x87_rgb.png` | `mem[0x600] = 0x87` · `c` | Pantalla RGB | 72838 | R 38912 I 29829 J 4097 · lect. 3073 escr. 2049 · saltos 9217 | los tres filtros |

(En la columna "Escribir en ArquiSim" el punto `·` separa dos líneas: primero una, Enter, luego la otra, Enter.)

Ejemplo completo, captura 8: escribir `./sim -n 100000 programas/proyecto.asm` y Enter; escribir
`mem[0x600] = 0x81` y Enter (abajo aparece `MEM[0x600] = 0x00000081` y en el recuadro BOTONES quedan
marcados el 7 y el 0); escribir `c` y Enter; esperar `FIN DEL PROGRAMA: 35974 instrucciones ejecutadas`;
comprobar que el panel izquierdo dice `PANTALLA RGB 32x32 (registros 0x604 y 0x608)`; capturar la
ventana como `cap_0x81_rgb.png`; `q` y Enter.

Si los contadores de la barra no coinciden con la tabla, **no corrija los números del informe**: avise al usuario.

### 0.6 Capturas complementarias

Mismo procedimiento de 0.4, salvo donde se indica "sin interfaz".

| Archivo | Para | Comando | Escribir en ArquiSim | Panel al capturar | Qué debe verse al capturar |
|---|---|---|---|---|---|
| `cap_0x8C_rgb.png` | Filtro 3 (sepia) | `./sim -n 100000 programas/proyecto.asm` | `mem[0x600] = 0x8C` · `c` | Pantalla RGB | `FIN DEL PROGRAMA: 52358 instrucciones ejecutadas`; imagen en tonos café |
| `cap_0x94_rgb.png` | Filtro 3 (umbral) | igual | `mem[0x600] = 0x94` · `c` | Pantalla RGB | mismo estado (52358); imagen en blanco y negro puros |
| `cap_0x9C_rgb.png` | Filtro 3 (visión nocturna) | igual | `mem[0x600] = 0x9C` · `c` | Pantalla RGB | mismo estado (52358); imagen en tonos verdes |
| `cap_paletas_memoria.png` | Filtro 3 (la tabla) | igual | `m PALETAS` | Programa (no se ejecuta nada) | a la derecha `MEMORIA desde 0x300`, con `0x300 = 00 02 43 A1` y `0x304 = E0 F0 FC FF` |
| `cap_tabla4_a16colores.png` | Tabla 4 | igual | `bp A_16COLORES` · `c` · `$a0 = 0xB1` · Enter × 12 | Programa | estado `EJECUTANDO: la siguiente instrucción está en PC = 0x17C`; registros `$a0 = 000000B1` y `$v0 = 00000003` |
| `cap_tabla4_argb.png` | Tabla 4 | igual | `mem[0x600] = 0x80` · `bp A_RGB` · `c` · `$a0 = 0xB1` · Enter × 8 | Programa | estado `… PC = 0x1A0`; registros `$a0 = 000000B1` y `$v0 = 00A08040` |
| `cap_p1_memoria.png` | Pregunta 1 | igual | (nada) | Programa | título `PROGRAMA (.text: 105 instrucciones, 420 B)`; abajo a la izquierda, en MAPA DE MEMORIA, `Programa: 708 B de 1024 B (69.1 %)   .text 420 B   .data 288 B` |
| `cap_p2_endian.png` | Pregunta 2 | `./sim entrega/experimentos/endian.asm` | `c` | Programa (`.text: 4 instrucciones, 16 B`) | `FIN DEL PROGRAMA: 4 instrucciones ejecutadas`; `$t0 = FFFFEDED`, `$t1 = 000000FF`, `$t2 = 000000ED`; en MEMORIA `0x200 = FF FF ED ED` |
| `cap_p4_botones.png` | Pregunta 4 | `./sim entrega/experimentos/botones.asm` | `bt 0` · `bt 2` · `c` | Programa | `FIN DEL PROGRAMA: 5 instrucciones ejecutadas`, sin la palabra EXCEPCIÓN; `$t0 = 00000005` y `$t2 = 00000005`; barra superior `lect. 2  escr. 2`; recuadro BOTONES `= 0x05` |
| `cap_p5_tabla.png` | Pregunta 5 | `python3 entrega/herramientas/probar.py --tabla` (sin interfaz) | — | terminal normal | las 12 líneas `PASA` y la tabla de contadores (igual a la sección 7) |
| `cap_p5_salida_0x87.png` | Pregunta 5 | `./sim -n 100000 programas/proyecto.asm` | `mem[0x600] = 0x87` · `c` · `q` | terminal normal (ya fuera de la interfaz) | desplácese hacia arriba hasta `== Estadísticas ==`: `Instrucciones ejecutadas   72838   (R 38912 = 53 %,  I 29829 = 41 %,  J 4097 = 6 %)`, `Accesos a datos   3073 lecturas, 2049 escrituras`, `Saltos tomados   9217` |
| `cap_p7_limite.png` | Pregunta 7 | `./sim programas/proyecto.asm` (**sin** `-n`) | `c` | Pantalla de 16 colores | estado en rojo `LÍMITE de 10000 instrucciones (¿ciclo infinito?)`; barra `paso: 10000`; la imagen dibujada a medias: 11 filas completas (0 a 10) y el comienzo de la fila 11 |
| `cap_p8_pila.png` | Pregunta 8 | `./sim -n 100000 programas/proyecto.asm` | `mem[0x600] = 0x02` · `bp BRILLO` · `c` · `p` · `p` · `p` | Programa (los tres `p` vuelven de la pantalla de 16 colores al programa) | estado `… PC = 0x11C`; a la derecha `PILA ($sp = 0x3FC)` con la fila `0x3FC s 00 00 00 40   64 <-$sp`; registros `$sp = 000003FC` y `$ra = 000000E0` |
| `cap_p8_pisa_s1.png` | Pregunta 8 | `./sim -n 100000 entrega/experimentos/pisa_s1.asm` | `c` (tarda unos segundos) | Pantalla de 16 colores | estado en rojo `LÍMITE de 100000 instrucciones (¿ciclo infinito?)`; solo 4 píxeles pintados en la fila 0; registro `$s1 = 00000009` |
| `cap_p9_cpp_0x81.png` | Pregunta 9 | `g++ -std=c++11 -O2 -o /tmp/filtros entrega/filtros.cpp && /tmp/filtros 0x81` (sin interfaz) | — | terminal normal | 32 líneas de bloques de color: la misma imagen en negativo de `cap_0x81_rgb.png` |

Notas:

- `cap_0x84_rgb.png` y `cap_0x04_16col.png` (dos de las 12) también ilustran el filtro 3.
- En las dos capturas de la Tabla 4 el punto de parada detiene el programa al entrar a la función; la
  línea `$a0 = 0xB1` le pone como argumento el píxel "Sopa" de la Tabla 4 (así el resultado no depende de
  la imagen) y los Enter ejecutan la función hasta quedar sobre su `jr $ra`. Resultados: 3 (`0x03`) en 16
  colores y `0xA08040` en RGB, los valores de la Tabla 4.
- Con una imagen propia cambian tres capturas: `cap_p2_endian.png` (los valores de `$t0`, `$t1`, `$t2` y
  de la memoria serán los de esa imagen), `cap_p8_pisa_s1.png` (lo que se vea será lo que anunció
  `Pregunta 8 (pisa_s1.asm)`) y `cap_p9_cpp_0x81.png` (mostrará la imagen nueva en negativo). En todas
  las demás solo cambia el dibujo; los contadores y estados indicados siguen iguales.
- Si en alguna captura complementaria lo que se ve no coincide, omítala y dígalo en el informe; no la reemplace por otra cosa.

### 0.7 Estructura del informe

Archivo de salida: `C:\Users\IHSOSU\Downloads\ArquiSim\entrega\informe_proyecto_final.pdf`. Idioma:
español. Las secciones son las que exige el enunciado, en este orden:

| # | Sección del informe | Qué debe contener | De dónde sale (este archivo) | Capturas |
|---|---|---|---|---|
| — | Portada | título "Proyecto Final: Filtros de Imagen en ArquiSim", Arquitectura del Computador, Pontificia Universidad Javeriana, semestre 2026-3, integrantes | los nombres de los integrantes **pídalos al usuario** | — |
| 1 | Introducción | qué se pedía, qué se construyó (programa de 105 instrucciones con 4 funciones), cómo está organizado y cómo se verificó | secciones 1, 2 y 3 | `cap_p1_memoria.png` (opcional) |
| 2 | Descripción y justificación del filtro 3 | qué hace, las 4 paletas, el código de 7 instrucciones, por qué se eligió, alternativas consideradas, limitación | sección 4 | `cap_0x04_16col.png`, `cap_0x84_rgb.png`, `cap_0x8C_rgb.png`, `cap_0x94_rgb.png`, `cap_0x9C_rgb.png`, `cap_paletas_memoria.png` |
| 3 | Capturas de las combinaciones de prueba | las 12 capturas, agrupadas por pantalla, cada una con su pie (botones, filtros, instrucciones ejecutadas); la tabla de la Tabla 4 | secciones 5 y 6 | las 12 de 0.5, `cap_tabla4_a16colores.png`, `cap_tabla4_argb.png` |
| 4 | Preguntas de reflexión | las 9 preguntas del enunciado, numeradas igual, cada una con su respuesta y sus datos; en la 5 la tabla completa y la explicación de las diferencias; aquí va también "¿negativo → grises es igual a grises → negativo?" | secciones 7, 8 y 9 | `cap_p1_…`, `cap_p2_…`, `cap_p4_…`, `cap_p5_…`, `cap_p7_…`, `cap_p8_…`, `cap_p9_…` |
| 5 | Bitácora del desarrollo | etapas en orden y problemas encontrados con su solución | sección 10 | — |
| 6 | Conclusiones | 4 a 6 conclusiones propias apoyadas en los datos | sección 11 | — |

Enunciados de las 9 preguntas (para titularlas en el informe):

1. ¿Cuántos bytes ocupa su programa en `.text` y cuántos la imagen? ¿Qué porcentaje del 1 KiB de RAM queda
   libre? ¿Cuánta memoria necesitaría una imagen de 32 × 32 con una palabra `0x00RRGGBB` por píxel? ¿Cabría?
2. Ejecute `lw $t0, 0($s0)` con `$s0 = IMAGEN`. ¿Qué valor queda en `$t0`? ¿Qué píxeles contiene y en qué
   orden? Relaciónelo con que ArquiSim es *big-endian*.
3. La pantalla de 16 colores es memoria y la RGB se controla con registros de comando. ¿Cuántas
   instrucciones necesita su programa por píxel en cada una para calcular dónde se dibuja? ¿Qué ventaja da
   el autoincremento de `0x608`?
4. ¿Qué ocurre si el programa intenta escribir en `0x600`? ¿Por qué tiene sentido que los botones sean de solo lectura?
5. Complete una tabla con, para cada combinación de botones: instrucciones ejecutadas, porcentaje de tipo
   R, I y J, lecturas y escrituras de memoria y saltos tomados. Explique las diferencias.
6. Si un procesador ejecuta una instrucción por ciclo de reloj (CPI = 1), ¿cuánto tarda su programa en el
   peor caso a 1 MHz? ¿Y a 100 MHz? ¿Podría mostrar 30 imágenes por segundo a cada frecuencia?
7. ¿Por qué el programa necesita la opción `-n 100000`? ¿Qué protege ese límite?
8. ¿Qué registros usa el programa principal y cuáles las funciones? ¿Qué pasaría si una función modificara
   un registro `$s` que el programa principal está usando como índice?
9. Escriba el mismo algoritmo en C++ o en Java, suponiendo que la imagen es un arreglo de 16 × 16 bytes y
   la pantalla de 16 colores un arreglo de 16 × 32 bytes. Compare la cantidad de líneas con el
   ensamblador. ¿Qué hace por usted el compilador que en ensamblador hizo a mano (direcciones, registros, llamadas)?

### 0.8 Reglas

- Use **solo** los datos de este archivo y lo que se vea en las capturas. No invente problemas,
  resultados ni cifras. Si falta un dato, pregúntelo al usuario.
- **No edite a mano** `entrega/proyecto.asm`, `programas/proyecto.asm`, `src/` ni `isa.txt`. El único
  cambio permitido es el de la imagen, con `poner_imagen.py`, una vez y con la imagen que entregó el
  usuario (0.3). No ejecute comandos de `git`.
- El filtro 3 se llama "mapa de color" y la función de brillo `BRILLO`; use los nombres de funciones y
  etiquetas tal como aparecen aquí.
- Los porcentajes de la tabla de la pregunta 5 tienen un decimal; el simulador, al salir, los muestra
  redondeados a enteros. Ambos son correctos; indíquelo si usa los dos.
- **Qué cambia con una imagen propia.** Los contadores, tamaños y tiempos (secciones 3, 7 y preguntas 1,
  3, 5, 6 y 7) **no cambian**: no dependen de la imagen. Sí cambian, y hay que escribirlos con los datos nuevos:
  - Pregunta 2: el valor de `lw` (`0xFFFFEDED` es de la imagen de ejemplo). Use lo que imprimió
    `Pregunta 2 (endian.asm)` y lo que se ve en `cap_p2_endian.png`; la explicación de *big-endian* es la misma.
  - Pregunta 8, experimento `pisa_s1.asm`: cuántos píxeles pinta, en qué x queda atrapado y si llega al
    límite dependen de los píxeles. Use lo que imprimió `Pregunta 8 (pisa_s1.asm)`. Si el programa dañado
    sí termina, descríbalo tal cual (termina, pero dibuja mal) y no afirme que se queda en un ciclo infinito.
  - Las descripciones de lo que se ve ("fondo negro", "rojo → cian", "blanco amarillento") y los nombres de
    los píxeles de la Tabla 4 (Fondo, Rojo, Letra, Sopa) son de la imagen de ejemplo. Los valores de la
    Tabla 4 siguen siendo válidos como prueba de las funciones; describa las capturas según lo que se vea.
  - El nombre de la imagen en la introducción.
- No comprima ni envíe la entrega: eso lo hace el usuario.

### 0.9 Lista de verificación final

- [ ] 12 archivos `cap_0x…_16col.png` / `cap_0x…_rgb.png` en `entrega/capturas/`, cada uno con el panel y los contadores de 0.5.
- [ ] Capturas complementarias tomadas (o anotadas como omitidas).
- [ ] El informe tiene las 6 secciones de 0.7, en orden, y las 9 preguntas numeradas como en el enunciado.
- [ ] Las cifras del informe coinciden con este archivo (105 instrucciones, 420 bytes, 14149 a 72838 instrucciones ejecutadas…).
- [ ] Portada con los nombres de los integrantes.
- [ ] El informe dice qué imagen se usó (la de ejemplo o la propia).
- [ ] PDF guardado como `entrega/informe_proyecto_final.pdf`.

---

## 1. Qué pide el enunciado (resumen)

- Programa en el ensamblador MIPS reducido de ArquiSim que lee una imagen de 16 × 16 píxeles RGB332
  (1 byte por píxel, `RRRGGGBB`) guardada en `.data`, lee los 8 botones **una vez** (palabra `0x600`),
  aplica los filtros elegidos y dibuja el resultado.
- Botones: bit 0 negativo, bit 1 escala de grises, bit 2 filtro propio, bits 3-6 libres, bit 7 pantalla
  (0 = pantalla de 16 colores, 1 = pantalla RGB). Los filtros se aplican en orden 1 → 2 → 3.
- Pantalla de 16 colores (memoria `0x400-0x5FF`, 32 × 16): original a la izquierda, filtrada a la derecha.
- Pantalla RGB (registros `0x604` posición y `0x608` color, 32 × 32): solo la filtrada, ampliada 2 × 2.
- Restricciones: máximo 128 instrucciones, la imagen no se modifica, mínimo 3 funciones con `jal`/`jr`
  (aplicar filtros, pasar a 16 colores, pasar a `0x00RRGGBB`), argumento en `$a0` y resultado en `$v0`.

Objetivos que declara el enunciado (sirven para la introducción del informe):

- Manipular datos a nivel de bit con máscaras y desplazamientos (`andi`, `or`, `xori`, `sll`, `srl`).
- Recorrer arreglos de dos dimensiones guardados en memoria, calculando direcciones de la forma base + desplazamiento.
- Usar entrada y salida mapeada en memoria: botones, una pantalla que es memoria y otra que se controla con registros de comando.
- Organizar el programa en funciones con `jal` y `jr`.
- Medir la ejecución con los contadores de ArquiSim y relacionar lo que se ve en el simulador con el funcionamiento del procesador.

Contexto: en el Taller 2 el grupo construyó el procesador de ArquiSim (la codificación de cada instrucción
en `isa.txt` y su comportamiento en `src/ejecutar.c`); en este proyecto el procesador ya está listo y el
grupo escribe el software que corre sobre él.

## 2. Entorno y forma de medir

| Dato | Valor |
|---|---|
| Simulador | ArquiSim del grupo (Taller 2), carpeta `ArquiSim/`, `src/` e `isa.txt` **sin modificar** |
| Máquina simulada | MIPS reducido de 32 bits, RAM de 1024 bytes big-endian, sin *delay slots* |
| Instrucciones usadas | `lw sw lbu sb add addi andi or xori sll srl slti beq bne j jal jr` y las pseudoinstrucciones `li` y `move` (cada una se expande a 1 instrucción: `addi` y `add`) |
| Instrucciones que NO se usaron | `mult`, `div`, acumulador, banderas, `syscall` (Etapa 5) |
| Comando | `./sim -n 100000 programas/proyecto.asm`, luego `mem[0x600] = 0xNN` y `c` (el panel de la pantalla usada aparece solo) |
| Compiladores para las herramientas | gcc/g++ 15.2.0 (MinGW-W64), Python 3.12 |

**Cómo se probó en automático.** El modo sin interfaz de ArquiSim (`-q`, `-t`, `--traza`) no permite
poner los botones. Para no tocar el simulador se escribió `herramientas/harness.c`: un programa aparte
que se enlaza con los mismos `src/ciclo.c`, `cpu.c`, `isa.c`, `ensamblador.c`, `sistema.c` y
`ejecutar.c` (sin cambiarlos), llama a `cambiar_botones()` igual que la interfaz cuando uno escribe
`mem[0x600] = valor`, ejecuta con `ejecutar_paso()` y saca en JSON la memoria de video, los colores
escritos en `0x608` y los contadores. `herramientas/probar.py` compara eso con `referencia.py`.

Los contadores del harness son los mismos del simulador: con los botones en `0x00`, `./sim -q -n 100000`
(el binario original del grupo, ejecutado en Linux/WSL) da exactamente los mismos números que el harness:
14149 instrucciones, R 7168, I 6212, J 769, 513 lecturas, 768 escrituras, 2560 saltos.

**Qué se verificó (todo pasa):**

| Prueba | Resultado |
|---|---|
| `referencia.py` contra la Tabla 4 del enunciado | 24 de 24 valores |
| `proyecto.asm` contra `referencia.py`, las 12 combinaciones del enunciado | 12 de 12 |
| `proyecto.asm` contra `referencia.py`, las 256 combinaciones posibles de botones | 256 de 256 |
| Cada llamada a cada función (`$a0` → `$v0`) contra la misma función del oráculo | todas |
| `filtros.cpp` contra `referencia.py`, las 256 combinaciones | 256 de 256 |
| La imagen (`0x200-0x2FF`) queda igual al terminar | sí, en las 256 |
| Los botones se leen una sola vez; `0x604` se escribe una sola vez (solo en modo RGB) | sí |
| `$sp` vuelve a `0x400`; la pila nunca baja de `0x3FC` | sí |
| Con todos los botones apagados las dos mitades son iguales | sí |

- Captura: `cap_p5_tabla.png` (muestra las 12 líneas `PASA`; pasos en 0.6).

## 3. El programa (`proyecto.asm`)

### 3.1 Tamaño

| Bloque | Dirección | Instrucciones |
|---|---|---|
| Inicio: leer botones y elegir pantalla | `0x000-0x008` | 3 |
| Ciclo de la pantalla de 16 colores | `0x00C-0x064` | 23 |
| Ciclo de la pantalla RGB | `0x068-0x0B4` | 20 |
| Función `APLICAR_FILTROS` | `0x0B8-0x118` | 25 |
| Función `BRILLO` | `0x11C-0x148` | 12 |
| Función `A_16COLORES` | `0x14C-0x17C` | 13 |
| Función `A_RGB` | `0x180-0x1A0` | 9 |
| **Total** | `0x000-0x1A3` | **105 de 128 (420 bytes de 512)** |

No hay pseudoinstrucciones que se expandan a más de una instrucción: `li` con valores pequeños es un
`addi` y `move` es un `add`. El simulador confirma "`.text 420 B, 105 instrucciones`". Quedan 23
instrucciones libres; no hubo que optimizar para que cupiera, así que el código prefiere ser claro.

### 3.2 Mapa de memoria usado

| Dirección | Contenido | Bytes |
|---|---|---|
| `0x000-0x1A3` | programa | 420 |
| `0x1A4-0x1FF` | libre en `.text` | 92 |
| `0x200-0x2FF` | `IMAGEN` (16 × 16, RGB332), no se modifica | 256 |
| `0x300-0x31F` | `PALETAS` del filtro 3 (4 paletas × 8 colores) | 32 |
| `0x320-0x3FB` | libre | 220 |
| `0x3FC-0x3FF` | pila: una palabra (el `$ra` de `APLICAR_FILTROS`) | 4 |
| `0x400-0x5FF` | pantalla de 16 colores (dispositivo) | 512 |
| `0x600`, `0x604`, `0x608` | botones, RGB posición, RGB color (dispositivos) | 12 |

### 3.3 Estructura (4 funciones; el enunciado pide 3 como mínimo)

| Función | Entrada | Salida | Nota |
|---|---|---|---|
| `APLICAR_FILTROS` | `$a0` píxel, `$a1` botones | `$v0` píxel filtrado | llama a `BRILLO`, por eso guarda `$ra` en la pila |
| `BRILLO` | `$a0` píxel | `$v0` = Y (0-7) | hoja; la usan el filtro 2 y el filtro 3 |
| `A_16COLORES` | `$a0` píxel | `$v0` color 0-15 | hoja |
| `A_RGB` | `$a0` píxel | `$v0` = `0x00RRGGBB` | hoja |

Flujo: leer botones → si bit 7 = 0, dos ciclos anidados (y, x de 0 a 15) que por cada píxel dibujan la
original a la izquierda y la filtrada a la derecha; si bit 7 = 1, se escribe la posición (0,0) una vez
y dos ciclos anidados (y, x de 0 a 31 de **pantalla**) leen el píxel (x/2, y/2), lo filtran, lo pasan a
24 bits y lo escriben en `0x608`. La imagen nunca se escribe: el filtro se aplica en el momento de dibujar.

### 3.4 Cómo se hizo cada cosa (con una frase para el informe)

- **Negativo en una instrucción:** `xori $v0, $v0, 0xFF`. Invertir los 3 bits de R da 7 − R, los 3 de G
  da 7 − G y los 2 de B da 3 − B (el complemento a uno de un número de n bits es (2ⁿ − 1) − valor).
- **Grises sin `mult` ni `div`:** Y = (R + 2G + 2B + 2) >> 2 con `sll 1` para multiplicar por 2 y
  `srl 2` para dividir por 4; luego R' = Y (`sll 5`), G' = Y (`sll 2`), B' = Y >> 1 (`srl 1`).
- **16 colores:** cada bit se lleva a su posición con un corrimiento, se aísla con `andi` y se junta
  con `or` (bit 7 → bit 0, bit 4 → bit 1, bit 1 → bit 2, (bit 6 OR bit 3) → bit 3).
- **RGB:** `(p & 0xE0) << 16`, `(p & 0x1C) << 11`, `(p & 0x03) << 6`: cada canal queda en los bits altos
  de su byte, que es lo mismo que (R << 21) | (G << 13) | (B << 6).
- **Ampliación 2 × 2:** los contadores recorren la pantalla (0-31) y el píxel que se lee es
  (x >> 1, y >> 1); cada píxel de la imagen se usa en 2 columnas y 2 filas.
- **Autoincremento:** `sw $zero, 0x604($zero)` una sola vez; después solo se escribe en `0x608`.

## 4. Filtro 3: mapa de color con paletas (botón 2; botones 3 y 4 eligen la paleta)

### 4.1 Qué hace

Es un filtro de **color**: calcula el brillo Y del píxel (el mismo de la escala de grises, de 0 a 7) y
lo usa como **índice de una tabla** de 8 colores guardada en `.data`. El píxel de salida es
`PALETAS[8 × paleta + Y]`. Hay 4 tablas y los botones 3 y 4 eligen cuál:

| Botones 4,3 | `mem[0x600]` (16 col. / RGB) | Paleta | Y=0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|---|---|
| 0 0 | `0x04` / `0x84` | térmica (negro → azul → morado → rojo → naranja → amarillo → blanco) | `00` | `02` | `43` | `A1` | `E0` | `F0` | `FC` | `FF` |
| 0 1 | `0x0C` / `0x8C` | sepia (tonos café) | `00` | `24` | `44` | `68` | `8D` | `B1` | `D1` | `F6` |
| 1 0 | `0x14` / `0x94` | umbral blanco y negro (Y ≥ 4 es blanco) | `00` | `00` | `00` | `00` | `FF` | `FF` | `FF` | `FF` |
| 1 1 | `0x1C` / `0x9C` | visión nocturna (solo canal verde, G = Y) | `00` | `04` | `08` | `0C` | `10` | `14` | `18` | `1C` |

(Valores en hexadecimal, formato RGB332.) En ensamblador son 7 instrucciones dentro de `APLICAR_FILTROS`:

```asm
F_MAPA: andi $t0, $a1, 0x04         # ¿botón 2?
        beq  $t0, $zero, F_FIN
        move $a0, $v0
        jal  BRILLO                 # $v0 = Y (0-7)
        andi $t0, $a1, 0x18         # botones 3 y 4 = 8 * número de paleta
        add  $t0, $t0, $v0          # índice = 8*paleta + Y
        lbu  $v0, PALETAS($t0)      # píxel = PALETAS[índice]
```

El detalle ingenioso: los botones 3 y 4 valen 8 y 16, y cada paleta mide 8 bytes, así que
`botones AND 0x18` **ya es** el desplazamiento de la paleta dentro de la tabla; no hay que correr ni multiplicar.

### 4.2 Por qué se eligió (justificación)

- **Aprovecha lo visto en el curso:** máscaras (`andi`), direccionamiento base + desplazamiento sobre
  un arreglo de `.data` (`lbu $v0, PALETAS($t0)`), entrada mapeada en memoria (botones 3 y 4 como
  opciones) y una función que llama a otra guardando `$ra` en la pila (`APLICAR_FILTROS` → `BRILLO`).
- **Es barato:** 7 instrucciones y 32 bytes de datos, y reutiliza `BRILLO`, que ya existía para los grises.
- **Es un filtro que son cuatro:** sepia y umbral blanco/negro (dos de las ideas del enunciado) salen
  con el mismo código; solo cambia la tabla. Agregar otro efecto es cambiar 8 bytes, no programar.
- **Se combina bien con los otros dos:** como trabaja sobre el brillo, "negativo + mapa" invierte la
  paleta (lo claro pasa a ser lo frío) y "grises + mapa" da casi lo mismo que "mapa" solo.
- **Es una técnica real:** las cámaras térmicas y las imágenes médicas usan exactamente esto
  (*look-up table* o falso color).

### 4.3 Opciones que se consideraron

| Opción | Tipo | Costo medido | Comentario |
|---|---|---|---|
| **Mapa de color con 4 paletas (elegida)** | color | 7 instrucciones + 32 bytes | incluye sepia y umbral; usa botones 3-4 |
| Espejo horizontal | geométrico | +4 instrucciones (programa de 109) | fácil pero poco vistoso; quedó como modificación de práctica |
| Intercambiar rojo y verde | color | 9 instrucciones (programa de 107) | solo máscaras y corrimientos; quedó como modificación de práctica |
| Dejar un solo canal | color | 3 instrucciones (programa de 101) | demasiado simple |

(Los costos de las alternativas se midieron ensamblándolas: `herramientas/probar_modificaciones.py`.)

### 4.4 Limitación observada

En la pantalla de 16 colores la paleta térmica pierde detalle: Y = 5 (`0xF0`) y Y = 6 (`0xFC`) se
convierten ambos en el color 11 (amarillo brillante), porque esa pantalla solo usa el bit alto de cada
canal. En la pantalla RGB sí se distinguen (naranja y amarillo).

- Capturas: `cap_0x84_rgb.png` (térmica, es una de las 12 obligatorias).
- Capturas: `cap_0x8C_rgb.png` (sepia), `cap_0x94_rgb.png` (umbral), `cap_0x9C_rgb.png` (visión nocturna); pasos en 0.6.
- Captura: `cap_0x04_16col.png` (original a la izquierda, térmica a la derecha; es una de las 12).
- Captura: `cap_paletas_memoria.png` (las 4 paletas en la ventana MEMORIA desde `0x300`; pasos en 0.6).

## 5. Casos de prueba de la Tabla 4 (valores obtenidos en el simulador)

Cada valor se obtuvo llamando a la función en ArquiSim con ese `$a0` y leyendo `$v0`
(`herramientas/trazas.py`). Coinciden todos con el enunciado.

| Píxel (x, y) | RGB332 | `A_16COLORES` | `A_RGB` | Negativo | 16 col. | Grises | 16 col. | Y | Filtro 3 (térmica) |
|---|---|---|---|---|---|---|---|---|---|
| Fondo (0, 0) | `0xFF` | 15 | `0xE0E0C0` | `0x00` | 0 | `0xFF` | 15 | 7 | `0xFF` |
| Rojo (5, 1) | `0xE0` | 9 | `0xE00000` | `0x1F` | 14 | `0x49` | 8 | 2 | `0x43` |
| Letra (3, 11) | `0x00` | 0 | `0x000000` | `0xFF` | 15 | `0x00` | 0 | 0 | `0x00` |
| Sopa (9, 10) | `0xB1` | 3 | `0xA08040` | `0x4E` | 12 | `0x92` | 7 | 4 | `0xE0` |

Con los tres filtros a la vez (`0x07`), los mismos píxeles dan `0x00`, `0xF0`, `0xFF` y `0xA1`.

- Captura: `cap_tabla4_a16colores.png` (`$a0 = 0xB1` → `$v0 = 0x03`; pasos en 0.6).
- Captura: `cap_tabla4_argb.png` (`$a0 = 0xB1` → `$v0 = 0x00A08040`; pasos en 0.6).

## 6. Capturas de las combinaciones de prueba (sección obligatoria del informe)

Son las 12 capturas de la sección 0.5 (ahí están el comando, las teclas, el panel y los contadores de
cada una). En el informe van agrupadas por pantalla, cada una con un pie como el de la última columna.

| Archivo | `mem[0x600]` | Pantalla | Qué muestra | Pie sugerido |
|---|---|---|---|---|
| `cap_0x00_16col.png` | `0x00` | 16 colores | las dos mitades iguales | Sin filtros: original (izquierda) y filtrada (derecha) idénticas. 14149 instrucciones. |
| `cap_0x01_16col.png` | `0x01` | 16 colores | derecha en negativo (fondo negro, rojo → cian) | Filtro 1, negativo. 14405 instrucciones. |
| `cap_0x02_16col.png` | `0x02` | 16 colores | derecha en grises | Filtro 2, escala de grises. 19013 instrucciones. |
| `cap_0x03_16col.png` | `0x03` | 16 colores | derecha: negativo y luego grises | Filtros 1 y 2. 19269 instrucciones. |
| `cap_0x04_16col.png` | `0x04` | 16 colores | derecha: mapa de color (térmica) | Filtro 3, mapa de color con paleta térmica. 18501 instrucciones. |
| `cap_0x07_16col.png` | `0x07` | 16 colores | derecha: los tres filtros | Filtros 1, 2 y 3 en orden. 23621 instrucciones. |
| `cap_0x80_rgb.png` | `0x80` | RGB | imagen original ampliada 2 × 2 | Sin filtros, pantalla RGB. 34950 instrucciones. |
| `cap_0x81_rgb.png` | `0x81` | RGB | negativo | Filtro 1, negativo. 35974 instrucciones. |
| `cap_0x82_rgb.png` | `0x82` | RGB | grises | Filtro 2, escala de grises. 54406 instrucciones. |
| `cap_0x83_rgb.png` | `0x83` | RGB | negativo y grises | Filtros 1 y 2. 55430 instrucciones. |
| `cap_0x84_rgb.png` | `0x84` | RGB | mapa de color (térmica) | Filtro 3, mapa de color con paleta térmica. 52358 instrucciones. |
| `cap_0x87_rgb.png` | `0x87` | RGB | los tres filtros | Filtros 1, 2 y 3 en orden (peor caso). 72838 instrucciones. |

Observaciones que se pueden comentar junto a las capturas:

- En `0x00` las dos mitades son iguales: `APLICAR_FILTROS` devuelve el píxel sin cambios cuando no hay botones.
- `0x03` y `0x83` muestran la imagen en grises con los tonos invertidos; da lo mismo aplicar primero el negativo o primero los grises (sección 9).
- En la pantalla RGB el fondo blanco de la imagen se ve ligeramente amarillo: `0xFF` se convierte en
  `0xE0E0C0` (el azul solo tiene 2 bits).
- Los colores de la pantalla de 16 colores dependen de la paleta de la terminal; los de la RGB son exactos
  si la terminal muestra color de 24 bits (por eso el `export COLORTERM=truecolor` de la sección 0.2).

## 7. Pregunta 5: tabla de ejecución

Medido con las 12 combinaciones. La barra superior de ArquiSim muestra los mismos conteos; al salir
(`q`) muestra los porcentajes redondeados a enteros (por ejemplo 51 %, 44 %, 5 % para `0x00`).

| Botones | Filtros | Pantalla | Instrucciones | R | % R | I | % I | J | % J | Lecturas | Escrituras | Saltos tomados |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `0x00` | ninguno | 16 col. | 14149 | 7168 | 50.7 % | 6212 | 43.9 % | 769 | 5.4 % | 513 | 768 | 2560 |
| `0x01` | negativo | 16 col. | 14405 | 7168 | 49.8 % | 6468 | 44.9 % | 769 | 5.3 % | 513 | 768 | 2304 |
| `0x02` | grises | 16 col. | 19013 | 10752 | 56.6 % | 7236 | 38.1 % | 1025 | 5.4 % | 513 | 768 | 2816 |
| `0x03` | neg. + grises | 16 col. | 19269 | 10752 | 55.8 % | 7492 | 38.9 % | 1025 | 5.3 % | 513 | 768 | 2560 |
| `0x04` | mapa de color | 16 col. | 18501 | 9728 | 52.6 % | 7748 | 41.9 % | 1025 | 5.5 % | 769 | 768 | 2816 |
| `0x07` | los tres | 16 col. | 23621 | 13312 | 56.4 % | 9028 | 38.2 % | 1281 | 5.4 % | 769 | 768 | 2816 |
| `0x80` | ninguno | RGB | 34950 | 14336 | 41.0 % | 18565 | 53.1 % | 2049 | 5.9 % | 2049 | 2049 | 8193 |
| `0x81` | negativo | RGB | 35974 | 14336 | 39.9 % | 19589 | 54.5 % | 2049 | 5.7 % | 2049 | 2049 | 7169 |
| `0x82` | grises | RGB | 54406 | 28672 | 52.7 % | 22661 | 41.7 % | 3073 | 5.6 % | 2049 | 2049 | 9217 |
| `0x83` | neg. + grises | RGB | 55430 | 28672 | 51.7 % | 23685 | 42.7 % | 3073 | 5.5 % | 2049 | 2049 | 8193 |
| `0x84` | mapa de color | RGB | 52358 | 24576 | 46.9 % | 24709 | 47.2 % | 3073 | 5.9 % | 3073 | 2049 | 9217 |
| `0x87` | los tres | RGB | 72838 | 38912 | 53.4 % | 29829 | 41.0 % | 4097 | 5.6 % | 3073 | 2049 | 9217 |

**Los contadores no dependen de la imagen.** El programa no tiene ningún salto que dependa del valor de
un píxel (solo de los botones), así que cualquier imagen da los mismos números. Se comprobó con una
imagen aleatoria: mismos 12 resultados. Tampoco dependen de los botones 3 y 4 (cambian la paleta, no el camino).

### 7.1 De dónde sale cada número

Fórmula general: `instrucciones = fijo + filas × 4 + píxeles × (base + filtros)`.

| | 16 colores | RGB |
|---|---|---|
| Vueltas del ciclo interno ("píxeles") | 256 (16 × 16) | 1024 (32 × 32) |
| Instrucciones base por vuelta, sin filtros | 55 | 34 |
| Costo por fila (`li` de x + `addi`/`slti`/`bne` de y) | 4 × 16 = 64 | 4 × 32 = 128 |
| Fijo (inicio + `j FIN`) | 5 | 6 |
| Total sin filtros | 5 + 64 + 256 × 55 = **14149** | 6 + 128 + 1024 × 34 = **34950** |

Las 55 instrucciones por píxel en 16 colores: 5 para direcciones (`sll`, `add`, `lbu`, `sll`, `add`),
2 + 13 + 1 para la mitad izquierda (`move`, `jal`, `A_16COLORES`, `sb`), 3 + 12 para llamar a
`APLICAR_FILTROS` sin filtros, 2 + 13 + 1 para la mitad derecha y 3 del ciclo (`addi`, `slti`, `bne`).
Las 34 de RGB: 7 para leer y llamar (`srl`, `sll`, `srl`, `add`, `lbu`, `move`, `jal`), 12 de
`APLICAR_FILTROS`, 2 + 9 de `A_RGB`, 1 `sw` y 3 del ciclo.

Costo adicional de cada filtro **por vuelta**:

| Filtro | Instrucciones extra | Tipo R | Tipo I | Tipo J | Lecturas extra | Saltos tomados |
|---|---|---|---|---|---|---|
| Negativo | +1 (`xori`) | 0 | +1 | 0 | 0 | −1 (el `beq` que lo saltaba ya no salta) |
| Grises | +19 (`move`, `jal`, 12 de `BRILLO`, 5 para armar el gris) | +14 | +4 | +1 | 0 | +1 (−1 `beq`, +1 `jal`, +1 `jr`) |
| Mapa de color | +17 (`move`, `jal`, 12 de `BRILLO`, `andi`, `add`, `lbu`) | +10 | +6 | +1 | +1 (`lbu PALETAS`) | +1 |

Comprobación: `0x07` = 14149 + 256 × (1 + 19 + 17) = 23621; `0x87` = 34950 + 1024 × 37 = 72838.

### 7.2 Explicación de las diferencias (para redactar)

- **RGB ejecuta ~2.5 veces más que 16 colores** sin filtros (34950 contra 14149) y hasta 3 veces más
  con todos (72838 contra 23621), porque dibuja 1024 píxeles en vez de 512 y, sobre todo, porque con la
  ampliación 2 × 2 **cada píxel de la imagen se filtra 4 veces** (una por cada píxel de pantalla que lo
  muestra). Por eso los filtros cuestan 4 veces más en RGB: los grises agregan 4864 instrucciones en 16
  colores y 19456 en RGB.
- **El negativo casi no cuesta** (+256 o +1024 instrucciones, 1.8 % y 2.9 %): es una sola `xori`. Incluso
  **baja** los saltos tomados, porque el `beq` que saltaba por encima de la `xori` deja de saltar.
- **Grises y mapa de color son los caros** porque llaman a `BRILLO` (12 instrucciones más el `jal` y el
  `jr`). Juntos calculan el brillo dos veces por píxel.
- **Tipo R sube con los filtros** (41 % → 53 % en RGB) porque `BRILLO` y el armado del gris son casi todo
  corrimientos, sumas y `or` (tipo R). Sin filtros, en RGB domina el tipo I (53 %): `andi`, `slti`,
  `addi`, `lbu`, `sw`, `beq`.
- **Tipo J se mantiene cerca de 5-6 %:** son los `jal` (2 o 3 por vuelta, más 1 por cada filtro que usa
  `BRILLO`) y el `j FIN`. Los `jr` cuentan como tipo R.
- **Lecturas:** 1 (botones) + 1 `lbu` de la imagen por vuelta + 1 `lw $ra` (pop) por vuelta = 513 y 2049.
  El mapa de color agrega 1 `lbu` de la paleta por vuelta (769 y 3073).
- **Escrituras:** en 16 colores 2 `sb` de pantalla + 1 `sw $ra` (push) por píxel = 768; en RGB 1 (posición)
  + 1024 colores + 1024 push = 2049. No cambian con los filtros.
- **La mitad de los accesos a memoria son de la pila:** `APLICAR_FILTROS` guarda y recupera `$ra` siempre,
  aunque ningún filtro esté activo. Se dejó así por claridad (una sola regla: "si llamo a otra función,
  guardo `$ra`"); guardar `$ra` solo cuando hace falta ahorraría 4 instrucciones por vuelta.

- Capturas: `cap_p5_tabla.png` (la tabla completa) y `cap_p5_salida_0x87.png` (estadísticas que imprime el simulador al salir, para `0x87`); pasos en 0.6. Los contadores de cada combinación también se ven en la barra superior de las 12 capturas obligatorias.

## 8. Preguntas de reflexión (con los datos de nuestro simulador)

### Pregunta 1 — Memoria

- `.text`: **105 instrucciones × 4 = 420 bytes** (`0x000-0x1A3`). Imagen: **256 bytes** (`0x200-0x2FF`).
- Además: 32 bytes de paletas (`0x300-0x31F`) y 4 bytes de pila en uso (`0x3FC`).

| Se cuenta | Bytes usados | % usado | Bytes libres | % libre de 1 KiB |
|---|---|---|---|---|
| Solo `.text` + imagen (lo que pregunta el enunciado) | 676 | 66.0 % | 348 | **34.0 %** |
| `.text` + imagen + paletas (lo que reporta ArquiSim: "708 B de 1024 B, 69.1 %") | 708 | 69.1 % | 316 | **30.9 %** |
| Lo anterior + la palabra de pila | 712 | 69.5 % | 312 | 30.5 % |

- Imagen de 32 × 32 con una palabra `0x00RRGGBB` por píxel: 1024 píxeles × 4 bytes = **4096 bytes = 4 KiB**.
  **No cabe:** es 4 veces toda la RAM (1024 bytes) y 8 veces la zona de datos (`0x200-0x3FF`, 512 bytes).
  Ni siquiera en RGB332 (1024 bytes) cabría, porque ocuparía toda la RAM sin dejar espacio al programa.
  En 512 bytes de datos caben como máximo 128 palabras, es decir una imagen de 11 × 11.
- Por eso la pantalla RGB guarda su imagen **dentro del dispositivo**: el procesador solo usa 2 registros
  (8 bytes de direcciones) para manejar 1024 píxeles de 24 bits.

- Captura: `cap_p1_memoria.png` (título `PROGRAMA (.text: 105 instrucciones, 420 B)` y, en MAPA DE MEMORIA, `Programa: 708 B de 1024 B (69.1 %)`; pasos en 0.6).

### Pregunta 2 — `lw $t0, 0($s0)` con `$s0 = IMAGEN` (experimento `experimentos/endian.asm`)

> Este dato **depende de la imagen**. Los valores de abajo son de la imagen de ejemplo; con la imagen
> propia use los que imprime `poner_imagen.py` bajo "Pregunta 2 (endian.asm)" (o vuelva a correr
> `python herramientas/experimentos.py`).

- Memoria: `0x200 = FF`, `0x201 = FF`, `0x202 = ED`, `0x203 = ED`.
- Resultado: **`$t0 = 0xFFFFEDED`**.
- Contiene los **4 primeros píxeles de la fila 0**: (0,0) = `0xFF`, (1,0) = `0xFF`, (2,0) = `0xED`,
  (3,0) = `0xED`, en ese orden de izquierda a derecha dentro de la palabra.
- Relación con *big-endian*: el byte de la dirección **menor** (`0x200`, píxel (0,0)) queda en el byte
  **más significativo** del registro (bits 31-24), y el de la dirección mayor (`0x203`, píxel (3,0)) en el
  menos significativo (bits 7-0). Comprobado con `lbu $t1, 0($s0)` → `0xFF` y `lbu $t2, 3($s0)` → `0xED`.
  En una máquina *little-endian* el mismo `lw` daría `0xEDEDFFFF`.
- Consecuencia práctica: por eso el programa lee los píxeles con `lbu` (un byte) y no con `lw`. Y se usa
  `lbu` y no `lb`: con `lb`, un píxel con el bit 7 en 1 se extiende con signo (`0xB1` queda como
  `0xFFFFFFB1` y `0xFF` como `0xFFFFFFFF` = −1), y un corrimiento como `srl 5` da `0x07FFFFFD` en lugar de 5.
  **Se probó cambiando `lbu` por `lb`:** las pantallas salen iguales en las 256 combinaciones, porque todas
  nuestras funciones aíslan cada campo con `andi` antes de usarlo; pero `APLICAR_FILTROS` devuelve
  `0xFFFFFF00` en vez de `0x00` para el negativo de `0xFF`, que ya no es un píxel válido. Funciona por las
  máscaras, no porque `lb` sea correcto.

- Captura: `cap_p2_endian.png` (`$t0 = FFFFEDED` junto a `0x200 = FF FF ED ED`; pasos en 0.6).

### Pregunta 3 — Entrada y salida

| | Pantalla de 16 colores (memoria) | Pantalla RGB (registros de comando) |
|---|---|---|
| Instrucciones para calcular **dónde** dibujar | 2 por píxel de imagen (`sll $s3, $s0, 5` y `add $s3, $s3, $s1`), que sirven para sus 2 píxeles de pantalla: **1 por píxel dibujado**; la base `0x400`/`0x410` la suma el propio `sb` | **0 por píxel**: 1 sola `sw` en `0x604` para los 1024 píxeles |
| Instrucción que dibuja | `sb $v0, 0x400($s3)` y `sb $v0, 0x410($s3)` | `sw $v0, 0x608($zero)` |
| Total en todo el dibujo | 512 instrucciones de dirección + 512 `sb` | 1 `sw` de posición + 1024 `sw` de color |
| Direcciones que ocupa el dispositivo | 512 | 2 |

Ventaja del autoincremento: sin él habría que enviar la posición antes de cada color
(`sll` de y, `or` con x y `sw` a `0x604`): 3 instrucciones y 1 escritura más por píxel, o sea
**3072 instrucciones** (+8.8 % sobre las 34950 de `0x80`) y **1024 escrituras** adicionales. Con
autoincremento los contadores x, y solo sirven para saber qué píxel leer y cuándo parar. La desventaja
es que obliga a dibujar en orden (de izquierda a derecha y de arriba abajo).

(En el ciclo RGB sí hay 4 instrucciones de direcciones por píxel — `srl`, `sll`, `srl`, `add` — pero son
para calcular de dónde se **lee** en la imagen, no dónde se dibuja.)

### Pregunta 4 — Escribir en `0x600` (experimento `experimentos/botones.asm`)

Programa: lee los botones, intenta `sw` de `0xFF` en `0x600` y `sb` en `0x603`, y los vuelve a leer.

| Botones puestos | `$t0` (antes) | `$t2` (después) | `mem[0x600]` al final | ¿Excepción? |
|---|---|---|---|---|
| `0x00` | `0x00` | `0x00` | `0x00` | no |
| `0x05` | `0x05` | `0x05` | `0x05` | no |

- **No pasa nada:** la escritura se ignora en silencio. No hay excepción y el programa sigue. El contador
  de escrituras sí sube (2 escrituras), porque la instrucción se ejecutó; el dispositivo simplemente no la obedece.
- En el código del simulador (`src/cpu.c`, función `escribir_ram`) las direcciones `0x600-0x603` se
  descartan: "los BOTONES son de entrada: las escrituras del programa no los cambian".
- Por qué tiene sentido: los botones son un dispositivo de **entrada**; su valor lo decide el usuario,
  no el programa. Si el programa pudiera escribirlos, dejarían de reflejar la realidad (un botón físico
  no se "presiona" porque el procesador escriba un 1). Es lo mismo que un registro de solo lectura de
  un periférico real.

- Captura: `cap_p4_botones.png` (`$t0 = 5`, `$t2 = 5`, sin excepción; pasos en 0.6).

### Pregunta 5 — ver la sección 7.

### Pregunta 6 — Tiempo con CPI = 1

Tiempo = instrucciones / frecuencia. Para 30 imágenes por segundo cada una debe tardar ≤ 33.3 ms.

| Caso | Instrucciones | A 1 MHz | Imágenes/s a 1 MHz | A 100 MHz | Imágenes/s a 100 MHz |
|---|---|---|---|---|---|
| **Peor caso: `0x87` (RGB, tres filtros)** | **72838** | **72.8 ms** | **13.7** → no alcanza 30 | **0.728 ms** | **1373** → sí, de sobra |
| RGB sin filtros (`0x80`) | 34950 | 35.0 ms | 28.6 → no (por poco) | 0.350 ms | 2861 |
| Peor caso en 16 colores (`0x07`) | 23621 | 23.6 ms | 42.3 → sí | 0.236 ms | 4234 |
| 16 colores sin filtros (`0x00`) | 14149 | 14.1 ms | 70.7 → sí | 0.141 ms | 7068 |

- A **1 MHz no** se llega a 30 imágenes por segundo en el peor caso (13.7); de hecho ninguna combinación
  de la pantalla RGB llega, y todas las de 16 colores sí.
- A **100 MHz sí**, con un margen de 45 veces.
- Frecuencia mínima para 30 imágenes/s en el peor caso: 72838 × 30 ≈ **2.19 MHz**.
- Es una cota optimista: un procesador multiciclo real gasta varios ciclos por instrucción (CPI > 1).

### Pregunta 7 — Por qué `-n 100000`

- El límite por defecto de ArquiSim es **10000 instrucciones** y el programa ejecuta entre **14149** (la
  combinación más corta) y **72838**. Todas las combinaciones pasan de 10000.
- Sin `-n`: el simulador se detiene a las 10000 con el mensaje "`Se alcanzó el límite de 10000
  instrucciones (¿ciclo infinito?). Use -n para cambiarlo.`" y la pantalla queda a medio dibujar (con
  `0x00` alcanza a dibujar unas 11 de las 16 filas: cada fila cuesta 884 instrucciones).
- Qué protege: de un programa que **nunca termina** (un ciclo infinito por un error). Sin límite, el
  simulador y las pruebas automáticas (`make test`) se quedarían colgados. Se vio en el experimento de la
  pregunta 8: el programa dañado ejecutó las 100000 instrucciones sin terminar y el límite lo detuvo.
- 100000 es suficiente (el peor caso usa el 73 %) y sigue protegiendo.

- Captura: `cap_p7_limite.png` (el simulador sin `-n`: `LÍMITE de 10000 instrucciones` y la imagen a medio dibujar; pasos en 0.6).

### Pregunta 8 — Registros

| Quién | Registros | Para qué |
|---|---|---|
| Programa principal | `$s0` (y), `$s1` (x), `$s2` (píxel original), `$s3` (32·y + x), `$s7` (botones) | valores que deben sobrevivir a las llamadas |
| Programa principal | `$t0`, `$t1` | cálculos cortos entre llamadas (direcciones, comparaciones) |
| Paso de parámetros | `$a0` (píxel), `$a1` (botones), `$v0` (resultado), `$ra` (retorno) | convención de llamada |
| Funciones | `$t0`, `$t1`, `$v0` (y `$a0` dentro de `APLICAR_FILTROS`) | temporales |
| `APLICAR_FILTROS` | `$sp` y la palabra `0x3FC` | guardar `$ra` antes de llamar a `BRILLO` |

Ninguna función escribe en un registro `$s`. Esa es la convención: los `$s` son "del que llama" y una
función que los use debe guardarlos y restaurarlos; los `$t` puede dañarlos cualquiera.

**Qué pasaría si una función modificara un `$s` (experimento `experimentos/pisa_s1.asm`).** Se cambió
`$t0` por `$s1` dentro de `A_16COLORES` (el programa principal usa `$s1` como x). Resultado medido
**con la imagen de ejemplo** (con otra imagen los detalles cambian: use lo que imprime
`poner_imagen.py` bajo "Pregunta 8 (pisa_s1.asm)"):

- El programa **no termina**: ejecuta las 100000 instrucciones y lo detiene el límite.
- Solo pinta 4 píxeles: (0,0) y (9,0) a la izquierda y (16,0) y (25,0) a la derecha.
- Queda con `$s0 = 0` y `$s1 = 9`: nunca sale de la fila 0.
- Explicación: `A_16COLORES` deja en `$s1` el último valor de su temporal (el bit de brillo: 0 u 8). Al
  volver, el programa principal hace `x = x + 1` sobre ese valor: x pasa de 0 a 9 y después queda
  atrapado en 9 (el píxel (9,0) también deja un 8), así que la condición `x < 16` nunca deja de cumplirse.

- Captura: `cap_p8_pisa_s1.png` (el programa dañado: límite alcanzado y solo 4 píxeles; pasos en 0.6).
- Captura: `cap_p8_pila.png` (pila en uso: `$sp = 0x3FC` y `0x3FC = 00 00 00 40`, donde `0x040` es la dirección de retorno al programa principal; pasos en 0.6).

### Pregunta 9 — Comparación con C++ (`filtros.cpp`)

`filtros.cpp` hace lo mismo con `uint8_t IMAGEN[16][16]`, `uint8_t pantalla[16][32]` y
`uint32_t pantallaRGB[32][32]`. Su salida es idéntica a la del oráculo en las 256 combinaciones de botones.

| | Ensamblador (`proyecto.asm`) | C++ (`filtros.cpp`) |
|---|---|---|
| Líneas del algoritmo | **105 instrucciones** (164 líneas en `.text` contando comentarios y separadores) | **35 líneas de código** (41 con la tabla de paletas) |
| Líneas por función: brillo | 12 | 4 |
| aplicar filtros | 25 | 11 |
| a 16 colores | 13 | 3 |
| a RGB | 9 | 3 |
| dibujar en 16 colores | 23 | 7 |
| dibujar en RGB | 20 | 5 |
| declaración de las dos pantallas | — (son dispositivos) | 2 |
| Archivo completo | 255 líneas (con encabezado e imagen) | 148 líneas (con imagen, impresión y `main`) |

Relación: 3 instrucciones de ensamblador por cada línea de C++.

Qué hace el compilador que en ensamblador se hizo a mano:

1. **Direcciones.** `IMAGEN[y][x]` en C++ es en ensamblador `sll $t0, $s0, 4` / `add $t0, $t0, $s1` /
   `lbu $s2, IMAGEN($t0)`; `pantalla[y][x + 16]` es `sll`, `add` y `sb $v0, 0x410($s3)`. El compilador
   sabe el ancho de cada arreglo y el tamaño de cada elemento; a mano hubo que saber que la imagen tiene
   16 columnas (correr 4), la pantalla 32 (correr 5) y que la base es `0x200` o `0x400`.
2. **Registros.** En C++ se escriben nombres (`x`, `y`, `p`, `Y`); a mano hubo que decidir que y es `$s0`,
   x es `$s1`, cuáles valores van en `$s` (sobreviven a una llamada) y cuáles en `$t`, y cuidar que dos
   valores vivos no compartan registro.
3. **Llamadas.** `a16Colores(aplicarFiltros(p, botones))` es una línea; a mano son `move $a0`, `move $a1`,
   `jal`, `move $a0, $v0`, `jal`, y dentro de la función el *push* y *pop* de `$ra` en la pila.
4. **Control de flujo.** `for` e `if` se vuelven `slti` + `bne`, `andi` + `beq` y etiquetas.
5. **Tipos.** `uint8_t` le dice al compilador que lea con `lbu` (sin signo) y no con `lb`; a mano es una
   decisión que el ensamblador no revisa: si se toma mal, el registro queda con un valor que no es el
   píxel y nadie avisa.
6. **Aritmética.** Uno escribe `2 * G` o `y / 2` y el compilador elige el corrimiento; a mano se escribe `sll` o `srl`.

Lo que **no** cambia: el algoritmo, las máscaras y los corrimientos son los mismos (`p ^ 0xFF`,
`(p & 0xE0) << 16`...). C++ ahorra la contabilidad, no las ideas.

- Captura: `cap_p9_cpp_0x81.png` (el programa en C++ mostrando el negativo; se compara con `cap_0x81_rgb.png`; pasos en 0.6).

## 9. ¿Negativo → grises es igual a grises → negativo?

**Sí, siempre.** Demostración (R, G de 0 a 7; B de 0 a 3; S = R + 2G + 2B, que va de 0 a 27):

1. Brillo del original: Y = ⌊(S + 2) / 4⌋.
2. Negativo y luego grises. El negativo es R' = 7 − R, G' = 7 − G, B' = 3 − B, con suma
   S' = (7 − R) + 2(7 − G) + 2(3 − B) = 27 − S. Su brillo es Y₁ = ⌊(27 − S + 2) / 4⌋ = ⌊(29 − S) / 4⌋.
3. Grises y luego negativo. El gris es (Y, Y, ⌊Y/2⌋) y su negativo (7 − Y, 7 − Y, 3 − ⌊Y/2⌋).
4. Canales R y G: hay que ver que Y₁ = 7 − Y. Como 7 = 28/4,
   7 − ⌊(S + 2)/4⌋ = ⌈(28 − S − 2)/4⌉ = ⌈(26 − S)/4⌉ = ⌊(26 − S + 3)/4⌋ = ⌊(29 − S)/4⌋ = Y₁. ✔
   (se usa que −⌊a⌋ = ⌈−a⌉ y que para enteros ⌈n/4⌉ = ⌊(n + 3)/4⌋).
5. Canal B: hay que ver que ⌊Y₁/2⌋ = 3 − ⌊Y/2⌋, o sea ⌊(7 − Y)/2⌋ = 3 − ⌊Y/2⌋.
   Si Y = 2k: ⌊(7 − 2k)/2⌋ = 3 − k. ✔  Si Y = 2k + 1: ⌊(6 − 2k)/2⌋ = 3 − k. ✔

Los tres canales coinciden, así que las dos órdenes dan el mismo píxel. La razón de fondo: el "+2" de
la fórmula redondea de forma simétrica (27 + 2 = 29 es impar frente a 4·7 = 28).

**Verificación numérica:** `python referencia.py --conmutan` → "`grises(negativo(p)) == negativo(grises(p))
en 256 de 256 píxeles`".

**Verificación en el simulador:** se ensambló una versión con el orden grises → negativo
(`modificaciones/5_orden_grises_negativo_mapa.asm`) y con `0x03` y `0x83` produce exactamente la misma
pantalla que el programa original.

Ojo: esto vale para los filtros 1 y 2. Con el filtro 3 el orden **sí** importa (mapa → negativo no es lo
mismo que negativo → mapa, porque las paletas no son simétricas).

Ejemplo con el píxel `0xB1`: negativo `0x4E` → grises `0x6D`; grises `0x92` → negativo `0x6D`.

## 10. Bitácora del desarrollo

Orden seguido (el que sugiere el enunciado). Cada etapa se ensambló, se corrió y se comparó con el oráculo
antes de pasar a la siguiente.

| Etapa | Qué se agregó | Instrucciones | Resultado |
|---|---|---|---|
| 0 | Reconocimiento: `isa.txt`, `ejecutar.c`, ejemplos 13, 14, 16, 17 y 18 | — | los 5 ejemplos pasan |
| — | Oráculo `referencia.py` (antes de escribir ensamblador) | — | Tabla 4: 24 de 24 |
| 1 | Original en la mitad izquierda (`A_16COLORES`) | 27 | pasa |
| 2 | Mitad derecha con filtro identidad | 35 | pasa |
| 3 | Botones, negativo y grises (`BRILLO`, pila) | 62 | pasa |
| 4 | Pantalla RGB sin ampliar (`A_RGB`) | 91 | pasa |
| 5 | Ampliación 2 × 2 | 93 | pasa |
| 6 | Filtro 3 y versión final comentada | 105 | 256 de 256 combinaciones |

Problemas y decisiones reales:

1. **No se podían probar los botones en automático.** Los modos sin interfaz no dejan escribir `0x600`.
   Solución: `harness.c`, un programa aparte enlazado con el `src/` original, sin cambiar el procesador.
   Se comprobó que da los mismos contadores que `./sim -q`.
2. **El programa pasa de 10000 instrucciones desde la etapa 2** (10050 con solo dibujar las dos mitades).
   Sin `-n 100000` el simulador lo corta a mitad del dibujo. De ahí sale la respuesta de la pregunta 7.
3. **Qué valores sobreviven a un `jal`.** El píxel original se necesita dos veces (izquierda y derecha) y
   entre las dos hay llamadas que usan `$t0`. Se decidió desde el principio guardarlo en `$s2`, y la
   posición de pantalla en `$s3`. El experimento `pisa_s1.asm` muestra lo que pasa si no se respeta.
4. **Función que llama a otra.** Para no escribir dos veces el cálculo del brillo (lo usan los grises y el
   mapa de color) se sacó a la función `BRILLO`. Eso obliga a `APLICAR_FILTROS` a guardar `$ra` en la
   pila; se verificó que `$sp` vuelve a `0x400` y que solo se usa la palabra `0x3FC`.
5. **`lbu` y no `lb`.** El fondo de la imagen es `0xFF`; con `lb` se cargaría como −1 (`0xFFFFFFFF`).
   Se usó `lbu` desde el inicio. Después se probó con `lb` por curiosidad: las pantallas salen iguales
   gracias a las máscaras `andi` de cada función, pero los valores intermedios dejan de ser píxeles
   válidos (detalle en la pregunta 2).
5b. **Qué pasa sin guardar `$ra`.** Se probó quitando el *push*/*pop* de `APLICAR_FILTROS`: con `0x00` y
   `0x01` el programa funciona (nunca llama a `BRILLO`), pero con `0x02` o `0x04` pinta un solo píxel y
   se queda en un ciclo infinito hasta el límite de 100000 instrucciones: el `jal BRILLO` cambia `$ra` y
   el `jr $ra` final vuelve al interior de la misma función en lugar de al programa principal.
6. **Claridad antes que tamaño.** Había una versión de `A_16COLORES` de 10 instrucciones (sacando dos bits
   con una sola máscara) y una de `BRILLO` de 10. Como sobraban instrucciones se dejaron las versiones
   de 13 y 12, donde cada bit o canal sigue el mismo patrón "correr, aislar, juntar".
7. **Ampliación 2 × 2 simple, no rápida.** Se recorre la pantalla y se lee (x/2, y/2), así que cada píxel
   se filtra 4 veces. Filtrar una vez y escribir el color dos veces ahorraría instrucciones, pero la
   segunda fila de cada bloque obligaría a repetir la fila o a guardar colores en memoria. El peor caso
   (72838) cabe en el límite, así que se dejó la versión que se explica en una frase.
8. **Paleta térmica en 16 colores.** Al previsualizar se vio que dos niveles (Y = 5 y Y = 6) dan el mismo
   color en la pantalla de 16 colores. Es una limitación de esa pantalla; se documentó (sección 4.4).
9. **Hallazgos:** los contadores no dependen de la imagen (sección 7) y negativo/grises conmutan (sección 9).
10. **Precaución con el ensamblador:** lee líneas de máximo 255 caracteres; los comentarios se mantuvieron cortos.
11. **Una prueba que falló a propósito sin querer:** la primera corrida de `probar.py` se hizo por error
    contra la plantilla vacía y dio 0 de 12. Sirvió para confirmar que las pruebas sí detectan un programa que no dibuja.

Las etapas 1 a 6 pasaron contra el oráculo en la primera ejecución; no hubo errores de lógica en el
ensamblador que corregir. No se encontró ningún error en el simulador.

## 11. Ideas para las conclusiones

- Con 105 instrucciones y 708 bytes se hace procesamiento de imágenes: todo se reduce a máscaras,
  corrimientos y direcciones base + desplazamiento.
- La forma de conectar un dispositivo cambia el programa: la pantalla mapeada en memoria necesita
  calcular una dirección por píxel; la de registros de comando no, pero obliga a dibujar en orden.
- La convención de llamada (`$a0`, `$v0`, `$ra`, `$s`/`$t`, pila) no es burocracia: romperla en un solo
  registro colgó el programa.
- El costo se puede predecir contando instrucciones por píxel: el modelo `fijo + filas × 4 + píxeles ×
  costo` reproduce los 12 contadores exactamente.
- Una tabla (*look-up table*) convierte un cálculo en una lectura de memoria: 4 filtros con 7 instrucciones.
- El compilador hace por uno direcciones, registros y llamadas: 3 instrucciones por cada línea de C++.

## 12. Pendiente

- **Imagen propia.** El enunciado pide entregar el resultado con una imagen del grupo. `proyecto.asm` y
  `filtros.cpp` todavía tienen la imagen de ejemplo (`imagen/ejemplo.bmp`). Se cambia con un solo comando,
  `python herramientas\poner_imagen.py "RUTA"`, que puede ejecutar Astra (pasos y revisiones en 0.3; qué
  cambia en el informe, en 0.8). Los contadores de las secciones 3, 7 y 8 no dependen de la imagen.
- Todas las capturas de las secciones 0.5 y 0.6 (carpeta `capturas/`).
- El informe en PDF (`informe_proyecto_final.pdf`), con las secciones de 0.7.
- Nombres de los integrantes.

## 13. Archivos de la entrega

| Archivo | Qué es |
|---|---|
| `proyecto.asm` | el programa (105 instrucciones), comentado línea por línea |
| `imagen/` | imagen original y `.bmp` (por ahora `ejemplo.bmp`) |
| `filtros.cpp` | el mismo algoritmo en C++ |
| `referencia.py` | oráculo en Python |
| `datos_informe.md` | este archivo |
| `GUIA_SUSTENTACION.md` | guía para sustentar el código |
| `herramientas/` | `harness.c`, `probar.py`, `trazas.py`, `experimentos.py`, `probar_modificaciones.py`, `poner_imagen.py`, `img_a_arquisim.c` |
| `capturas/` | carpeta vacía donde van las capturas `cap_….png` |
| `experimentos/` | `endian.asm`, `botones.asm`, `pisa_s1.asm` (preguntas 2, 4 y 8) |
| `modificaciones/` | versiones del programa con cada modificación de la guía, ya probadas |

Comandos para repetir todo (desde `entrega/`; en WSL el intérprete se llama `python3`, y desde la carpeta
del simulador las rutas llevan `entrega/` delante, como en la sección 0):

```
python referencia.py                          # Tabla 4 y resumen
python herramientas/probar.py --todas         # 256 combinaciones en el simulador
python herramientas/probar.py --tabla         # tabla de la sección 7
python herramientas/experimentos.py           # preguntas 2, 4 y 8
python herramientas/probar_modificaciones.py  # modificaciones de la guía
g++ -std=c++11 -O2 -o filtros filtros.cpp     # programa en C++
```
