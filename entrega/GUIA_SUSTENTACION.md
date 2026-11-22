# Guía de sustentación — `proyecto.asm`

Para poder explicar y modificar el programa sin haberlo escrito. Todo lo que dice aquí está comprobado
en el simulador (las trazas de la sección 4 las generó el propio ArquiSim; las modificaciones de la
sección 8 están ensambladas y probadas en `modificaciones/`).

**Cómo estudiar esta guía:** (1) lea las secciones 1 y 2 con `proyecto.asm` abierto al lado; (2) siga un
píxel a mano con la sección 4 y compruébelo en el simulador con un punto de parada; (3) apréndase la
hoja de datos de la sección 9; (4) haga usted mismo dos de las modificaciones de la sección 8 sin mirar.

---

## 0. El programa en 30 segundos

> "El programa lee los botones una vez. Si el botón 7 está apagado recorre la imagen de 16 × 16 con dos
> ciclos y por cada píxel dibuja dos cosas en la pantalla de 16 colores: el original a la izquierda y el
> filtrado a la derecha. Si está encendido recorre la pantalla RGB de 32 × 32, lee el píxel (x/2, y/2),
> lo filtra, lo convierte a 24 bits y lo manda al registro de color, que avanza solo. Los filtros están
> en una función que revisa los botones 0, 1 y 2 en orden. Son 105 instrucciones y 4 funciones."

## 1. Flujo general

```
                         ┌──────────────────────────────┐
                         │ $s7 = botones  (lw 0x600)    │   una sola vez
                         └──────────────┬───────────────┘
                                        │
                              ¿bit 7 de $s7 = 1?
                         no ┌───────────┴───────────┐ sí
                            ▼                       ▼
        PANTALLA DE 16 COLORES (0x400)        PANTALLA RGB (0x604 / 0x608)
        para y = 0..15                        posición = (0,0)   (sw 0x604, una vez)
          para x = 0..15                      para y = 0..31
            p = IMAGEN[16*y + x]                para x = 0..31
            ┌─────────────────────┐               p = IMAGEN[16*(y/2) + x/2]
            │ c = A_16COLORES(p)  │               ┌──────────────────────────┐
            └─────────────────────┘               │ f = APLICAR_FILTROS(p,b) │
            pantalla[32*y + x] = c                └──────────────────────────┘
            ┌──────────────────────────┐          ┌─────────────────┐
            │ f = APLICAR_FILTROS(p,b) │          │ c = A_RGB(f)    │
            └──────────────────────────┘          └─────────────────┘
            ┌─────────────────────┐               color = c        (sw 0x608: dibuja y avanza)
            │ c = A_16COLORES(f)  │
            └─────────────────────┘
            pantalla[32*y + x + 16] = c
                            │                       │
                            └───────────┬───────────┘
                                        ▼
                                  FIN (el PC llega al final de .text)


        APLICAR_FILTROS(p, botones):                      BRILLO(p):
          guardar $ra en la pila                            Y = (R + 2G + 2B + 2) >> 2
          r = p
          si botón 0:  r = r XOR 0xFF            (negativo)
          si botón 1:  Y = BRILLO(r)
                       r = (Y<<5)|(Y<<2)|(Y>>1)  (grises)
          si botón 2:  Y = BRILLO(r)
                       r = PALETAS[(botones AND 0x18) + Y]   (mapa de color)
          recuperar $ra
          devolver r
```

Quién llama a quién:

```
principal ──jal──► A_16COLORES              (hoja)
principal ──jal──► A_RGB                    (hoja)
principal ──jal──► APLICAR_FILTROS ──jal──► BRILLO (hoja)
                   └ guarda $ra en la pila porque ella misma usa jal
```

## 2. Recorrido de `proyecto.asm` bloque por bloque

Las direcciones son las reales del programa ensamblado (se ven en el panel PROGRAMA de ArquiSim).

### 2.1 Datos (`.data`)

| Dirección | Etiqueta | Qué hay |
|---|---|---|
| `0x200-0x2FF` | `IMAGEN` | 256 bytes: el píxel (x, y) está en `IMAGEN + 16*y + x` |
| `0x300-0x31F` | `PALETAS` | 4 paletas de 8 bytes para el filtro 3 |

### 2.2 Inicio (`0x000-0x008`, 3 instrucciones)

| Dir. | Instrucción | Qué hace | Por qué |
|---|---|---|---|
| `0x000` | `lw $s7, 0x600($zero)` | `$s7` = palabra de los botones | Se lee una sola vez (lo exige el enunciado). Va en un `$s` porque se necesita durante todo el programa y las funciones no tocan los `$s`. La dirección es `$zero + 0x600`: no hace falta registro base. |
| `0x004` | `andi $t0, $s7, 0x80` | deja solo el bit 7 | `0x80 = 1000 0000`. El resultado es 0 o `0x80`. |
| `0x008` | `bne $t0, $zero, RGB` | si no es 0, salta a `RGB` | bit 7 = 1 → pantalla RGB. Si es 0 sigue derecho a la pantalla de 16 colores. |

### 2.3 Ciclo de la pantalla de 16 colores (`0x00C-0x064`, 23 instrucciones)

| Dir. | Instrucción | Qué hace | Por qué |
|---|---|---|---|
| `0x00C` | `li $s0, 0` | y = 0 | Contador de filas. (`li` es `addi $s0, $zero, 0`.) |
| `0x010` | `C_FILA: li $s1, 0` | x = 0 | Se repite al empezar cada fila. |
| `0x014` | `C_COL: sll $t0, $s0, 4` | `$t0` = 16·y | Cada fila de la imagen mide 16 bytes; correr 4 = multiplicar por 16. |
| `0x018` | `add $t0, $t0, $s1` | `$t0` = 16·y + x | Desplazamiento del píxel dentro de la imagen. |
| `0x01C` | `lbu $s2, IMAGEN($t0)` | `$s2` = píxel | Dirección = `0x200 + $t0`. `lbu` porque el píxel no tiene signo. En `$s2` porque se usa dos veces y hay llamadas en medio. |
| `0x020` | `sll $s3, $s0, 5` | `$s3` = 32·y | Cada fila de la **pantalla** mide 32 bytes; correr 5 = multiplicar por 32. |
| `0x024` | `add $s3, $s3, $s1` | `$s3` = 32·y + x | Desplazamiento del píxel dentro de la pantalla. En `$s3` porque se usa después de las llamadas. |
| `0x028` | `move $a0, $s2` | argumento = píxel original | Convención: el argumento va en `$a0`. |
| `0x02C` | `jal A_16COLORES` | llama; `$ra = 0x030` | |
| `0x030` | `sb $v0, 0x400($s3)` | pinta la mitad izquierda | Dirección = `0x400 + 32·y + x`. `sb` porque un píxel es 1 byte. |
| `0x034` | `move $a0, $s2` | argumento 1 = píxel original | Otra vez desde `$s2`: `$a0` no se asume intacto después de una llamada. |
| `0x038` | `move $a1, $s7` | argumento 2 = botones | |
| `0x03C` | `jal APLICAR_FILTROS` | llama; `$ra = 0x040` | |
| `0x040` | `move $a0, $v0` | argumento = píxel filtrado | La salida de una función es la entrada de la otra. |
| `0x044` | `jal A_16COLORES` | llama; `$ra = 0x048` | |
| `0x048` | `sb $v0, 0x410($s3)` | pinta la mitad derecha | `0x410 = 0x400 + 16`: la misma fila, 16 columnas a la derecha. |
| `0x04C` | `addi $s1, $s1, 1` | x++ | |
| `0x050` | `slti $t0, $s1, 16` | `$t0` = 1 si x < 16 | |
| `0x054` | `bne $t0, $zero, C_COL` | repite si x < 16 | |
| `0x058` | `addi $s0, $s0, 1` | y++ | |
| `0x05C` | `slti $t0, $s0, 16` | `$t0` = 1 si y < 16 | |
| `0x060` | `bne $t0, $zero, C_FILA` | repite si y < 16 | Vuelve a `C_FILA`, que pone x = 0. |
| `0x064` | `j FIN` | salta al final | Para no "caerse" dentro del ciclo RGB ni de las funciones. |

### 2.4 Ciclo de la pantalla RGB (`0x068-0x0B4`, 20 instrucciones)

Aquí `$s0` y `$s1` son coordenadas de **pantalla** (0 a 31), no de imagen.

| Dir. | Instrucción | Qué hace | Por qué |
|---|---|---|---|
| `0x068` | `RGB: sw $zero, 0x604($zero)` | posición = (0, 0) | (y << 16) \| x con x = y = 0 es 0. Una sola vez: después el dispositivo avanza solo. |
| `0x06C` | `li $s0, 0` | y = 0 | |
| `0x070` | `R_FILA: li $s1, 0` | x = 0 | |
| `0x074` | `R_COL: srl $t0, $s0, 1` | `$t0` = y/2 | Fila de la imagen que corresponde a esta fila de pantalla. |
| `0x078` | `sll $t0, $t0, 4` | `$t0` = 16·(y/2) | No se puede simplificar a `sll 3`: primero hay que botar el bit bajo de y. |
| `0x07C` | `srl $t1, $s1, 1` | `$t1` = x/2 | Columna de la imagen. |
| `0x080` | `add $t0, $t0, $t1` | `$t0` = 16·(y/2) + x/2 | |
| `0x084` | `lbu $a0, IMAGEN($t0)` | argumento 1 = píxel | Aquí se carga directo en `$a0`: solo se usa una vez. |
| `0x088` | `move $a1, $s7` | argumento 2 = botones | |
| `0x08C` | `jal APLICAR_FILTROS` | llama; `$ra = 0x090` | |
| `0x090` | `move $a0, $v0` | argumento = píxel filtrado | |
| `0x094` | `jal A_RGB` | llama; `$ra = 0x098` | |
| `0x098` | `sw $v0, 0x608($zero)` | dibuja el píxel | `sw` porque el color es una palabra de 24 bits. El dispositivo avanza x, y al final de la fila pasa a la siguiente. |
| `0x09C` | `addi $s1, $s1, 1` | x++ | Solo para saber qué píxel leer y cuándo parar. |
| `0x0A0` | `slti $t0, $s1, 32` | x < 32 | |
| `0x0A4` | `bne $t0, $zero, R_COL` | repite | |
| `0x0A8` | `addi $s0, $s0, 1` | y++ | |
| `0x0AC` | `slti $t0, $s0, 32` | y < 32 | |
| `0x0B0` | `bne $t0, $zero, R_FILA` | repite | |
| `0x0B4` | `j FIN` | termina | |

### 2.5 `APLICAR_FILTROS` (`0x0B8-0x118`, 25 instrucciones)

Entrada: `$a0` = píxel, `$a1` = botones. Salida: `$v0`. El resultado se va acumulando en `$v0`.

| Dir. | Instrucción | Qué hace | Por qué |
|---|---|---|---|
| `0x0B8` | `addi $sp, $sp, -4` | reserva una palabra | *Push*: la pila crece hacia abajo; `$sp` pasa de `0x400` a `0x3FC`. |
| `0x0BC` | `sw $ra, 0($sp)` | guarda `$ra` en `0x3FC` | Más abajo hay `jal BRILLO`, que sobrescribe `$ra`. Sin esto no podría volver al programa principal. |
| `0x0C0` | `move $v0, $a0` | resultado = píxel | Si no hay ningún filtro activo, devuelve el píxel igual (por eso con `0x00` las dos mitades son iguales). |
| `0x0C4` | `andi $t0, $a1, 0x01` | aísla el botón 0 | |
| `0x0C8` | `beq $t0, $zero, F_GRIS` | apagado → salta la `xori` | |
| `0x0CC` | `xori $v0, $v0, 0xFF` | **negativo** | Invierte los 8 bits (sección 5.1). |
| `0x0D0` | `F_GRIS: andi $t0, $a1, 0x02` | aísla el botón 1 | |
| `0x0D4` | `beq $t0, $zero, F_MAPA` | apagado → siguiente filtro | |
| `0x0D8` | `move $a0, $v0` | argumento = píxel hasta ahora | Si el negativo estaba activo, ya viene invertido: por eso se combinan en orden. |
| `0x0DC` | `jal BRILLO` | `$v0` = Y; `$ra = 0x0E0` | |
| `0x0E0` | `sll $t0, $v0, 5` | Y en los bits 7-5 | R' = Y |
| `0x0E4` | `sll $t1, $v0, 2` | Y en los bits 4-2 | G' = Y |
| `0x0E8` | `or $t0, $t0, $t1` | junta R' y G' | |
| `0x0EC` | `srl $t1, $v0, 1` | Y/2 en los bits 1-0 | B' = Y >> 1 (B solo tiene 2 bits) |
| `0x0F0` | `or $v0, $t0, $t1` | **píxel gris** | |
| `0x0F4` | `F_MAPA: andi $t0, $a1, 0x04` | aísla el botón 2 | |
| `0x0F8` | `beq $t0, $zero, F_FIN` | apagado → terminar | |
| `0x0FC` | `move $a0, $v0` | argumento = píxel hasta ahora | |
| `0x100` | `jal BRILLO` | `$v0` = Y; `$ra = 0x104` | |
| `0x104` | `andi $t0, $a1, 0x18` | botones 3 y 4 | Vale 0, 8, 16 o 24 = 8 × número de paleta = dónde empieza esa paleta. |
| `0x108` | `add $t0, $t0, $v0` | índice = 8·paleta + Y | |
| `0x10C` | `lbu $v0, PALETAS($t0)` | **píxel = PALETAS[índice]** | Dirección = `0x300 + índice`. |
| `0x110` | `F_FIN: lw $ra, 0($sp)` | recupera `$ra` | *Pop*. |
| `0x114` | `addi $sp, $sp, 4` | libera la palabra | `$sp` vuelve a `0x400`. |
| `0x118` | `jr $ra` | vuelve | A `0x040` (16 colores) o `0x090` (RGB). |

### 2.6 `BRILLO` (`0x11C-0x148`, 12 instrucciones)

Y = (R + 2G + 2B + 2) >> 2. Solo usa `$t0`, `$t1` y `$v0`.

| Dir. | Instrucción | Qué hace |
|---|---|---|
| `0x11C` | `srl $t0, $a0, 5` | baja R a los bits 2-0 |
| `0x120` | `andi $t0, $t0, 0x07` | `$t0` = R |
| `0x124` | `srl $t1, $a0, 2` | baja G a los bits 2-0 (arriba queda R) |
| `0x128` | `andi $t1, $t1, 0x07` | `$t1` = G (se bota R) |
| `0x12C` | `sll $t1, $t1, 1` | `$t1` = 2G |
| `0x130` | `add $t0, $t0, $t1` | R + 2G |
| `0x134` | `andi $t1, $a0, 0x03` | `$t1` = B (ya está abajo) |
| `0x138` | `sll $t1, $t1, 1` | `$t1` = 2B |
| `0x13C` | `add $t0, $t0, $t1` | R + 2G + 2B |
| `0x140` | `addi $t0, $t0, 2` | + 2 (redondeo) |
| `0x144` | `srl $v0, $t0, 2` | Y = suma / 4 |
| `0x148` | `jr $ra` | vuelve |

### 2.7 `A_16COLORES` (`0x14C-0x17C`, 13 instrucciones)

Para cada bit: correr hasta su posición final, aislar con `andi`, juntar con `or`.

| Dir. | Instrucción | Qué hace |
|---|---|---|
| `0x14C` | `srl $v0, $a0, 7` | bit 7 → bit 0 |
| `0x150` | `andi $v0, $v0, 0x01` | `$v0` = rojo |
| `0x154` | `srl $t0, $a0, 3` | bit 4 → bit 1 |
| `0x158` | `andi $t0, $t0, 0x02` | verde |
| `0x15C` | `or $v0, $v0, $t0` | junta |
| `0x160` | `sll $t0, $a0, 1` | bit 1 → bit 2 |
| `0x164` | `andi $t0, $t0, 0x04` | azul |
| `0x168` | `or $v0, $v0, $t0` | junta |
| `0x16C` | `srl $t0, $a0, 3` | bit 6 → bit 3 |
| `0x170` | `or $t0, $t0, $a0` | OR con el píxel sin correr (su bit 3 ya está en el bit 3) |
| `0x174` | `andi $t0, $t0, 0x08` | brillo = bit 6 OR bit 3 |
| `0x178` | `or $v0, $v0, $t0` | junta |
| `0x17C` | `jr $ra` | vuelve |

### 2.8 `A_RGB` (`0x180-0x1A0`, 9 instrucciones)

| Dir. | Instrucción | Qué hace |
|---|---|---|
| `0x180` | `andi $t0, $a0, 0xE0` | R donde está (bits 7-5) |
| `0x184` | `sll $v0, $t0, 16` | R a los bits 23-21 |
| `0x188` | `andi $t0, $a0, 0x1C` | G donde está (bits 4-2) |
| `0x18C` | `sll $t0, $t0, 11` | G a los bits 15-13 |
| `0x190` | `or $v0, $v0, $t0` | junta |
| `0x194` | `andi $t0, $a0, 0x03` | B donde está (bits 1-0) |
| `0x198` | `sll $t0, $t0, 6` | B a los bits 7-6 |
| `0x19C` | `or $v0, $v0, $t0` | junta |
| `0x1A0` | `jr $ra` | vuelve |

`FIN` queda en `0x1A4`: no hay instrucción ahí; el simulador termina cuando el PC llega al final de `.text`.

## 3. Registros: quién usa qué

| Registro | Dueño | Contenido |
|---|---|---|
| `$s0` | principal | y (fila) |
| `$s1` | principal | x (columna) |
| `$s2` | principal | píxel original (solo en 16 colores) |
| `$s3` | principal | 32·y + x (solo en 16 colores) |
| `$s7` | principal | botones |
| `$a0` | quien llama | argumento: píxel |
| `$a1` | quien llama | argumento: botones (solo para `APLICAR_FILTROS`) |
| `$v0` | función | resultado |
| `$t0`, `$t1` | cualquiera | temporales; nunca se espera que sobrevivan a un `jal` |
| `$ra` | `jal` | dirección de retorno |
| `$sp` | `APLICAR_FILTROS` | `0x400` normalmente, `0x3FC` mientras corre esa función |

Regla que se cumple en todo el programa: **ninguna función escribe en un `$s`** y **el programa principal
nunca usa un `$t` cuyo valor venga de antes de un `jal`**.

## 4. Traza de los 4 píxeles de la Tabla 4, función por función

Generadas por ArquiSim (`python herramientas/trazas.py`): cada fila es una instrucción y cada columna el
valor que queda en el registro "Destino" para ese píxel. Para repetirlo en el simulador: `bp A_16COLORES`
(o la función que sea), `c`, y avanzar con Enter mirando el registro.

Notas para leerlas:

- Las direcciones de retorno (`$ra`, "vuelve a") son las del programa de prueba mínimo que usa
  `trazas.py`, no las de `proyecto.asm`. En el programa real son las de la sección 2.
- Todos los píxeles siguen el mismo camino: en el programa **ningún salto depende del valor del píxel**,
  solo de los botones.
- Resultado esperado (Tabla 4): 16 colores 15 / 9 / 0 / 3; RGB `E0E0C0` / `E00000` / `000000` / `A08040`;
  negativo `00` / `1F` / `FF` / `4E` (16 colores 0 / 14 / 15 / 12); grises `FF` / `49` / `00` / `92`
  (16 colores 15 / 8 / 0 / 7).

#### A_16COLORES con el píxel original

| # | Instrucción | Destino | Fondo (0,0)<br>`$a0 = 0xFF` | Rojo (5,1)<br>`$a0 = 0xE0` | Letra (3,11)<br>`$a0 = 0x00` | Sopa (9,10)<br>`$a0 = 0xB1` |
|---|---|---|---|---|---|---|
| 1 | `srl $v0, $a0, 7` | $v0 | `0x01` = 0000 0001 | `0x01` = 0000 0001 | `0x00` = 0000 0000 | `0x01` = 0000 0001 |
| 2 | `andi $v0, $v0, 0x01` | $v0 | `0x01` = 0000 0001 | `0x01` = 0000 0001 | `0x00` = 0000 0000 | `0x01` = 0000 0001 |
| 3 | `srl $t0, $a0, 3` | $t0 | `0x1F` = 0001 1111 | `0x1C` = 0001 1100 | `0x00` = 0000 0000 | `0x16` = 0001 0110 |
| 4 | `andi $t0, $t0, 0x02` | $t0 | `0x02` = 0000 0010 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x02` = 0000 0010 |
| 5 | `or $v0, $v0, $t0` | $v0 | `0x03` = 0000 0011 | `0x01` = 0000 0001 | `0x00` = 0000 0000 | `0x03` = 0000 0011 |
| 6 | `sll $t0, $a0, 1` | $t0 | `0x1FE` | `0x1C0` | `0x00` = 0000 0000 | `0x162` |
| 7 | `andi $t0, $t0, 0x04` | $t0 | `0x04` = 0000 0100 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 8 | `or $v0, $v0, $t0` | $v0 | `0x07` = 0000 0111 | `0x01` = 0000 0001 | `0x00` = 0000 0000 | `0x03` = 0000 0011 |
| 9 | `srl $t0, $a0, 3` | $t0 | `0x1F` = 0001 1111 | `0x1C` = 0001 1100 | `0x00` = 0000 0000 | `0x16` = 0001 0110 |
| 10 | `or $t0, $t0, $a0` | $t0 | `0xFF` = 1111 1111 | `0xFC` = 1111 1100 | `0x00` = 0000 0000 | `0xB7` = 1011 0111 |
| 11 | `andi $t0, $t0, 0x08` | $t0 | `0x08` = 0000 1000 | `0x08` = 0000 1000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 12 | `or $v0, $v0, $t0` | $v0 | `0x0F` = 0000 1111 | `0x09` = 0000 1001 | `0x00` = 0000 0000 | `0x03` = 0000 0011 |
| 13 | `jr $ra` | PC | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) |

#### A_16COLORES con el píxel en negativo

| # | Instrucción | Destino | Fondo (0,0)<br>`$a0 = 0x00` | Rojo (5,1)<br>`$a0 = 0x1F` | Letra (3,11)<br>`$a0 = 0xFF` | Sopa (9,10)<br>`$a0 = 0x4E` |
|---|---|---|---|---|---|---|
| 1 | `srl $v0, $a0, 7` | $v0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x01` = 0000 0001 | `0x00` = 0000 0000 |
| 2 | `andi $v0, $v0, 0x01` | $v0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x01` = 0000 0001 | `0x00` = 0000 0000 |
| 3 | `srl $t0, $a0, 3` | $t0 | `0x00` = 0000 0000 | `0x03` = 0000 0011 | `0x1F` = 0001 1111 | `0x09` = 0000 1001 |
| 4 | `andi $t0, $t0, 0x02` | $t0 | `0x00` = 0000 0000 | `0x02` = 0000 0010 | `0x02` = 0000 0010 | `0x00` = 0000 0000 |
| 5 | `or $v0, $v0, $t0` | $v0 | `0x00` = 0000 0000 | `0x02` = 0000 0010 | `0x03` = 0000 0011 | `0x00` = 0000 0000 |
| 6 | `sll $t0, $a0, 1` | $t0 | `0x00` = 0000 0000 | `0x3E` = 0011 1110 | `0x1FE` | `0x9C` = 1001 1100 |
| 7 | `andi $t0, $t0, 0x04` | $t0 | `0x00` = 0000 0000 | `0x04` = 0000 0100 | `0x04` = 0000 0100 | `0x04` = 0000 0100 |
| 8 | `or $v0, $v0, $t0` | $v0 | `0x00` = 0000 0000 | `0x06` = 0000 0110 | `0x07` = 0000 0111 | `0x04` = 0000 0100 |
| 9 | `srl $t0, $a0, 3` | $t0 | `0x00` = 0000 0000 | `0x03` = 0000 0011 | `0x1F` = 0001 1111 | `0x09` = 0000 1001 |
| 10 | `or $t0, $t0, $a0` | $t0 | `0x00` = 0000 0000 | `0x1F` = 0001 1111 | `0xFF` = 1111 1111 | `0x4F` = 0100 1111 |
| 11 | `andi $t0, $t0, 0x08` | $t0 | `0x00` = 0000 0000 | `0x08` = 0000 1000 | `0x08` = 0000 1000 | `0x08` = 0000 1000 |
| 12 | `or $v0, $v0, $t0` | $v0 | `0x00` = 0000 0000 | `0x0E` = 0000 1110 | `0x0F` = 0000 1111 | `0x0C` = 0000 1100 |
| 13 | `jr $ra` | PC | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) |

#### A_16COLORES con el píxel en grises

| # | Instrucción | Destino | Fondo (0,0)<br>`$a0 = 0xFF` | Rojo (5,1)<br>`$a0 = 0x49` | Letra (3,11)<br>`$a0 = 0x00` | Sopa (9,10)<br>`$a0 = 0x92` |
|---|---|---|---|---|---|---|
| 1 | `srl $v0, $a0, 7` | $v0 | `0x01` = 0000 0001 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x01` = 0000 0001 |
| 2 | `andi $v0, $v0, 0x01` | $v0 | `0x01` = 0000 0001 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x01` = 0000 0001 |
| 3 | `srl $t0, $a0, 3` | $t0 | `0x1F` = 0001 1111 | `0x09` = 0000 1001 | `0x00` = 0000 0000 | `0x12` = 0001 0010 |
| 4 | `andi $t0, $t0, 0x02` | $t0 | `0x02` = 0000 0010 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x02` = 0000 0010 |
| 5 | `or $v0, $v0, $t0` | $v0 | `0x03` = 0000 0011 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x03` = 0000 0011 |
| 6 | `sll $t0, $a0, 1` | $t0 | `0x1FE` | `0x92` = 1001 0010 | `0x00` = 0000 0000 | `0x124` |
| 7 | `andi $t0, $t0, 0x04` | $t0 | `0x04` = 0000 0100 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x04` = 0000 0100 |
| 8 | `or $v0, $v0, $t0` | $v0 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x07` = 0000 0111 |
| 9 | `srl $t0, $a0, 3` | $t0 | `0x1F` = 0001 1111 | `0x09` = 0000 1001 | `0x00` = 0000 0000 | `0x12` = 0001 0010 |
| 10 | `or $t0, $t0, $a0` | $t0 | `0xFF` = 1111 1111 | `0x49` = 0100 1001 | `0x00` = 0000 0000 | `0x92` = 1001 0010 |
| 11 | `andi $t0, $t0, 0x08` | $t0 | `0x08` = 0000 1000 | `0x08` = 0000 1000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 12 | `or $v0, $v0, $t0` | $v0 | `0x0F` = 0000 1111 | `0x08` = 0000 1000 | `0x00` = 0000 0000 | `0x07` = 0000 0111 |
| 13 | `jr $ra` | PC | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) |

#### A_RGB con el píxel original

| # | Instrucción | Destino | Fondo (0,0)<br>`$a0 = 0xFF` | Rojo (5,1)<br>`$a0 = 0xE0` | Letra (3,11)<br>`$a0 = 0x00` | Sopa (9,10)<br>`$a0 = 0xB1` |
|---|---|---|---|---|---|---|
| 1 | `andi $t0, $a0, 0xE0` | $t0 | `0xE0` = 1110 0000 | `0xE0` = 1110 0000 | `0x00` = 0000 0000 | `0xA0` = 1010 0000 |
| 2 | `sll $v0, $t0, 16` | $v0 | `0xE00000` | `0xE00000` | `0x00` = 0000 0000 | `0xA00000` |
| 3 | `andi $t0, $a0, 0x1C` | $t0 | `0x1C` = 0001 1100 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x10` = 0001 0000 |
| 4 | `sll $t0, $t0, 11` | $t0 | `0x00E000` | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x008000` |
| 5 | `or $v0, $v0, $t0` | $v0 | `0xE0E000` | `0xE00000` | `0x00` = 0000 0000 | `0xA08000` |
| 6 | `andi $t0, $a0, 0x03` | $t0 | `0x03` = 0000 0011 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x01` = 0000 0001 |
| 7 | `sll $t0, $t0, 6` | $t0 | `0xC0` = 1100 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x40` = 0100 0000 |
| 8 | `or $v0, $v0, $t0` | $v0 | `0xE0E0C0` | `0xE00000` | `0x00` = 0000 0000 | `0xA08040` |
| 9 | `jr $ra` | PC | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) |

#### BRILLO con el píxel original

| # | Instrucción | Destino | Fondo (0,0)<br>`$a0 = 0xFF` | Rojo (5,1)<br>`$a0 = 0xE0` | Letra (3,11)<br>`$a0 = 0x00` | Sopa (9,10)<br>`$a0 = 0xB1` |
|---|---|---|---|---|---|---|
| 1 | `srl $t0, $a0, 5` | $t0 | `0x07` = 0000 0111 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x05` = 0000 0101 |
| 2 | `andi $t0, $t0, 0x07` | $t0 | `0x07` = 0000 0111 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x05` = 0000 0101 |
| 3 | `srl $t1, $a0, 2` | $t1 | `0x3F` = 0011 1111 | `0x38` = 0011 1000 | `0x00` = 0000 0000 | `0x2C` = 0010 1100 |
| 4 | `andi $t1, $t1, 0x07` | $t1 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x04` = 0000 0100 |
| 5 | `sll $t1, $t1, 1` | $t1 | `0x0E` = 0000 1110 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x08` = 0000 1000 |
| 6 | `add $t0, $t0, $t1` | $t0 | `0x15` = 0001 0101 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x0D` = 0000 1101 |
| 7 | `andi $t1, $a0, 0x03` | $t1 | `0x03` = 0000 0011 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x01` = 0000 0001 |
| 8 | `sll $t1, $t1, 1` | $t1 | `0x06` = 0000 0110 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x02` = 0000 0010 |
| 9 | `add $t0, $t0, $t1` | $t0 | `0x1B` = 0001 1011 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x0F` = 0000 1111 |
| 10 | `addi $t0, $t0, 2` | $t0 | `0x1D` = 0001 1101 | `0x09` = 0000 1001 | `0x02` = 0000 0010 | `0x11` = 0001 0001 |
| 11 | `srl $v0, $t0, 2` | $v0 | `0x07` = 0000 0111 | `0x02` = 0000 0010 | `0x00` = 0000 0000 | `0x04` = 0000 0100 |
| 12 | `jr $ra` | PC | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) |

#### APLICAR_FILTROS con botones = 0x00 (sin filtros)

| # | Instrucción | Destino | Fondo (0,0)<br>`$a0 = 0xFF` | Rojo (5,1)<br>`$a0 = 0xE0` | Letra (3,11)<br>`$a0 = 0x00` | Sopa (9,10)<br>`$a0 = 0xB1` |
|---|---|---|---|---|---|---|
| 1 | `addi $sp, $sp, -4` | $sp | `0x3FC` | `0x3FC` | `0x3FC` | `0x3FC` |
| 2 | `sw $ra, 0($sp)` | MEM[0x3FC] | `0x00C` | `0x00C` | `0x00C` | `0x00C` |
| 3 | `move $v0, $a0` | $v0 | `0xFF` = 1111 1111 | `0xE0` = 1110 0000 | `0x00` = 0000 0000 | `0xB1` = 1011 0001 |
| 4 | `andi $t0, $a1, 0x01` | $t0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 5 | `beq $t0, $zero, F_GRIS` |  | **salta** | **salta** | **salta** | **salta** |
| 6 | `andi $t0, $a1, 0x02` | $t0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 7 | `beq $t0, $zero, F_MAPA` |  | **salta** | **salta** | **salta** | **salta** |
| 8 | `andi $t0, $a1, 0x04` | $t0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 9 | `beq $t0, $zero, F_FIN` |  | **salta** | **salta** | **salta** | **salta** |
| 10 | `lw $ra, 0($sp)` | $ra | `0x00C` | `0x00C` | `0x00C` | `0x00C` |
| 11 | `addi $sp, $sp, 4` | $sp | `0x400` | `0x400` | `0x400` | `0x400` |
| 12 | `jr $ra` | PC | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) |

#### APLICAR_FILTROS con botones = 0x01 (negativo)

| # | Instrucción | Destino | Fondo (0,0)<br>`$a0 = 0xFF` | Rojo (5,1)<br>`$a0 = 0xE0` | Letra (3,11)<br>`$a0 = 0x00` | Sopa (9,10)<br>`$a0 = 0xB1` |
|---|---|---|---|---|---|---|
| 1 | `addi $sp, $sp, -4` | $sp | `0x3FC` | `0x3FC` | `0x3FC` | `0x3FC` |
| 2 | `sw $ra, 0($sp)` | MEM[0x3FC] | `0x00C` | `0x00C` | `0x00C` | `0x00C` |
| 3 | `move $v0, $a0` | $v0 | `0xFF` = 1111 1111 | `0xE0` = 1110 0000 | `0x00` = 0000 0000 | `0xB1` = 1011 0001 |
| 4 | `andi $t0, $a1, 0x01` | $t0 | `0x01` = 0000 0001 | `0x01` = 0000 0001 | `0x01` = 0000 0001 | `0x01` = 0000 0001 |
| 5 | `beq $t0, $zero, F_GRIS` |  | no salta | no salta | no salta | no salta |
| 6 | `xori $v0, $v0, 0xFF` | $v0 | `0x00` = 0000 0000 | `0x1F` = 0001 1111 | `0xFF` = 1111 1111 | `0x4E` = 0100 1110 |
| 7 | `andi $t0, $a1, 0x02` | $t0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 8 | `beq $t0, $zero, F_MAPA` |  | **salta** | **salta** | **salta** | **salta** |
| 9 | `andi $t0, $a1, 0x04` | $t0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 10 | `beq $t0, $zero, F_FIN` |  | **salta** | **salta** | **salta** | **salta** |
| 11 | `lw $ra, 0($sp)` | $ra | `0x00C` | `0x00C` | `0x00C` | `0x00C` |
| 12 | `addi $sp, $sp, 4` | $sp | `0x400` | `0x400` | `0x400` | `0x400` |
| 13 | `jr $ra` | PC | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) |

#### APLICAR_FILTROS con botones = 0x02 (grises)

| # | Instrucción | Destino | Fondo (0,0)<br>`$a0 = 0xFF` | Rojo (5,1)<br>`$a0 = 0xE0` | Letra (3,11)<br>`$a0 = 0x00` | Sopa (9,10)<br>`$a0 = 0xB1` |
|---|---|---|---|---|---|---|
| 1 | `addi $sp, $sp, -4` | $sp | `0x3FC` | `0x3FC` | `0x3FC` | `0x3FC` |
| 2 | `sw $ra, 0($sp)` | MEM[0x3FC] | `0x00C` | `0x00C` | `0x00C` | `0x00C` |
| 3 | `move $v0, $a0` | $v0 | `0xFF` = 1111 1111 | `0xE0` = 1110 0000 | `0x00` = 0000 0000 | `0xB1` = 1011 0001 |
| 4 | `andi $t0, $a1, 0x01` | $t0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 5 | `beq $t0, $zero, F_GRIS` |  | **salta** | **salta** | **salta** | **salta** |
| 6 | `andi $t0, $a1, 0x02` | $t0 | `0x02` = 0000 0010 | `0x02` = 0000 0010 | `0x02` = 0000 0010 | `0x02` = 0000 0010 |
| 7 | `beq $t0, $zero, F_MAPA` |  | no salta | no salta | no salta | no salta |
| 8 | `move $a0, $v0` | $a0 | `0xFF` = 1111 1111 | `0xE0` = 1110 0000 | `0x00` = 0000 0000 | `0xB1` = 1011 0001 |
| 9 | `jal BRILLO` | $ra | `0x038` | `0x038` | `0x038` | `0x038` |
| 10 | `srl $t0, $a0, 5` | $t0 | `0x07` = 0000 0111 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x05` = 0000 0101 |
| 11 | `andi $t0, $t0, 0x07` | $t0 | `0x07` = 0000 0111 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x05` = 0000 0101 |
| 12 | `srl $t1, $a0, 2` | $t1 | `0x3F` = 0011 1111 | `0x38` = 0011 1000 | `0x00` = 0000 0000 | `0x2C` = 0010 1100 |
| 13 | `andi $t1, $t1, 0x07` | $t1 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x04` = 0000 0100 |
| 14 | `sll $t1, $t1, 1` | $t1 | `0x0E` = 0000 1110 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x08` = 0000 1000 |
| 15 | `add $t0, $t0, $t1` | $t0 | `0x15` = 0001 0101 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x0D` = 0000 1101 |
| 16 | `andi $t1, $a0, 0x03` | $t1 | `0x03` = 0000 0011 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x01` = 0000 0001 |
| 17 | `sll $t1, $t1, 1` | $t1 | `0x06` = 0000 0110 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x02` = 0000 0010 |
| 18 | `add $t0, $t0, $t1` | $t0 | `0x1B` = 0001 1011 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x0F` = 0000 1111 |
| 19 | `addi $t0, $t0, 2` | $t0 | `0x1D` = 0001 1101 | `0x09` = 0000 1001 | `0x02` = 0000 0010 | `0x11` = 0001 0001 |
| 20 | `srl $v0, $t0, 2` | $v0 | `0x07` = 0000 0111 | `0x02` = 0000 0010 | `0x00` = 0000 0000 | `0x04` = 0000 0100 |
| 21 | `jr $ra` | PC | `0x038` (vuelve) | `0x038` (vuelve) | `0x038` (vuelve) | `0x038` (vuelve) |
| 22 | `sll $t0, $v0, 5` | $t0 | `0xE0` = 1110 0000 | `0x40` = 0100 0000 | `0x00` = 0000 0000 | `0x80` = 1000 0000 |
| 23 | `sll $t1, $v0, 2` | $t1 | `0x1C` = 0001 1100 | `0x08` = 0000 1000 | `0x00` = 0000 0000 | `0x10` = 0001 0000 |
| 24 | `or $t0, $t0, $t1` | $t0 | `0xFC` = 1111 1100 | `0x48` = 0100 1000 | `0x00` = 0000 0000 | `0x90` = 1001 0000 |
| 25 | `srl $t1, $v0, 1` | $t1 | `0x03` = 0000 0011 | `0x01` = 0000 0001 | `0x00` = 0000 0000 | `0x02` = 0000 0010 |
| 26 | `or $v0, $t0, $t1` | $v0 | `0xFF` = 1111 1111 | `0x49` = 0100 1001 | `0x00` = 0000 0000 | `0x92` = 1001 0010 |
| 27 | `andi $t0, $a1, 0x04` | $t0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 28 | `beq $t0, $zero, F_FIN` |  | **salta** | **salta** | **salta** | **salta** |
| 29 | `lw $ra, 0($sp)` | $ra | `0x00C` | `0x00C` | `0x00C` | `0x00C` |
| 30 | `addi $sp, $sp, 4` | $sp | `0x400` | `0x400` | `0x400` | `0x400` |
| 31 | `jr $ra` | PC | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) |

#### APLICAR_FILTROS con botones = 0x04 (mapa de color, paleta térmica)

| # | Instrucción | Destino | Fondo (0,0)<br>`$a0 = 0xFF` | Rojo (5,1)<br>`$a0 = 0xE0` | Letra (3,11)<br>`$a0 = 0x00` | Sopa (9,10)<br>`$a0 = 0xB1` |
|---|---|---|---|---|---|---|
| 1 | `addi $sp, $sp, -4` | $sp | `0x3FC` | `0x3FC` | `0x3FC` | `0x3FC` |
| 2 | `sw $ra, 0($sp)` | MEM[0x3FC] | `0x00C` | `0x00C` | `0x00C` | `0x00C` |
| 3 | `move $v0, $a0` | $v0 | `0xFF` = 1111 1111 | `0xE0` = 1110 0000 | `0x00` = 0000 0000 | `0xB1` = 1011 0001 |
| 4 | `andi $t0, $a1, 0x01` | $t0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 5 | `beq $t0, $zero, F_GRIS` |  | **salta** | **salta** | **salta** | **salta** |
| 6 | `andi $t0, $a1, 0x02` | $t0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 7 | `beq $t0, $zero, F_MAPA` |  | **salta** | **salta** | **salta** | **salta** |
| 8 | `andi $t0, $a1, 0x04` | $t0 | `0x04` = 0000 0100 | `0x04` = 0000 0100 | `0x04` = 0000 0100 | `0x04` = 0000 0100 |
| 9 | `beq $t0, $zero, F_FIN` |  | no salta | no salta | no salta | no salta |
| 10 | `move $a0, $v0` | $a0 | `0xFF` = 1111 1111 | `0xE0` = 1110 0000 | `0x00` = 0000 0000 | `0xB1` = 1011 0001 |
| 11 | `jal BRILLO` | $ra | `0x05C` | `0x05C` | `0x05C` | `0x05C` |
| 12 | `srl $t0, $a0, 5` | $t0 | `0x07` = 0000 0111 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x05` = 0000 0101 |
| 13 | `andi $t0, $t0, 0x07` | $t0 | `0x07` = 0000 0111 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x05` = 0000 0101 |
| 14 | `srl $t1, $a0, 2` | $t1 | `0x3F` = 0011 1111 | `0x38` = 0011 1000 | `0x00` = 0000 0000 | `0x2C` = 0010 1100 |
| 15 | `andi $t1, $t1, 0x07` | $t1 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x04` = 0000 0100 |
| 16 | `sll $t1, $t1, 1` | $t1 | `0x0E` = 0000 1110 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x08` = 0000 1000 |
| 17 | `add $t0, $t0, $t1` | $t0 | `0x15` = 0001 0101 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x0D` = 0000 1101 |
| 18 | `andi $t1, $a0, 0x03` | $t1 | `0x03` = 0000 0011 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x01` = 0000 0001 |
| 19 | `sll $t1, $t1, 1` | $t1 | `0x06` = 0000 0110 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x02` = 0000 0010 |
| 20 | `add $t0, $t0, $t1` | $t0 | `0x1B` = 0001 1011 | `0x07` = 0000 0111 | `0x00` = 0000 0000 | `0x0F` = 0000 1111 |
| 21 | `addi $t0, $t0, 2` | $t0 | `0x1D` = 0001 1101 | `0x09` = 0000 1001 | `0x02` = 0000 0010 | `0x11` = 0001 0001 |
| 22 | `srl $v0, $t0, 2` | $v0 | `0x07` = 0000 0111 | `0x02` = 0000 0010 | `0x00` = 0000 0000 | `0x04` = 0000 0100 |
| 23 | `jr $ra` | PC | `0x05C` (vuelve) | `0x05C` (vuelve) | `0x05C` (vuelve) | `0x05C` (vuelve) |
| 24 | `andi $t0, $a1, 0x18` | $t0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 25 | `add $t0, $t0, $v0` | $t0 | `0x07` = 0000 0111 | `0x02` = 0000 0010 | `0x00` = 0000 0000 | `0x04` = 0000 0100 |
| 26 | `lbu $v0, PALETAS($t0)` | $v0 | `0xFF` = 1111 1111 | `0x43` = 0100 0011 | `0x00` = 0000 0000 | `0xE0` = 1110 0000 |
| 27 | `lw $ra, 0($sp)` | $ra | `0x00C` | `0x00C` | `0x00C` | `0x00C` |
| 28 | `addi $sp, $sp, 4` | $sp | `0x400` | `0x400` | `0x400` | `0x400` |
| 29 | `jr $ra` | PC | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) |

#### APLICAR_FILTROS con botones = 0x07 (los tres filtros en orden)

| # | Instrucción | Destino | Fondo (0,0)<br>`$a0 = 0xFF` | Rojo (5,1)<br>`$a0 = 0xE0` | Letra (3,11)<br>`$a0 = 0x00` | Sopa (9,10)<br>`$a0 = 0xB1` |
|---|---|---|---|---|---|---|
| 1 | `addi $sp, $sp, -4` | $sp | `0x3FC` | `0x3FC` | `0x3FC` | `0x3FC` |
| 2 | `sw $ra, 0($sp)` | MEM[0x3FC] | `0x00C` | `0x00C` | `0x00C` | `0x00C` |
| 3 | `move $v0, $a0` | $v0 | `0xFF` = 1111 1111 | `0xE0` = 1110 0000 | `0x00` = 0000 0000 | `0xB1` = 1011 0001 |
| 4 | `andi $t0, $a1, 0x01` | $t0 | `0x01` = 0000 0001 | `0x01` = 0000 0001 | `0x01` = 0000 0001 | `0x01` = 0000 0001 |
| 5 | `beq $t0, $zero, F_GRIS` |  | no salta | no salta | no salta | no salta |
| 6 | `xori $v0, $v0, 0xFF` | $v0 | `0x00` = 0000 0000 | `0x1F` = 0001 1111 | `0xFF` = 1111 1111 | `0x4E` = 0100 1110 |
| 7 | `andi $t0, $a1, 0x02` | $t0 | `0x02` = 0000 0010 | `0x02` = 0000 0010 | `0x02` = 0000 0010 | `0x02` = 0000 0010 |
| 8 | `beq $t0, $zero, F_MAPA` |  | no salta | no salta | no salta | no salta |
| 9 | `move $a0, $v0` | $a0 | `0x00` = 0000 0000 | `0x1F` = 0001 1111 | `0xFF` = 1111 1111 | `0x4E` = 0100 1110 |
| 10 | `jal BRILLO` | $ra | `0x038` | `0x038` | `0x038` | `0x038` |
| 11 | `srl $t0, $a0, 5` | $t0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x07` = 0000 0111 | `0x02` = 0000 0010 |
| 12 | `andi $t0, $t0, 0x07` | $t0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x07` = 0000 0111 | `0x02` = 0000 0010 |
| 13 | `srl $t1, $a0, 2` | $t1 | `0x00` = 0000 0000 | `0x07` = 0000 0111 | `0x3F` = 0011 1111 | `0x13` = 0001 0011 |
| 14 | `andi $t1, $t1, 0x07` | $t1 | `0x00` = 0000 0000 | `0x07` = 0000 0111 | `0x07` = 0000 0111 | `0x03` = 0000 0011 |
| 15 | `sll $t1, $t1, 1` | $t1 | `0x00` = 0000 0000 | `0x0E` = 0000 1110 | `0x0E` = 0000 1110 | `0x06` = 0000 0110 |
| 16 | `add $t0, $t0, $t1` | $t0 | `0x00` = 0000 0000 | `0x0E` = 0000 1110 | `0x15` = 0001 0101 | `0x08` = 0000 1000 |
| 17 | `andi $t1, $a0, 0x03` | $t1 | `0x00` = 0000 0000 | `0x03` = 0000 0011 | `0x03` = 0000 0011 | `0x02` = 0000 0010 |
| 18 | `sll $t1, $t1, 1` | $t1 | `0x00` = 0000 0000 | `0x06` = 0000 0110 | `0x06` = 0000 0110 | `0x04` = 0000 0100 |
| 19 | `add $t0, $t0, $t1` | $t0 | `0x00` = 0000 0000 | `0x14` = 0001 0100 | `0x1B` = 0001 1011 | `0x0C` = 0000 1100 |
| 20 | `addi $t0, $t0, 2` | $t0 | `0x02` = 0000 0010 | `0x16` = 0001 0110 | `0x1D` = 0001 1101 | `0x0E` = 0000 1110 |
| 21 | `srl $v0, $t0, 2` | $v0 | `0x00` = 0000 0000 | `0x05` = 0000 0101 | `0x07` = 0000 0111 | `0x03` = 0000 0011 |
| 22 | `jr $ra` | PC | `0x038` (vuelve) | `0x038` (vuelve) | `0x038` (vuelve) | `0x038` (vuelve) |
| 23 | `sll $t0, $v0, 5` | $t0 | `0x00` = 0000 0000 | `0xA0` = 1010 0000 | `0xE0` = 1110 0000 | `0x60` = 0110 0000 |
| 24 | `sll $t1, $v0, 2` | $t1 | `0x00` = 0000 0000 | `0x14` = 0001 0100 | `0x1C` = 0001 1100 | `0x0C` = 0000 1100 |
| 25 | `or $t0, $t0, $t1` | $t0 | `0x00` = 0000 0000 | `0xB4` = 1011 0100 | `0xFC` = 1111 1100 | `0x6C` = 0110 1100 |
| 26 | `srl $t1, $v0, 1` | $t1 | `0x00` = 0000 0000 | `0x02` = 0000 0010 | `0x03` = 0000 0011 | `0x01` = 0000 0001 |
| 27 | `or $v0, $t0, $t1` | $v0 | `0x00` = 0000 0000 | `0xB6` = 1011 0110 | `0xFF` = 1111 1111 | `0x6D` = 0110 1101 |
| 28 | `andi $t0, $a1, 0x04` | $t0 | `0x04` = 0000 0100 | `0x04` = 0000 0100 | `0x04` = 0000 0100 | `0x04` = 0000 0100 |
| 29 | `beq $t0, $zero, F_FIN` |  | no salta | no salta | no salta | no salta |
| 30 | `move $a0, $v0` | $a0 | `0x00` = 0000 0000 | `0xB6` = 1011 0110 | `0xFF` = 1111 1111 | `0x6D` = 0110 1101 |
| 31 | `jal BRILLO` | $ra | `0x05C` | `0x05C` | `0x05C` | `0x05C` |
| 32 | `srl $t0, $a0, 5` | $t0 | `0x00` = 0000 0000 | `0x05` = 0000 0101 | `0x07` = 0000 0111 | `0x03` = 0000 0011 |
| 33 | `andi $t0, $t0, 0x07` | $t0 | `0x00` = 0000 0000 | `0x05` = 0000 0101 | `0x07` = 0000 0111 | `0x03` = 0000 0011 |
| 34 | `srl $t1, $a0, 2` | $t1 | `0x00` = 0000 0000 | `0x2D` = 0010 1101 | `0x3F` = 0011 1111 | `0x1B` = 0001 1011 |
| 35 | `andi $t1, $t1, 0x07` | $t1 | `0x00` = 0000 0000 | `0x05` = 0000 0101 | `0x07` = 0000 0111 | `0x03` = 0000 0011 |
| 36 | `sll $t1, $t1, 1` | $t1 | `0x00` = 0000 0000 | `0x0A` = 0000 1010 | `0x0E` = 0000 1110 | `0x06` = 0000 0110 |
| 37 | `add $t0, $t0, $t1` | $t0 | `0x00` = 0000 0000 | `0x0F` = 0000 1111 | `0x15` = 0001 0101 | `0x09` = 0000 1001 |
| 38 | `andi $t1, $a0, 0x03` | $t1 | `0x00` = 0000 0000 | `0x02` = 0000 0010 | `0x03` = 0000 0011 | `0x01` = 0000 0001 |
| 39 | `sll $t1, $t1, 1` | $t1 | `0x00` = 0000 0000 | `0x04` = 0000 0100 | `0x06` = 0000 0110 | `0x02` = 0000 0010 |
| 40 | `add $t0, $t0, $t1` | $t0 | `0x00` = 0000 0000 | `0x13` = 0001 0011 | `0x1B` = 0001 1011 | `0x0B` = 0000 1011 |
| 41 | `addi $t0, $t0, 2` | $t0 | `0x02` = 0000 0010 | `0x15` = 0001 0101 | `0x1D` = 0001 1101 | `0x0D` = 0000 1101 |
| 42 | `srl $v0, $t0, 2` | $v0 | `0x00` = 0000 0000 | `0x05` = 0000 0101 | `0x07` = 0000 0111 | `0x03` = 0000 0011 |
| 43 | `jr $ra` | PC | `0x05C` (vuelve) | `0x05C` (vuelve) | `0x05C` (vuelve) | `0x05C` (vuelve) |
| 44 | `andi $t0, $a1, 0x18` | $t0 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 | `0x00` = 0000 0000 |
| 45 | `add $t0, $t0, $v0` | $t0 | `0x00` = 0000 0000 | `0x05` = 0000 0101 | `0x07` = 0000 0111 | `0x03` = 0000 0011 |
| 46 | `lbu $v0, PALETAS($t0)` | $v0 | `0x00` = 0000 0000 | `0xF0` = 1111 0000 | `0xFF` = 1111 1111 | `0xA1` = 1010 0001 |
| 47 | `lw $ra, 0($sp)` | $ra | `0x00C` | `0x00C` | `0x00C` | `0x00C` |
| 48 | `addi $sp, $sp, 4` | $sp | `0x400` | `0x400` | `0x400` | `0x400` |
| 49 | `jr $ra` | PC | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) | `0x00C` (vuelve) |

## 5. La aritmética de bits de cada filtro

Formato del píxel: `RRRGGGBB` → R = bits 7-5, G = bits 4-2, B = bits 1-0.

### 5.1 Negativo: por qué `xori 0xFF`

XOR con 1 invierte un bit; XOR con 0 lo deja igual. `0xFF` tiene los 8 bits bajos en 1, así que invierte
exactamente el byte del píxel. Invertir todos los bits de un número de n bits da (2ⁿ − 1) − número:

- R tiene 3 bits → 7 − R. G tiene 3 bits → 7 − G. B tiene 2 bits → 3 − B.

Como los tres campos están pegados sin compartir bits, una sola `xori` hace las tres restas a la vez.
Ejemplo: `0xB1 = 101 100 01` → `010 011 10 = 0x4E`. R: 5 → 2, G: 4 → 3, B: 1 → 2. ✔

¿Por qué `xori` y no `not`/`nor`? `nor` invierte los 32 bits (`0xFFFFFF4E`); el inmediato de `xori` se
extiende con ceros, así que solo toca los 8 bits bajos.

### 5.2 Grises

Y = (R + 2G + 2B + 2) >> 2

- `2G` y `2B` son `sll 1` (correr a la izquierda multiplica por 2).
- `>> 2` es `srl 2` (correr a la derecha divide por 4, botando el residuo).
- El `+ 2` es la mitad del divisor: convierte "truncar" en "redondear al más cercano".
- `2B` existe porque B solo llega a 3: multiplicado por 2 queda de 0 a 6, parecido a R y G (0 a 7).
- Pesos: R cuenta 1/4, G cuenta 2/4 (el ojo es más sensible al verde) y B (ya escalado) 1/4.
- Nunca se pasa de 7: el máximo es (7 + 14 + 6 + 2) >> 2 = 29 >> 2 = 7. No puede desbordar.

Armar el gris: R' = Y → `sll 5`; G' = Y → `sll 2`; B' = Y >> 1 → `srl 1` (B solo tiene 2 bits, de 0 a 3,
y Y va de 0 a 7). Píxel = `(Y << 5) | (Y << 2) | (Y >> 1)`.

Ejemplo `0xB1`: R = 5, G = 4, B = 1 → 5 + 8 + 2 + 2 = 17 → 17 >> 2 = 4 → `100 100 10 = 0x92`. ✔
Ejemplo `0xE0`: R = 7, G = 0, B = 0 → 9 >> 2 = 2 → `010 010 01 = 0x49`. ✔

### 5.3 Mapa de color (filtro 3)

Y otra vez (misma función `BRILLO`), y con él se lee una tabla: `PALETAS[(botones AND 0x18) + Y]`.

- `0x18 = 0001 1000` deja los bits 3 y 4, que valen 8 y 16. Las combinaciones dan 0, 8, 16 o 24.
- Cada paleta mide 8 bytes, así que la paleta 0 empieza en +0, la 1 en +8, la 2 en +16 y la 3 en +24.
  El valor de los botones ya es el desplazamiento: no hay que correr nada.
- La dirección final es `0x300 + 8·paleta + Y` (base + desplazamiento).

Ejemplo `0xB1` con `0x04` (térmica): Y = 4 → `PALETAS[0 + 4] = 0xE0` (rojo).
Con `0x14` (umbral): `PALETAS[16 + 4] = 0xFF` (blanco, porque Y ≥ 4).

### 5.4 De dónde sale cada bit del color de 16 colores

Paleta: bit 0 = rojo, bit 1 = verde, bit 2 = azul, bit 3 = brillo.

```
píxel:   bit   7   6   5   4   3   2   1   0
               R2  R1  R0  G2  G1  G0  B1  B0
               │   │       │   │       │
               │   └──OR───┼───┘       │
               ▼      ▼    ▼           ▼
color:       bit0   bit3  bit1        bit2
             rojo  brillo verde       azul
```

| Bit del color | Viene de | Corrimiento | Máscara |
|---|---|---|---|
| 0 rojo | bit 7 (R alto) | `srl 7` | `0x01` |
| 1 verde | bit 4 (G alto) | `srl 3` | `0x02` |
| 2 azul | bit 1 (B alto) | `sll 1` | `0x04` |
| 3 brillo | bit 6 OR bit 3 | `srl 3` y OR con el original | `0x08` |

Idea: de cada canal se usa el bit más significativo ("¿hay mucho de este color?") y el brillo se enciende
si el segundo bit del rojo o del verde está en 1.

Ejemplo `0xB1 = 1011 0001`: bit 7 = 1, bit 4 = 1, bit 1 = 0, bit 6 = 0, bit 3 = 0 → `0011` = 3 (amarillo). ✔
Ejemplo `0xE0 = 1110 0000`: bit 7 = 1, bit 4 = 0, bit 1 = 0, bit 6 = 1 → `1001` = 9 (rojo brillante). ✔

### 5.5 De dónde sale cada bit de `0x00RRGGBB`

```
píxel (8 bits):                         R2 R1 R0 G2 G1 G0 B1 B0
                                         7  6  5  4  3  2  1  0

color (24 bits):  RR = R2 R1 R0 0 0 0 0 0   GG = G2 G1 G0 0 0 0 0 0   BB = B1 B0 0 0 0 0 0 0
                       23 22 21                  15 14 13                  7  6
```

| Canal | Está en | Debe quedar en | Corrimiento | Equivale a |
|---|---|---|---|---|
| R | bits 7-5 | bits 23-21 | `sll 16` | R << 21 (porque ya estaba corrido 5) |
| G | bits 4-2 | bits 15-13 | `sll 11` | G << 13 (ya estaba corrido 2) |
| B | bits 1-0 | bits 7-6 | `sll 6` | B << 6 |

Cada canal se pone en los bits **altos** de su byte para que conserve su brillo relativo; los bits bajos
quedan en 0. Por eso el blanco `0xFF` no da `0xFFFFFF` sino `0xE0E0C0`: 7 se vuelve `1110 0000` = 224 y
3 se vuelve `1100 0000` = 192. Como el azul queda más bajo que rojo y verde, el blanco se ve amarillento.

Ejemplo `0xB1`: R = `101` → `1010 0000 = 0xA0`; G = `100` → `0x80`; B = `01` → `0100 0000 = 0x40` → `0xA08040`. ✔

## 6. Direcciones en cada pantalla y la ampliación 2 × 2

### 6.1 Leer la imagen

`dirección = IMAGEN + 16·y + x = 0x200 + (y << 4) + x`

La imagen se guarda por filas y cada fila mide 16 bytes. Ejemplo: píxel (9, 10) → 0x200 + 160 + 9 =
`0x2A9`, donde está `0xB1`. En el código: `sll $t0, $s0, 4` / `add $t0, $t0, $s1` / `lbu $s2, IMAGEN($t0)`.
La base `0x200` va como **inmediato** de la `lbu` y el desplazamiento en el registro.

### 6.2 Pantalla de 16 colores (memoria)

`dirección = 0x400 + 32·y + x` con x de 0 a 31.

- Mitad izquierda: x de imagen → `sb $v0, 0x400($s3)` con `$s3 = 32·y + x`.
- Mitad derecha: x + 16 → `0x400 + 32·y + x + 16` = `sb $v0, 0x410($s3)`. El +16 se metió en el inmediato.

Ejemplo: píxel (9, 10) → `$s3` = 320 + 9 = 329 = `0x149`; izquierda `0x549`, derecha `0x559`.

Ojo con los dos corrimientos distintos: la imagen tiene 16 columnas (`sll 4`) y la pantalla 32 (`sll 5`).

### 6.3 Pantalla RGB (registros de comando)

No hay dirección por píxel. Hay dos registros:

- `0x604` posición = (y << 16) | x. Se escribe **una vez** con 0 → (0, 0).
- `0x608` color: cada `sw` dibuja en la posición actual y avanza x; cuando x llega a 32 vuelve a 0 y sube y.

Como el programa manda los 1024 colores en orden (fila por fila, de izquierda a derecha), cada uno cae en
su sitio sin calcular nada. Al final la posición da la vuelta y queda otra vez en (0, 0).

### 6.4 Ampliación 2 × 2

El píxel (x, y) de la **pantalla** muestra el píxel (x/2, y/2) de la **imagen**.

```
x de pantalla:   0  1  2  3  4  5 ...  30 31
x/2 (srl 1):     0  0  1  1  2  2 ...  15 15      ← cada columna de la imagen sale 2 veces
```

Lo mismo con y: las filas 0 y 1 de pantalla leen la fila 0 de la imagen, la 2 y 3 leen la 1, etc. Así cada
píxel de la imagen ocupa un bloque de 2 × 2. Código: `srl $t0, $s0, 1` (y/2), `sll $t0, $t0, 4` (×16),
`srl $t1, $s1, 1` (x/2), `add`.

Por qué no se puede escribir `sll $t0, $s0, 3` en lugar de `srl 1` + `sll 4`: con y = 3, `3 << 3 = 24`, pero
lo correcto es `(3 >> 1) << 4 = 16`. El `srl` bota el bit bajo de y; eso es precisamente lo que hace que
dos filas seguidas lean la misma fila de la imagen.

Costo: cada píxel de la imagen se filtra 4 veces. Es más lento pero el código es directo.

## 7. Preguntas probables del profesor (25) con respuestas cortas

1. **¿Cómo hace el negativo en una sola instrucción?** `xori $v0, $v0, 0xFF`. Invertir los bits de un
   campo de n bits da (2ⁿ − 1) − valor: 7 − R, 7 − G y 3 − B, todos a la vez porque los campos no se solapan.
2. **¿Por qué `lbu` y no `lb`?** El píxel es un byte sin signo. `lb` extiende el bit 7: `0xB1` quedaría
   `0xFFFFFFB1`. (Lo probamos: nuestras funciones enmascaran con `andi` y la imagen sale igual, pero el
   valor del registro ya no es el píxel; lo correcto es `lbu`.)
3. **¿Por qué no `lw` para leer el píxel?** `lw` trae 4 píxeles en una palabra y exige dirección múltiplo
   de 4. Además por ser big-endian el primer píxel queda en el byte alto: `lw` en `0x200` da `0xFFFFEDED`.
4. **¿Por qué `APLICAR_FILTROS` guarda `$ra` y las otras no?** Porque es la única que hace `jal` (a
   `BRILLO`), y `jal` sobrescribe `$ra`. Las otras tres son hojas: no llaman a nadie.
5. **¿Qué pasa si quita el push y el pop?** Lo probamos: con `0x00` y `0x01` funciona (no se llama a
   `BRILLO`); con `0x02` o `0x04` pinta un píxel y queda en ciclo infinito, porque `jr $ra` vuelve a la
   instrucción siguiente al `jal BRILLO`, dentro de la misma función.
6. **¿Dónde queda la pila y cuánto usa?** Empieza en `0x400` y crece hacia abajo. Usa una sola palabra,
   `0x3FC`, que guarda `0x040` (16 colores) o `0x090` (RGB). No choca con las paletas (`0x300-0x31F`).
7. **¿Por qué el píxel va en `$s2` y no en un `$t`?** Se necesita dos veces (izquierda y derecha) y en
   medio hay llamadas que usan `$t0` y `$t1`. Los `$s` son los que las funciones no tocan.
8. **¿Qué pasaría si una función modifica `$s1`?** Lo probamos: `A_16COLORES` con `$s1` de temporal deja
   x en 0 u 8; el ciclo se queda atrapado en x = 9, pinta 4 píxeles y nunca termina.
9. **¿Cómo calcula la dirección de un píxel de la imagen?** `0x200 + 16·y + x`: `sll 4`, `add`, y la
   base va en el inmediato de `lbu`. Base + desplazamiento.
10. **¿Por qué `0x410` en el segundo `sb`?** `0x400 + 16`: misma fila, 16 columnas a la derecha. Se
    ahorra una suma metiendo el 16 en el inmediato.
11. **¿Por qué `sll 4` en la imagen y `sll 5` en la pantalla?** La imagen tiene 16 columnas (2⁴) y la
    pantalla 32 (2⁵).
12. **¿Cómo sabe la pantalla RGB dónde dibujar si no le manda la posición?** Se la manda una vez
    (`0x604` = 0). Cada escritura en `0x608` avanza x sola y baja de fila al final.
13. **¿Qué ganó con el autoincremento?** 3 instrucciones y 1 escritura menos por píxel: 3072
    instrucciones y 1024 escrituras en total.
14. **¿Cómo hace la ampliación 2 × 2?** Recorre la pantalla de 32 × 32 y lee el píxel (x >> 1, y >> 1).
15. **¿Cómo divide por 4 sin `div`?** `srl 2`. Y multiplica por 2 con `sll 1`.
16. **¿Para qué el `+ 2` en el brillo?** Para redondear: es la mitad del divisor (4).
17. **¿Por qué B' = Y >> 1?** B solo tiene 2 bits (0 a 3) y Y va de 0 a 7.
18. **¿Cómo se combinan los filtros?** El resultado parcial vive en `$v0`. Cada filtro revisa su botón
    con `andi` + `beq` y, si está activo, transforma `$v0`. Van en orden 1, 2, 3 dentro de la función.
19. **¿Cuántas veces lee los botones? ¿Y si los cambio mientras corre?** Una vez (`0x000`). Si se cambian
    después no pasa nada hasta reiniciar con `0`.
20. **¿Puede el programa escribir en `0x600`?** Puede ejecutar el `sw`, pero el dispositivo lo ignora: no
    hay excepción y el valor no cambia. Lo probamos.
21. **¿Cuántas instrucciones tiene y cuántas ejecuta?** 105 (420 bytes; el máximo es 128). Ejecuta de
    14149 (`0x00`) a 72838 (`0x87`). Por eso necesita `-n 100000`: el límite por defecto es 10000.
22. **¿`li` y `move` existen en el procesador?** No, son pseudoinstrucciones: `li $s0, 0` es
    `addi $s0, $zero, 0` y `move $a0, $s2` es `add $a0, $s2, $zero`. Cada una es una instrucción real.
23. **¿Por qué `slti` + `bne` y no `blt`?** `blt` con un número se expande a 3 instrucciones (`addi $at`,
    `slt`, `bne`) y usa `$at`. `slti` + `bne` son 2 y se ve exactamente qué se ejecuta.
24. **¿Cómo termina el programa?** `j FIN`, y `FIN` está al final de `.text`. ArquiSim termina cuando el
    PC llega ahí (`0x1A4`). No usa `syscall`.
25. **¿Cómo garantiza que no modifica la imagen?** Solo hay 4 instrucciones de escritura: dos `sb` a la
    pantalla (`0x400+`), un `sw` a la pila (`0x3FC`) y los `sw` a `0x604`/`0x608`. Ninguna cae en `0x200-0x2FF`.

Preguntas extra que también pueden salir:

26. **¿Qué formato tiene cada instrucción?** Tipo R: `add or sll srl jr` (y `move`). Tipo I:
    `addi andi xori slti lw sw lbu sb beq bne` (y `li`). Tipo J: `j jal`. `jr` es tipo R aunque sea un salto.
27. **Codifique `bne $t0, $zero, RGB`.** `0x15000017`: opcode `000101`, rs = `$t0` (8), rt = 0, inmediato
    `0x17` = 23. Es relativo al PC: destino = (0x008 + 4) + 23 × 4 = `0x068`.
28. **Codifique `jal A_16COLORES`.** Pseudodirecto: el campo es la dirección / 4 → `0x14C` / 4 = `0x53` →
    `0x0C000053`. Y `j FIN` es `0x08000069` (`0x1A4` / 4 = `0x69`).
29. **¿Por qué usa `$a1` después de llamar a `BRILLO`? ¿No lo puede dañar?** Por convención general sí
    podría, pero `BRILLO` es nuestra y solo toca `$t0`, `$t1` y `$v0` (está escrito en su encabezado). Si
    alguien la cambiara para usar `$a1`, habría que guardarlo en la pila junto con `$ra`.
30. **¿Negativo y luego grises da lo mismo que al revés?** Sí, en los 256 píxeles (demostración en
    `datos_informe.md`). Con el filtro 3 el orden sí importa.
31. **¿Por qué el blanco se ve amarillento en RGB?** `0xFF` → `0xE0E0C0`: los canales se ponen en los
    bits altos y el azul solo tiene 2 bits (queda en 192 y no en 224).
32. **¿Qué mejoraría?** Filtrar cada píxel una sola vez en RGB (hoy se filtra 4) y guardar `$ra` solo
    cuando de verdad se llama a `BRILLO` (ahorra 4 instrucciones y 2 accesos a memoria por píxel).
33. **¿El tiempo depende de la imagen?** No. Ningún salto depende del píxel; se comprobó con una imagen aleatoria.

## 8. Modificaciones en vivo (código exacto, ya probado)

Cada una está ensamblada y verificada en `modificaciones/` (`python herramientas/probar_modificaciones.py`).
Se muestran las líneas que se cambian en `proyecto.asm`.

### 8.1 Poner la original a la derecha y la filtrada a la izquierda (sigue en 105 instrucciones)

Intercambiar los dos inmediatos de los `sb`:

```asm
        jal  A_16COLORES            # (el primero, con el píxel original)
        sb   $v0, 0x410($s3)        # ANTES 0x400: ahora la original va a la derecha
        ...
        jal  A_16COLORES            # (el segundo, con el píxel filtrado)
        sb   $v0, 0x400($s3)        # ANTES 0x410: ahora la filtrada va a la izquierda
```

### 8.2 Espejo horizontal de la imagen filtrada (109 instrucciones)

Idea: la columna espejada es 15 − x, y para un número de 4 bits eso es `xori 15`.

En el ciclo de 16 colores, **reemplazar** `sb $v0, 0x410($s3)` por:

```asm
        xori $t1, $s1, 15           # $t1 = 15 - x
        sll  $t0, $s0, 5            # 32*y
        add  $t0, $t0, $t1          # 32*y + (15 - x)
        sb   $v0, 0x410($t0)        # mitad derecha, columna espejada
```

En el ciclo RGB, **agregar** una línea después de `srl $t1, $s1, 1`:

```asm
        srl  $t1, $s1, 1            # x/2
        xori $t1, $t1, 15           # NUEVA: leer la columna 15 - x/2
```

Espejo **vertical** (también probado, `2b_espejo_vertical.asm`): igual pero con y. En 16 colores,
reemplazar `sb $v0, 0x410($s3)` por:

```asm
        xori $t1, $s0, 15           # $t1 = 15 - y
        sll  $t0, $t1, 5            # 32*(15 - y)
        add  $t0, $t0, $s1          # + x
        sb   $v0, 0x410($t0)        # mitad derecha, fila espejada
```

y en RGB agregar `xori $t0, $t0, 15` después de `srl $t0, $s0, 1`.

### 8.3 Cambiar el filtro 3

**a) Cambiar una paleta** (lo más rápido): editar los 8 bytes de una línea `.byte` de `PALETAS`. Ejemplo
(probado), umbral más exigente (blanco solo si Y ≥ 6), en la paleta 2:

```asm
        .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF    # paleta 2: umbral en 6
```

**b) Dejar solo el canal rojo** (101 instrucciones). Reemplazar las 5 líneas que van desde
`move $a0, $v0` hasta `lbu $v0, PALETAS($t0)` (las que están debajo de `F_MAPA`) por:

```asm
        andi $v0, $v0, 0xE0         # dejar solo R (borra G y B)
```

(Solo verde: `0x1C`. Solo azul: `0x03`. Quitar el azul: `0xFC`.)

**c) Intercambiar rojo y verde** (107 instrucciones). Reemplazar las mismas 5 líneas por:

```asm
        andi $t0, $v0, 0xE0         # R (bits 7-5)
        srl  $t0, $t0, 3            # R baja al lugar de G (bits 4-2)
        andi $t1, $v0, 0x1C         # G (bits 4-2)
        sll  $t1, $t1, 3            # G sube al lugar de R (bits 7-5)
        andi $v0, $v0, 0x03         # B se queda donde está
        or   $v0, $v0, $t0          # juntar
        or   $v0, $v0, $t1          # juntar
```

En b) y c) las líneas `F_MAPA: andi $t0, $a1, 0x04` y `beq $t0, $zero, F_FIN` se dejan como están.

### 8.4 Pantalla RGB sin ampliar, 1 × 1 (104 instrucciones)

La imagen queda de 16 × 16 en la esquina superior izquierda. Como el autoincremento pasa de fila en la
columna 32 y no en la 16, hay que escribir la posición al empezar **cada fila**. Reemplazar el inicio del
ciclo RGB (desde `RGB:` hasta el `add $t0, $t0, $t1`) por:

```asm
RGB:    li   $s0, 0                 # y = 0
R_FILA: sll  $t0, $s0, 16           # posición = (y << 16) | 0: inicio de la fila y
        sw   $t0, 0x604($zero)      # se escribe al empezar CADA fila (16 veces)
        li   $s1, 0                 # x = 0
R_COL:  sll  $t0, $s0, 4            # 16*y (ya no se divide por 2)
        add  $t0, $t0, $s1          # 16*y + x
```

y cambiar los dos límites:

```asm
        slti $t0, $s1, 16           # ANTES 32
        ...
        slti $t0, $s0, 16           # ANTES 32
```

### 8.5 Otro orden de filtros: grises → negativo → mapa (sigue en 105)

Mover el bloque del negativo debajo del de grises y corregir las etiquetas de los `beq`:

```asm
        move $v0, $a0               # (igual que antes)

        andi $t0, $a1, 0x02         # primero el filtro 2 (grises)
        beq  $t0, $zero, F_NEG      # ANTES saltaba a F_MAPA
        move $a0, $v0
        jal  BRILLO
        sll  $t0, $v0, 5
        sll  $t1, $v0, 2
        or   $t0, $t0, $t1
        srl  $t1, $v0, 1
        or   $v0, $t0, $t1

F_NEG:  andi $t0, $a1, 0x01         # después el filtro 1 (negativo)
        beq  $t0, $zero, F_MAPA     # ANTES saltaba a F_GRIS
        xori $v0, $v0, 0xFF

F_MAPA: andi $t0, $a1, 0x04         # (igual que antes)
```

Dato para comentar: con `0x03` el resultado es idéntico al original, porque negativo y grises conmutan.

### 8.6 Otros cambios pequeños que pueden pedir

| Cambio | Qué tocar |
|---|---|
| Que el botón 6 active el negativo (en vez del 0) | `andi $t0, $a1, 0x01` → `andi $t0, $a1, 0x40` |
| Que la pantalla RGB sea la de por defecto | `bne $t0, $zero, RGB` → `beq $t0, $zero, RGB` |
| Dibujar solo la mitad de arriba (16 colores) | `slti $t0, $s0, 16` → `slti $t0, $s0, 8` |
| Usar los botones 5 y 6 para la paleta | `andi $t0, $a1, 0x18` → `andi $t0, $a1, 0x60` y agregar `srl $t0, $t0, 2` (para que valga 0, 8, 16, 24) |

(Los cuatro están probados: tres quedaron en `modificaciones/` como `6a`, `6c` y `6d`; el de la pantalla
por defecto se comprobó aparte: con el cambio, `0x00` dibuja lo que antes dibujaba `0x80`.)

### 8.7 Cómo no enredarse al modificar en vivo

1. Diga primero en voz alta qué va a cambiar y en qué bloque (sección 2).
2. Si agrega instrucciones no tiene que recalcular saltos: el ensamblador lo hace con las etiquetas.
3. Si necesita un registro nuevo dentro de una función use `$t0` o `$t1`; en el principal, un `$s` libre
   (`$s4`, `$s5`, `$s6`). Nunca un `$s` dentro de una función.
4. Recargue con `r` en ArquiSim y pruebe con `0x00` primero (las dos mitades deben seguir iguales, salvo en 8.1-8.2).
5. Quedan 23 instrucciones libres; ninguna de estas modificaciones se acerca a 128.

## 9. Hoja de datos (para tener a la mano)

| Dato | Valor |
|---|---|
| Instrucciones / bytes de `.text` | 105 / 420 (máximo 128 / 512) |
| Funciones | `APLICAR_FILTROS` (25), `BRILLO` (12), `A_16COLORES` (13), `A_RGB` (9) |
| Imagen / paletas / pila | 256 B en `0x200` / 32 B en `0x300` / 4 B en `0x3FC` |
| Instrucciones ejecutadas | 14149 (`0x00`) … 23621 (`0x07`); 34950 (`0x80`) … 72838 (`0x87`) |
| Por píxel sin filtros | 55 (16 colores, pinta 2 píxeles) y 34 (RGB) |
| Costo de cada filtro por píxel | negativo +1, grises +19, mapa de color +17 |
| Peor caso a 1 MHz / 100 MHz | 72.8 ms (13.7 imágenes/s) / 0.73 ms (1373 imágenes/s) |
| `lw` en `0x200` (imagen de ejemplo) | `0xFFFFEDED` |
| Direcciones clave | `C_COL 0x014`, `RGB 0x068`, `APLICAR_FILTROS 0x0B8`, `BRILLO 0x11C`, `A_16COLORES 0x14C`, `A_RGB 0x180`, `FIN 0x1A4` |
| Botones | 0 negativo, 1 grises, 2 mapa de color, 3-4 paleta, 7 pantalla |
| Paletas (`mem[0x600]`) | `0x04` térmica, `0x0C` sepia, `0x14` umbral, `0x1C` visión nocturna (+`0x80` para RGB) |

Comandos útiles en la sustentación:

```
./sim -n 100000 programas/proyecto.asm
mem[0x600] = 0x87        botones
bp APLICAR_FILTROS       punto de parada (también bp BRILLO, bp C_COL, bp 0x0CC)
c                        continuar hasta el punto de parada o el final
Enter                    una instrucción
p                        cambiar de panel (programa / 16 colores / RGB)
m 0x300                  ver las paletas     m 0x200  ver la imagen
0                        reiniciar conservando los botones
r                        recargar el archivo después de editarlo
```
