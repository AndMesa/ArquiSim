# ArquiSim

**ArquiSim** es un simulador en terminal de un procesador MIPS reducido de 32 bits. Lee un programa
en ensamblador, lo traduce a binario, lo carga en la RAM y lo ejecuta instrucción
por instrucción, mostrando cómo cambian los registros y la memoria.

## Compilar y ejecutar

```sh
make                                # compila ./sim
./sim                               # interfaz interactiva: elige un programa de programas/
./sim programas/00_suma.asm         # interfaz interactiva con ese programa
./sim --traza programas/00_suma.asm # imprime todos los pasos, sin interfaz
make test                           # ejecuta todos los programas de prueba
```

### Interfaz interactiva

La pantalla se redibuja en cada instrucción (la terminal debe medir al menos 103 × 31):

```
┌─┤ ArquiSim ├──────────────────────────────────────────────────────────────────────────────────┐
│ programa: 05_ciclo.asm   paso: 6   PC = 0x018   R 2  I 4  J 0 | lecturas 1  escrituras 0     │
├─┤ PROGRAMA (.text: 9 instrucciones, 36 B) ├──────────┬─┤ BANCO DE REGISTROS (32 x 32 bits) ├─┤
│       0x014  8D0A0000        lw   $t2, 0($t0)        │  $t0   0000020C    $s0   00000200     │
│ PC>   0x018  022A8820        add  $s1, $s1, $t2      │ *$t2   00000004    $s1   00000000     │
│     o 0x020  08000003        j    LOOP               │  PC 00000018  IR 8D0A0000  ACC 0      │
├─┤ ÚLTIMA INSTRUCCIÓN EJECUTADA (IR) ├────────────────┼─┤ MEMORIA desde 0x200  [m dir] ├──────┤
│ IR = 0x8D0A0000   formato I   lw   $t2, 0($t0)       │  0x200 = 00 00 00 01          1 A:    │
│ 100011 01000 01010 0000000000000000                  │  0x204 = 00 00 00 02          2       │
│ op     rs    rt    imm                               ├─┤ PILA vacía ($sp = 0x400) ├──────────┤
├─┤ CAMBIOS EN ESTE PASO ├─────────────────────────────┤  0x3FC . 00 00 00 00          0       │
│ $t2       0x00000000 -> 0x00000004  (4)              │                                       │
├──────────────────────────────────────────────────────┴───────────────────────────────────────┤
│ EJECUTANDO: la siguiente instrucción está en PC = 0x018                                       │
└───────────────────────────────────────────────────────────────────────────────────────────────┘
```

Las asignaciones modifican el estado actual. Si se hacen antes de la primera instrucción
(paso 0) se conservan al reiniciar con `0`, lo que sirve para probar el mismo programa con
otros datos de entrada.

El encabezado muestra las **estadísticas** de la ejecución: instrucciones por formato
(R, I, J), lecturas y escrituras de datos en memoria y saltos tomados. Al salir se
muestran también en porcentaje.

En el banco de registros, cada registro muestra su **dirección de 5 bits** (`01000 $t0`):
es el número que aparece en los campos rs, rt y rd de la instrucción. Debajo están los
registros **internos** (PC, IR, ACC, A, B, TEMP y FLAGS), que no tienen dirección.

`o` marca un punto de parada; `PC>` marca la siguiente instrucción y `*` los registros o palabras que cambiaron. En la
memoria, `#` es código, `=` datos y `s` pila.

| Comando   | Efecto |
|-----------|--------|
| `Enter`   | Ejecuta una instrucción. |
| `b`       | Retrocede una instrucción. |
| `c`       | Continúa hasta el final (o hasta una excepción). |
| `p`       | Alterna el panel izquierdo: programa → pantalla (16 colores) → pantalla RGB → consola. |
| `bt 3`    | Activa o desactiva el botón 3 (`bt` solo los apaga todos). |
| `l`       | Muestra la lista de programas para cargar otro (`l ruta.asm` lo carga directamente). |
| `e`       | Abre el programa en un editor y lo recarga al cerrarlo. Usa la variable `EDITOR` (por defecto `nano`; para VS Code: `export EDITOR="code --wait"`). |
| `r`       | Recarga el programa actual desde el disco (si lo editó en otra ventana). |
| `bp LOOP` | Activa o desactiva un punto de parada (etiqueta o dirección); `c` y `a` se detienen allí. `bp` solo los borra todos. |
| `a [ms]`  | **Tiempo real**: ejecuta una instrucción cada *ms* milisegundos (por defecto 400). Mientras corre, las teclas actúan sin Enter: `0`–`7` cambian los botones, `+`/`-` cambian la velocidad, espacio pausa. |
| `m 0x200` | Mueve la ventana de memoria a esa dirección (también acepta etiquetas: `m A`). Solo cambia lo que se ve. |
| `$t0 = 42` | Escribe un valor en un registro (`acc = 5` para el acumulador). |
| `mem[0x200] = -5` | Escribe una palabra en memoria (dirección múltiplo de 4, o una etiqueta: `mem[A] = 7`). |
| `0`       | Reinicia: vuelve al paso 0 sin leer el archivo (conserva lo asignado a mano en el paso 0). |
| `t`       | Cambia el tema de colores (`sobrio`, `clasico`, `azul`, `inverso`, `sin-gris`). |
| `↑` / `↓`  | Recorren los comandos anteriores (historial). |
| `q`       | Sale y muestra el estado final. |

### Opciones

| Opción        | Efecto |
|---------------|--------|
| `--traza`     | Sin interfaz: imprime cada paso uno tras otro (útil para guardar en un archivo). Es el modo por defecto si la salida no es una terminal. |
| `-a`          | Con `--traza`: muestra el banco de registros completo en cada paso. |
| `-q`          | Silencioso: solo el estado final. |
| `-t`          | Prueba: compara el estado final con `programa.esperado`. |
| `-i archivo`  | Usa otro archivo de ISA (por defecto `isa.txt`). |
| `-n N`        | Máximo de instrucciones a ejecutar (por defecto 10000). |
| `--sin-color` | Desactiva los colores. |
| `--ascii`   | Bordes con `+ - \|` en lugar de cajas (si la terminal no las muestra bien). |
| `--tema T`  | Tema de colores al iniciar (también con la variable `ARQUISIM_TEMA`). |

## Estructura

| Archivo | Qué es |
|---|---|
| `isa.txt` | **El ISA**: qué instrucciones existen y cómo se codifican (el contrato). | 
| `programas/*.asm` | **El software**: programas de prueba. | 
| `programas/*.esperado` | Estado final esperado de cada programa. | 
| `src/ejecutar.c` | **La microarquitectura**: qué hace el hardware con cada instrucción. | 
| `src/main.c` | Opciones y modos de ejecución. | 
| `src/ciclo.c` | Ciclo Fetch → Decode → Execute. | 
| `src/isa.c` | Lectura de `isa.txt` y decodificación (la unidad de control). | 
| `src/ensamblador.c` | Traduce `.asm` a binario y lo carga en la RAM. | No |
| `src/cpu.c`, `src/cpu.h` | Estado de la máquina y herramientas de acceso a memoria. | 
| `src/tui.c`, `src/terminal.c` | Interfaz de pantalla completa. | 
| `src/mostrar.c` | Salida de texto (modo `--traza` y estado final). |

## Actividad

El proyecto base solo trae `add` y `addi`. Para cada instrucción nueva hay que trabajar
en las **dos capas**:

1. **ISA** (`isa.txt`): agregue una línea con su formato, opcode, funct y operandos,
   según la codificación dada en clase. Sin esa línea, el ensamblador responde:
   ```
   programas/01_aritmetica.asm:13: error: instrucción desconocida 'sub' (no está definida en el ISA)
   ```
2. **Microarquitectura** (`src/ejecutar.c`): implemente lo que hace. Sin eso, el procesador
   se detiene con una excepción:
   ```
   EXCEPCIÓN en PC = 0x018: instrucción 'sub' no implementada (ver src/ejecutar.c)
   ```

Orden sugerido:

| Programa | Instrucciones | Concepto |
|---|---|---|
| `01_aritmetica` | `sub`, `addu`, `addiu`, `subu` | ALU y desbordamiento |
| `02_logica` | `and`, `or`, `xor`, `nor`, `andi`, `ori`, `xori`, `lui`, `sll`, `srl`, `sra` | Máscaras; extensión de ceros vs. de signo |
| `03_memoria` | `lw`, `sw` | Base + desplazamiento, alineación |
| `04_if` | `bne`, `beq`, `j` | Saltos relativos al PC y pseudodirectos |
| `05_ciclo` | `slt`, `sltu`, `slti` | Comparaciones |
| `06_funcion` | `jal`, `jr` | Llamadas a función y `$ra` |
| `07_texto` | `lb`, `lbu`, `sb` | Acceso a bytes |
| `08_excepcion` | — | `add`/`addi` deben detectar el desbordamiento |
| `09_pantalla` | (las anteriores) | E/S mapeada en memoria y arreglos 2D |
| `10_direcciones` | — (funciona con el proyecto base) | Instrucciones de 3, 2 y 1 dirección |
| `11_acumulador` | `mtacc`, `mfacc`, `suba`, `addia` | Máquina de acumulador |
| `12_pseudo` | `move`, `li`, `la`, `nop`, `clear`, `b` (solo en `isa.txt`) | Pseudoinstrucciones |
| `13_botones` | (las anteriores) | Entrada mapeada en memoria y sondeo |
| `14_pila` | (las anteriores) | Guardar `$ra` en la pila al llamar funciones |
| `15_banderas` | `cmp`, `jz`, `jn`, `jc`, `jv` | Registro de banderas (como en el PDUA) |
| `16_datos` | (las anteriores) | Variables y arreglos en `.data`: traer con `lw`, guardar con `sw` |
| `17_interrupciones` | `ei`, `di`, `eret`, `mfepc` | Interrupciones de botones y temporizador |
| `18_pantalla_rgb` | (las anteriores) | Dispositivo con registros de comando (pantalla RGB) |
| `19_mult_div` | `mult`, `multu`, `div`, `divu`, `mfhi`, `mflo`, `mul` | Registros HI/LO; saltos `blt`/`ble` con `$at` |
| `20_consola` | `syscall` | Consola: imprimir y leer (servicios del sistema) |

Después de cada grupo ejecute `make test`. Cada programa que falla le dice qué valor
esperaba y cuál obtuvo.

## La máquina

| Recurso | Tamaño |
|---|---|
| PC, IR | 32 bits |
| Banco de registros | 32 registros × 32 bits = 128 bytes (`$zero` siempre vale 0) |
| RAM (unificada, Von Neumann) | 1024 bytes, direccionada por byte, **big-endian** |
| Banderas Z N C V | Cero, negativo, acarreo/préstamo, desbordamiento (como en el PDUA) |
| A, B, TEMP | Registros internos (el programador no los ve): A y B reciben los operandos en el *decode*, TEMP guarda el resultado de la ALU (o la dirección en `lw`/`sw`) |
| HI, LO | Resultado de `mult` (64 bits: HI = parte alta, LO = baja) y de `div` (LO = cociente, HI = residuo) |
| ACC (acumulador) | 32 bits, fuera del banco de registros (como HI/LO en MIPS o el ACC del PDUA) |
| Pantalla (E/S mapeada en memoria) | 32 × 16 píxeles, 1 byte por píxel = 512 bytes |
| Botones (E/S mapeada en memoria) | 8 botones en los 8 bits bajos de la palabra 0x600 (solo lectura) |

Mapa de memoria:

```
0x000 - 0x1FF   .text   código (hasta 128 instrucciones)
0x200 - ...     .data   datos
  ...  - 0x3FF  pila    $sp inicia en 0x400 y crece hacia abajo
0x400 - 0x5FF   video   pantalla de 32 x 16 píxeles
0x600           botones       8 botones (bits 7..0), solo lectura
0x604           temporizador  interrumpir cada N instrucciones (0 = apagado)
0x608           vector        dirección de la rutina de servicio (ISR)
0x60C           causa         bit 0 = botones, bit 1 = temporizador (solo lectura)
0x610           rgb posición  pantalla RGB: (y << 16) | x
0x614           rgb color     pantalla RGB: escribir 0x00RRGGBB dibuja y avanza
```

### Pantalla RGB (registros de comando)

Es otra forma de conectar una pantalla. La imagen (32 × 32, 24 bits por píxel) vive
**dentro del dispositivo**, no en la memoria del procesador: el programa solo usa dos
registros. Primero escribe la posición en 0x610 (`(y << 16) | x`) y luego el color en
0x614 (`0x00RRGGBB`); la escritura del color dibuja el píxel y **avanza x** sola
(autoincremento), así que para llenar una fila basta enviar colores seguidos.

| | Pantalla 0x400 (*framebuffer*) | Pantalla RGB (comandos) |
|---|---|---|
| Direcciones que ocupa | 512 (una por píxel) | 2 |
| Color | 16 colores (4 bits) | 24 bits |
| Pintar un píxel | 1 `sb` | 1 `sw` de posición + 1 `sw` de color (o solo el color, con autoincremento) |

Con `p` se alterna entre el programa, la pantalla de 16 colores y la RGB. Si la terminal
soporta color de 24 bits (`COLORTERM=truecolor`: iTerm2, Windows Terminal, VS Code), los
colores son exactos; si no (Terminal de macOS), se aproximan a la paleta de 256 colores.
En `.esperado`: `rgb[x,y] = 0xRRGGBB`.

### Botones

Los 8 botones son un dispositivo de **entrada**: el programa los lee con
`lw $t0, 0x600($zero)` (bit *n* = 1 si el botón *n* está activado) y no puede
cambiarlos. En la interfaz se activan y desactivan con `bt n` (`bt` solo los apaga).

Como el programa no se entera cuando alguien presiona un botón, tiene que leerlos una
y otra vez en un ciclo: eso se llama **sondeo** (*polling*). Vea `programas/13_botones.asm`:
ponga `bp LEER`, active botones con `bt 3` y continúe con `c`. O más directo: ejecútelo en
**tiempo real** con `a 10` y presione las teclas `0`–`7` mientras corre.

### Interrupciones

Dos dispositivos pueden pedir una interrupción: los **botones** (cuando uno pasa de 0 a 1)
y el **temporizador** (cada N instrucciones, con N escrito en 0x604). Al final de cada
instrucción, si las interrupciones están habilitadas (IE = 1) y hay una pendiente, el
hardware hace:

1. `EPC = PC` (a dónde volver)
2. `IE = 0` (no anidar interrupciones)
3. `CAUSA` (0x60C) = qué dispositivo la pidió
4. `PC = VECTOR` (0x608): salta a la rutina de servicio (ISR)

La ISR atiende el dispositivo y termina con `eret` (`PC = EPC`, `IE = 1`). Instrucciones:
`ei` (habilitar), `di` (deshabilitar), `eret` y `mfepc rd` (leer EPC). El panel
**E/S: BOTONES E INTERRUPCIONES** muestra IE, las pendientes (`B` botones, `T` temporizador),
EPC y la cuenta del temporizador. Vea `programas/17_interrupciones.asm` y ejecútelo en
tiempo real (`a 20`) presionando las teclas `0`–`7`: compárelo con el sondeo de `13_botones`.

### Consola (syscall)

Como en MARS, `syscall` pide un servicio al sistema; el número va en `$v0`:

| `$v0` | Servicio |
|---|---|
| 1 | imprimir el entero de `$a0` |
| 4 | imprimir la cadena (`.asciiz`) cuya dirección está en `$a0` |
| 5 | leer un entero y dejarlo en `$v0` |
| 10 | terminar el programa |
| 11 | imprimir el carácter de `$a0` |
| 32 | pausar `$a0` milisegundos (en tiempo real) |

La salida aparece en la vista **CONSOLA** (`p`), que se muestra sola con la primera salida;
cuando el programa pide un número, se escribe en la línea de comandos. En `make test`, las
entradas se leen del archivo `programa.entrada` (un número por línea).

### Banderas

Como en el PDUA, las instrucciones del acumulador (`add`, `sub` y `addi` de 1 dirección) y
`cmp rs, rt` (que calcula rs − rt sin guardarlo) actualizan el registro de banderas:

| Bandera | Se enciende si… |
|---|---|
| Z | el resultado es cero |
| N | el resultado es negativo (bit 31 = 1) |
| C | hubo acarreo en una suma, o préstamo en una resta (sin signo) |
| V | hubo desbordamiento con signo (no detiene el procesador) |

Los saltos `jz`, `jn`, `jc` y `jv` saltan si la bandera correspondiente es 1.
En `.esperado` y en la interfaz se usan como `z = 1`.

### Pantalla

La pantalla no es parte de la RAM: es un dispositivo de salida conectado al bus. El
procesador la usa con `sb`/`sw` como si fuera memoria (**E/S mapeada en memoria**).

```
dirección del píxel (x, y) = 0x400 + 32*y + x          (x = 0..31, y = 0..15)
```

El byte es el color (4 bits menos significativos):

| 0 negro | 1 rojo | 2 verde | 3 amarillo | 4 azul | 5 magenta | 6 cian | 7 blanco |
|---|---|---|---|---|---|---|---|

8–15 son las mismas versiones brillantes. Ejemplo: `addi $t0, $zero, 9` y `sb $t0, 0x421($zero)`
pinta de rojo brillante el píxel (1, 1). La primera escritura en la pantalla cambia
la vista automáticamente; `p` alterna entre programa y pantalla.

En la interfaz, el panel derecho tiene dos ventanas de memoria:

- **MEMORIA**: una lupa libre sobre cualquier dirección (empieza en 0x200). Se mueve con
  `m dir`, cuando usted escribe a mano con `mem[dir] = valor`, y cuando el programa escribe
  en datos fuera de lo visible. Las escrituras del programa en la pila o en la pantalla no
  la mueven, porque esas se ven en su propio panel.
- **PILA**: muestra la zona alrededor de `$sp`. Dice "vacía" mientras `$sp` = 0x400.

El programa termina cuando el PC llega al final del segmento `.text`.

## Sintaxis del ensamblador

```asm
# comentario
.data
A:      .word 1, 2, 3, 4        # palabras de 32 bits
BUF:    .space 16               # 16 bytes en cero

.text
        addi $s0, $zero, A      # una etiqueta como inmediato es su dirección
        lw   $t0, 4($s0)        # base + desplazamiento
LOOP:   beq  $t0, $zero, FIN    # las etiquetas terminan en ':'
        addi $t0, $t0, -1
        j    LOOP
FIN:
```

- Algunas instrucciones aceptan formas de 2 y 1 dirección (definidas al final de `isa.txt`):
  `add $t1, $t2` es `$t1 = $t1 + $t2` (misma codificación que la de 3 direcciones), y
  `add $t1` es `ACC = ACC + $t1`: una instrucción distinta que usa el acumulador.
- Pseudoinstrucciones (si están en `isa.txt`): `move`, `li`, `la`, `nop`, `clear`, `b`, `not`.
  `li` con un valor de 32 bits se expande a `lui` + `ori`.
- Saltos de comparación `blt`, `bgt`, `ble`, `bge`: el ensamblador los expande a `slt` +
  `bne`/`beq` usando el registro **`$at`** (por eso está reservado). El segundo operando
  puede ser un número: `blt $t0, 32, CICLO`.
- Directivas de datos: `.word`, `.byte`, `.space n`, `.asciiz "texto\n"` (termina en 0),
  `.ascii`, `.align n`. `.globl` se acepta y se ignora (compatibilidad con MARS).
- Registros: `$zero`, `$t0`…`$t9`, `$s0`…`$s7`, `$a0`…`$a3`, `$v0`, `$v1`, `$sp`, `$ra`, … o por número (`$8`).
- Valores: `42`, `-7`, `0x2A`, `0b101010`, `'a'`.

## Archivo `.esperado`

```
$s0 = 20                # valor de un registro
acc = 21                # valor del acumulador
mem[0x230] = 104        # palabra en memoria
pc = 0x010              # valor final del PC
excepcion = si          # el programa debe terminar con una excepción
```
