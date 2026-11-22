# Proyecto final ArquiSim: reparto del trabajo y guía del equipo

Léanlo completo una vez (15 minutos). Después cada uno se queda con su parte.

- **Persona 1**: ya ha usado ArquiSim. Se encarga de las **pantallas, las direcciones y la entrada/salida**.
- **Persona 2**: conoce el proyecto completo (es quien coordina). Se encarga de los **filtros, las funciones y la pila**, de la imagen propia, del informe y de la entrega.
- **Persona 3**: nunca ha abierto ArquiSim. Empieza por la instalación y el tutorial de la sección 3, y se encarga de los **temas de aritmética de bits, de la comparación con C++ y de la presentación inicial**.

En la sustentación el profesor puede preguntarle cualquier cosa a cualquiera. Por eso hay un **mínimo
común** que los tres deben saber (sección 5) y, encima de eso, un tema propio para cada uno (sección 6).

---

## 1. Qué es esto (en 1 minuto)

- **ArquiSim** es el simulador de un procesador MIPS que construimos en el Taller 2. Corre en la terminal:
  se le da un programa en ensamblador, lo ejecuta instrucción por instrucción y muestra registros,
  memoria y dos "pantallas".
- **El proyecto final** es un programa en ese ensamblador (`entrega/proyecto.asm`, 105 instrucciones)
  que toma una imagen de 16 × 16 píxeles, lee 8 botones y la dibuja aplicando filtros: negativo, escala de
  grises y un filtro nuestro (mapa de color con 4 paletas). Hay dos pantallas: una de 16 colores (original a
  la izquierda, filtrada a la derecha) y una RGB de 32 × 32 (la filtrada, ampliada al doble).
- Ya está hecho y probado: pasa las 256 combinaciones de botones. Lo que falta es **entenderlo para
  defenderlo**, poner la imagen del grupo, tomar las capturas y armar el informe.

## 2. Instalación (cada uno, en su computador)

El simulador es un programa de **Linux**. En Windows se usa con **WSL** (un Linux dentro de Windows). El
repositorio **no trae el simulador compilado**: se compila con un comando (`make`).

### 2.1 Windows (WSL)

1. **Instalar WSL** (solo la primera vez). Abra **PowerShell como administrador** (clic derecho en el menú
   Inicio → "Terminal (Administrador)") y escriba:

   ```
   wsl --install
   ```

   Reinicie el computador cuando lo pida. Al volver se abre sola una ventana de **Ubuntu** que pide crear
   un usuario y una contraseña de Linux (pueden ser cualquiera; **anote la contraseña**, se usa con `sudo`).
   Si la ventana no se abre, búsquela en el menú Inicio como "Ubuntu".

   Si ya tiene WSL, salte este paso. Si el computador no le deja instalarlo (computador de la universidad,
   sin permisos de administrador), avise al grupo: practique en el computador de otro.

2. **Instalar el compilador, git y Python** (en la ventana de Ubuntu; pide la contraseña del paso 1, que no
   se ve mientras se escribe):

   ```
   sudo apt update
   sudo apt install -y build-essential git python3
   ```

3. **Bajar el repositorio y compilar el simulador**:

   ```
   cd ~
   git clone https://github.com/AndMesa/ArquiSim.git
   cd ArquiSim
   make
   make test
   ```

   La última línea debe decir **`Resultado: 25 de 25 programas pasan`**.

4. **Comprobar el proyecto**:

   ```
   python3 entrega/herramientas/probar.py
   ```

   Deben salir 12 líneas con `PASA` y al final `12 de 12 combinaciones pasan`.

5. **Abrir el proyecto en el simulador**. Use **Windows Terminal** (ya viene en Windows 11) con la ventana
   **maximizada**: la interfaz necesita al menos 107 × 41 caracteres. Si no cabe, reduzca la letra con
   `Ctrl` + `-`.

   ```
   export COLORTERM=truecolor
   ./sim -n 100000 programas/proyecto.asm
   ```

   El `export` hace que la pantalla RGB muestre los colores exactos; repítalo cada vez que abra la
   terminal. Siga el tutorial de la sección 3.

Cada vez que vuelva a abrir Ubuntu: `cd ~/ArquiSim`.

### 2.2 Mac

En la Terminal:

```
xcode-select --install
cd ~
git clone https://github.com/AndMesa/ArquiSim.git
cd ArquiSim
make
make test
python3 entrega/herramientas/probar.py
./sim -n 100000 programas/proyecto.asm
```

(`xcode-select --install` instala el compilador, git y Python; si dice que ya están, siga.) La Terminal
de macOS no muestra color de 24 bits: los colores de la pantalla RGB se ven aproximados. Para ver los
exactos use iTerm2 o VS Code.

### 2.3 Recibir los cambios que suba Persona 2

```
cd ~/ArquiSim
git pull
```

**No edite** los archivos del repositorio (`entrega/proyecto.asm`, `programas/proyecto.asm`, los `.md`):
si los cambia, `git pull` puede fallar. Para practicar modificaciones, haga una copia con otro nombre:

```
cp entrega/proyecto.asm programas/mi_prueba.asm
./sim -n 100000 programas/mi_prueba.asm
```

Si `git pull` dice que sus cambios locales se perderían: `git stash` y otra vez `git pull`.

### 2.4 Leer los documentos

Los documentos `.md` se leen mejor en GitHub, que los muestra con formato:
**https://github.com/AndMesa/ArquiSim/tree/main/entrega** (clic en el archivo). También se pueden abrir en
VS Code si lo tiene instalado (`code .` desde la carpeta en Ubuntu abre VS Code conectado a WSL; ahí, clic derecho en el `.md` →
"Open Preview").

El enunciado (`proyecto_final.pdf`) **no está en el repositorio**: Persona 2 lo manda por el grupo.

## 3. Tutorial: los primeros 20 minutos con ArquiSim (Persona 3 obligatorio; los demás, repaso)

Abra el proyecto (`./sim -n 100000 programas/proyecto.asm`). Los comandos se escriben abajo, en la línea
que empieza con `>`, y se confirman con Enter.

| Paso | Escriba | Qué pasa / qué mirar |
|---|---|---|
| 1 | (nada, solo mire) | A la izquierda el **programa**: cada línea tiene la dirección (`0x000`, `0x004`…), el código en hexadecimal y la instrucción. A la derecha los **32 registros**, la **memoria**, la **pila** y los **botones**. Arriba, los **contadores**. |
| 2 | Enter | Ejecuta **una** instrucción: `lw $s7, 0x600($zero)` (leer los botones). Cuando una instrucción cambia un registro, ese registro queda marcado con `*`. |
| 3 | Enter (varias veces) | Avance despacio. Mire cómo cambian `$s0` (fila y) y `$s1` (columna x). |
| 4 | `0` | Reinicia el programa desde el principio. |
| 5 | `mem[0x600] = 0x01` | Enciende el **botón 0** (negativo). En el recuadro BOTONES queda marcado el 0. |
| 6 | `c` | Ejecuta **todo** hasta el final. El panel izquierdo cambia solo a la **pantalla de 16 colores**: original a la izquierda, negativo a la derecha. Abajo dice `FIN DEL PROGRAMA: 14405 instrucciones ejecutadas`. |
| 7 | `p` | Cambia de panel: programa → pantalla de 16 colores → pantalla RGB → consola → programa. |
| 8 | `0`, luego `mem[0x600] = 0x81`, luego `c` | Botones 0 y 7: negativo en la **pantalla RGB** (32 × 32). |
| 9 | `0`, `mem[0x600] = 0x02`, `bp BRILLO`, `c` | Pone un **punto de parada** en la función `BRILLO` y corre hasta ahí (el panel izquierdo queda en la pantalla; con `p` vuelve al programa). Mire `$a0` (el píxel que entra). |
| 10 | Enter × 11 | Ejecuta `BRILLO` instrucción por instrucción hasta quedar sobre su `jr $ra`. Mire `$v0`: es el brillo (0 a 7). |
| 11 | `q` | Sale. El simulador imprime el estado final y las estadísticas. |

Otros comandos útiles: `b` (retroceder una instrucción), `bt 3` (prender o apagar el botón 3), `m 0x200`
(ver la memoria desde la imagen), `bp` (borrar puntos de parada).

**Vocabulario mínimo** (Persona 3):

| Palabra | Qué es |
|---|---|
| registro | una "variable" del procesador, de 32 bits. Hay 32: `$s0`…`$s7`, `$t0`…`$t9`, `$a0`, `$v0`, `$ra`, `$sp`… |
| `$s` / `$t` | `$s`: valores que deben sobrevivir cuando se llama una función. `$t`: temporales, cualquiera los puede dañar. |
| `$a0` / `$v0` / `$ra` | argumento de una función / resultado de una función / dirección a donde volver. |
| `jal` / `jr $ra` | llamar una función / volver de ella. |
| `lbu` / `sb` | leer un byte de memoria (sin signo) / escribir un byte. |
| `lw` / `sw` | leer / escribir una palabra (4 bytes). |
| `sll` / `srl` | correr bits a la izquierda (multiplicar por 2ⁿ) / a la derecha (dividir por 2ⁿ). |
| `andi` / `or` / `xori` | máscara (dejar solo ciertos bits) / juntar bits / invertir bits. |
| inmediato | un número escrito dentro de la instrucción, como el `0x80` de `andi $t0, $s7, 0x80`. |
| RGB332 | formato del píxel: 1 byte = 3 bits de rojo, 3 de verde, 2 de azul (`RRRGGGBB`). |
| pila | memoria donde una función guarda `$ra` antes de llamar a otra (`$sp` apunta al tope). |

## 4. Qué leer (todo está en `entrega/`)

| Archivo | Qué es | Quién |
|---|---|---|
| `EQUIPO.md` | este archivo | los tres |
| `GUIA_SUSTENTACION.md` | explicación del programa línea por línea, trazas, preguntas y modificaciones | los tres, por partes (abajo) |
| `proyecto.asm` | el programa (cada línea tiene un comentario en español) | los tres |
| `datos_informe.md` | todos los datos medidos y las respuestas de las 9 preguntas del enunciado | por partes (abajo) |
| `filtros.cpp` | el mismo programa en C++ | Persona 3 |
| `modificaciones/` | 10 versiones del programa con cambios típicos, ya probadas | cada uno las suyas |

## 5. Mínimo común: lo que los tres deben saber responder

**Leer:** `GUIA_SUSTENTACION.md` secciones **0** (el programa en 30 segundos), **1** (flujo), **3**
(registros) y **9** (hoja de datos).

Cada uno debe poder contestar sin ayuda, en dos frases:

1. ¿Qué hace el programa? (GUIA 0)
2. ¿Cuántas instrucciones tiene y cuántas ejecuta? (105; de 14149 a 72838; por eso `-n 100000`)
3. ¿Cómo hace el negativo en una instrucción? (`xori 0xFF`, GUIA 5.1)
4. ¿Por qué `lbu` y no `lb`? (GUIA 7, pregunta 2)
5. ¿Por qué `APLICAR_FILTROS` guarda `$ra` en la pila? (GUIA 7, pregunta 4)
6. ¿Cómo se calcula la dirección de un píxel de la imagen? (`0x200 + 16·y + x`, GUIA 6.1)
7. ¿Qué es el autoincremento de la pantalla RGB? (GUIA 6.3)
8. ¿Qué hace nuestro filtro 3? (GUIA 5.3)

Y deben poder hacer en el simulador, sin mirar: abrir el proyecto, poner los botones, ejecutar, cambiar de
panel y poner un punto de parada (tutorial de la sección 3).

## 6. Reparto por persona

### 6.1 Persona 1: pantallas, direcciones y entrada/salida

**Temas que domina en la sustentación:** los dos ciclos del programa principal, cómo se calcula cada
dirección, la ampliación 2 × 2, el autoincremento, los botones, `A_16COLORES` y `A_RGB`, *big-endian* y la
codificación de instrucciones.

**Leer:**
- `GUIA_SUSTENTACION.md`: 2.1 a 2.4 (inicio y los dos ciclos), 2.7 y 2.8 (`A_16COLORES`, `A_RGB`),
  5.4 y 5.5 (bits de cada color), 6 completa (direcciones y 2 × 2), en la sección 4 las trazas de
  `A_16COLORES` y `A_RGB`.
- `datos_informe.md`: sección 8, preguntas **1, 2, 3 y 4**.

**Preguntas de la GUIA (sección 7) que le tocan:** 2, 3, 9, 10, 11, 12, 13, 14, 19, 20, 27, 28, 31.

**Modificaciones en vivo que debe saber hacer** (GUIA 8; las versiones probadas están en `modificaciones/`):
- 8.1 poner la original a la derecha (`1_original_a_la_derecha.asm`)
- 8.2 espejo horizontal y vertical (`2_espejo_horizontal.asm`, `2b_espejo_vertical.asm`)
- 8.4 pantalla RGB sin ampliar (`4_rgb_sin_ampliar.asm`)

**Tareas del proyecto:**
- Revisar las capturas que tome Astra: que cada una muestre el panel y los contadores de la tabla 0.5 de
  `datos_informe.md`. Si un número no coincide, avisar a Persona 2.
- En el informe, revisar la sección de capturas y las respuestas de las preguntas 1 a 4.
- En la sustentación, **manejar el simulador** cuando el profesor pida ver algo (botones, pantallas,
  puntos de parada).

### 6.2 Persona 2 (coordina): filtros, funciones, pila, imagen, informe y entrega

**Temas que domina en la sustentación:** `APLICAR_FILTROS` (cómo se combinan los filtros y en qué orden),
`BRILLO`, el filtro 3 y por qué se eligió, la pila y `$ra`, qué registro usa cada parte, los contadores
y los tiempos.

**Leer:**
- `GUIA_SUSTENTACION.md`: 2.5 y 2.6 (`APLICAR_FILTROS`, `BRILLO`), 5.2 y 5.3 (grises y mapa de color),
  en la sección 4 las trazas de `BRILLO` y de `APLICAR_FILTROS`, 8.7 (cómo no enredarse al modificar).
- `datos_informe.md`: secciones **4** (filtro 3), **7** (tabla de contadores y por qué cambian) y **10**
  (bitácora); sección 8, preguntas **5, 6, 7 y 8**.

**Preguntas de la GUIA (sección 7) que le tocan:** 4, 5, 6, 7, 8, 18, 21, 23, 29, 32, 33.

**Modificaciones en vivo:**
- 8.3 cambiar el filtro 3: cambiar una paleta, dejar solo un canal, intercambiar rojo y verde
  (`3a_…`, `3b_…`)
- 8.5 otro orden de filtros (`5_orden_grises_negativo_mapa.asm`)

**Tareas del proyecto:**
1. Elegir con el grupo la **imagen propia** (ícono, logo, bandera o dibujo de colores planos; no fotos) y
   ponerla (sección 0.3 de `datos_informe.md`; lo puede hacer Astra).
2. Coordinar a Astra: capturas y redacción del informe (sección 0 de `datos_informe.md`).
3. Subir a GitHub la imagen y las capturas para que el grupo haga `git pull`.
4. Revisión final del informe y armar el archivo comprimido de la entrega.

### 6.3 Persona 3: aritmética de bits, C++ y presentación

**Temas que domina en la sustentación:** abrir la sustentación con "el programa en 30 segundos", el
negativo, la fórmula de grises (por qué sumas y corrimientos, el `+2`, el `B' = Y/2`), por qué negativo y
grises dan lo mismo en cualquier orden, la comparación con C++, y qué instrucciones existen (tipos R, I, J,
pseudoinstrucciones).

**Leer, en este orden:**
1. Este archivo y el tutorial de la sección 3 (con el simulador abierto).
2. `GUIA_SUSTENTACION.md`: 0, 1, 2.2 (las 3 primeras instrucciones), 5.1 y 5.2 (negativo y grises), en
   la sección 4 la traza de `APLICAR_FILTROS` con botones `0x01` (negativo).
3. `filtros.cpp` (el programa en C++: es la forma más fácil de entender qué hace el ensamblador).
4. `datos_informe.md`: sección **9** (negativo y grises conmutan) y sección 8, pregunta **9** (C++).

**Preguntas de la GUIA (sección 7) que le tocan:** 1, 15, 16, 17, 22, 24, 25, 26, 30.

**Modificación en vivo** (la más sencilla, GUIA 8.6): que el **botón 6** active el negativo en vez del 0.
Es cambiar un solo número: `andi $t0, $a1, 0x01` → `andi $t0, $a1, 0x40`
(`modificaciones/6a_boton_6_activa_el_negativo.asm`). Practíquela así:

```
cp entrega/proyecto.asm programas/mi_prueba.asm
nano programas/mi_prueba.asm          # buscar con Ctrl+W: andi $t0, $a1, 0x01 ; cambiar 0x01 por 0x40 ; guardar con Ctrl+O, salir con Ctrl+X
./sim -n 100000 programas/mi_prueba.asm
```

y dentro del simulador: `mem[0x600] = 0x40`, `c`. La mitad derecha debe salir en negativo.

**Saber compilar y mostrar el C++:**

```
g++ -std=c++11 -O2 -o filtros entrega/filtros.cpp
./filtros 0x81
```

Dibuja en la terminal la imagen en negativo, igual que el simulador con `0x81`.

**Tareas del proyecto:**
- En el informe, revisar que la introducción, la pregunta 9 y las conclusiones se entiendan (si usted lo
  entiende, el profesor también).
- Preparar y decir en la sustentación el resumen de 30 segundos (GUIA 0).

## 7. Plan de estudio

| Día | Los tres | Persona 1 | Persona 2 | Persona 3 |
|---|---|---|---|---|
| 1 | Instalar (sección 2) y avisar en el grupo cuando `make test` diga 25 de 25 | — | poner la imagen propia y subirla | tutorial de la sección 3 completo |
| 2 | Leer el mínimo común (sección 5) | leer sus secciones | leer sus secciones; Astra toma capturas | leer sus secciones 1 y 2 |
| 3 | — | practicar 8.1, 8.2, 8.4 en `mi_prueba.asm` | practicar 8.3 y 8.5; Astra redacta el informe | leer `filtros.cpp`; practicar 8.6 |
| 4 | **Simulacro 1** (sección 8) | revisar capturas | revisar el informe | revisar intro, pregunta 9 y conclusiones |
| 5 | **Simulacro 2** con preguntas cruzadas | | entregar | |

## 8. Simulacro (hacerlo dos veces, 30 minutos)

1. Persona 3 presenta el programa en 30 segundos.
2. Persona 1 lo corre en el simulador con `0x00`, `0x07` y `0x87` y explica las dos pantallas.
3. Persona 2 explica `APLICAR_FILTROS` con un punto de parada (`bp APLICAR_FILTROS`).
4. Preguntas: cada uno le hace al otro 5 preguntas de la GUIA sección 7 **que no son suyas**. La respuesta
   debe caber en dos frases.
5. Cada uno hace su modificación en vivo, en menos de 3 minutos, con el programa corriendo después.
6. Segundo simulacro: las preguntas se sacan al azar entre las 33, para cualquiera.

## 9. El día de la sustentación

- [ ] Al menos **dos** computadores con el simulador compilado y probado (`make test`, `probar.py`).
- [ ] `git pull` hecho la noche anterior.
- [ ] Windows Terminal maximizado, `cd ~/ArquiSim`, `export COLORTERM=truecolor`.
- [ ] El proyecto abierto: `./sim -n 100000 programas/proyecto.asm`.
- [ ] Una copia limpia lista para modificar: `cp entrega/proyecto.asm programas/mi_prueba.asm`.
- [ ] `GUIA_SUSTENTACION.md` sección 9 (hoja de datos) abierta en el celular.
- [ ] Si se pide una modificación que no practicamos: decir en voz alta qué se va a cambiar y en qué
  bloque antes de escribir (GUIA 8.7).
- [ ] Si no sabe una respuesta, dígalo y deje que conteste el compañero dueño del tema; no adivine.
