# Proyecto final -- Filtros de imagen con ArquiSim
# Arquitectura del Computador - Pontificia Universidad Javeriana
#
# El programa lee los botones UNA vez al empezar (palabra 0x600) y dibuja la imagen
# de .data con los filtros elegidos:
#
#   botón 0   filtro 1: negativo                  botón 7 = 0  pantalla de 16 colores:
#   botón 1   filtro 2: escala de grises                       original (x 0-15) | filtrada (x 16-31)
#   botón 2   filtro 3: mapa de color             botón 7 = 1  pantalla RGB 32 x 32:
#   botones 3 y 4: paleta del filtro 3                         filtrada, cada píxel en 2 x 2
#       00 térmica   01 sepia   10 umbral blanco/negro   11 visión nocturna
#
# Los filtros activos se aplican en orden 1 -> 2 -> 3 y se pueden combinar.
#
# Cómo probarlo:
#   (copie este archivo a la carpeta programas/ de su simulador)
#   ./sim -n 100000 programas/proyecto.asm
#   mem[0x600] = 0x81      activa los botones 0 y 7 (o use bt 0 y bt 7)
#   c                      ejecuta hasta el final;  p  cambia de pantalla
#   0                      reinicia: los botones se conservan para probar otra vez
#
# ---------------------------------------------------------------------------------
# MAPA DE MEMORIA
#   0x000-0x1FF  .text    este programa: 105 instrucciones = 420 bytes (máximo 128)
#   0x200-0x2FF  IMAGEN   16 x 16 bytes RGB332 (RRRGGGBB). NO se modifica.
#   0x300-0x31F  PALETAS  4 paletas de 8 colores para el filtro 3
#   0x3FC        pila     una sola palabra: el $ra que guarda APLICAR_FILTROS ($sp = 0x400)
#   0x400-0x5FF  pantalla de 16 colores: dirección = 0x400 + 32*y + x
#   0x600        botones (solo lectura)
#   0x604        RGB posición = (y << 16) | x
#   0x608        RGB color 0x00RRGGBB: dibuja el píxel y avanza x
#
# MAPA DE REGISTROS
#   Programa principal (registros $s: las funciones NO los tocan)
#     $s0  y: fila actual     (0-15 en 16 colores, 0-31 en RGB)
#     $s1  x: columna actual  (0-15 en 16 colores, 0-31 en RGB)
#     $s2  píxel original leído de la imagen (se usa dos veces: izquierda y derecha)
#     $s3  32*y + x: posición del píxel dentro de la pantalla de 16 colores
#     $s7  botones (se leen una sola vez)
#   Paso de parámetros
#     $a0  argumento: el píxel RGB332        $a1  segundo argumento: los botones
#     $v0  resultado de la función           $ra  dirección de retorno (la pone jal)
#   Temporales (cualquiera los puede dañar; nunca se usan a través de un jal)
#     $t0, $t1
#   $sp  pila: APLICAR_FILTROS guarda ahí su $ra porque llama a BRILLO
#
# FUNCIONES (todas: argumento en $a0, resultado en $v0)
#   APLICAR_FILTROS  píxel + botones ($a1) -> píxel filtrado       (llama a BRILLO)
#   BRILLO           píxel -> brillo Y de 0 a 7                    (hoja)
#   A_16COLORES      píxel -> color 0-15 de la pantalla 0x400      (hoja)
#   A_RGB            píxel -> color 0x00RRGGBB de la pantalla RGB  (hoja)
# ---------------------------------------------------------------------------------
#
# La imagen ocupa 0x200-0x2FF (16 x 16 bytes, RGB332 = RRRGGGBB). Para cambiarla,
# reemplace el bloque de abajo por la salida de img_a_arquisim (vea la sección
# "Preparar la imagen" del enunciado). El programa NO debe modificar la imagen.
#
# >>> IMAGEN (generado con: ./img_a_arquisim ejemplo.bmp, a partir de ejemplo.jpg)
# Imagen generada con img_a_arquisim a partir de 'ejemplo.bmp' (1500 x 1500)
# 16 x 16 píxeles, 1 byte por píxel en formato RGB332 (RRRGGGBB) = 256 bytes
# El píxel (x, y) está en la dirección IMAGEN + 16*y + x
.data
IMAGEN:
        .byte 0xFF, 0xFF, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xFF, 0xFF    # fila 0
        .byte 0xFF, 0xFF, 0xE0, 0xE4, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE4, 0xFF, 0xFF    # fila 1
        .byte 0xFF, 0xFF, 0xE9, 0xE5, 0xC4, 0xA0, 0xA0, 0xA0, 0xE0, 0xE4, 0xE4, 0xE0, 0xE0, 0xE9, 0xFF, 0xFF    # fila 2
        .byte 0xFF, 0xFF, 0xF2, 0xE0, 0xC4, 0x85, 0x44, 0xA4, 0xE4, 0xE4, 0xE4, 0xE4, 0xE0, 0xF2, 0xFF, 0xFF    # fila 3
        .byte 0xFF, 0xFF, 0xF2, 0xE0, 0x65, 0x44, 0x65, 0x65, 0xE0, 0xE4, 0xE4, 0xE4, 0xE0, 0xF2, 0xFF, 0xFF    # fila 4
        .byte 0xFF, 0xFF, 0xF2, 0xE0, 0x89, 0x45, 0x24, 0x69, 0x84, 0xE4, 0xE4, 0xE4, 0xE0, 0xF6, 0xFF, 0xFF    # fila 5
        .byte 0xFF, 0xFF, 0xF2, 0xC4, 0x49, 0x49, 0x45, 0x89, 0x69, 0xA4, 0xC4, 0xC0, 0xE0, 0xF6, 0xFF, 0xFF    # fila 6
        .byte 0xFF, 0xFF, 0xF2, 0xE0, 0xA4, 0x65, 0x65, 0x84, 0xA5, 0x69, 0x89, 0x85, 0xE0, 0xF6, 0xFF, 0xFF    # fila 7
        .byte 0xFF, 0xFF, 0xF2, 0xE0, 0xE4, 0xA5, 0x84, 0xE0, 0xA8, 0x88, 0x88, 0xA8, 0xE0, 0xF6, 0xFF, 0xFF    # fila 8
        .byte 0xFF, 0xFF, 0xF2, 0xE0, 0xE4, 0xA4, 0x84, 0xC0, 0x89, 0x6D, 0x6D, 0x68, 0x84, 0xD6, 0xFF, 0xFF    # fila 9
        .byte 0xFF, 0xFF, 0xF6, 0xE4, 0xA4, 0xE4, 0x65, 0x44, 0x8D, 0xB1, 0xD5, 0x71, 0x91, 0xDA, 0xFF, 0xFF    # fila 10
        .byte 0xFF, 0xFF, 0x92, 0x00, 0x20, 0x24, 0x69, 0xD1, 0xD5, 0x8D, 0xA8, 0x68, 0x95, 0xDA, 0xFF, 0xFF    # fila 11
        .byte 0xFF, 0xFF, 0x92, 0x00, 0x20, 0x24, 0xAD, 0xCC, 0xAD, 0xAD, 0x84, 0x90, 0xAC, 0xD6, 0xFF, 0xFF    # fila 12
        .byte 0xFF, 0xFF, 0x6D, 0x00, 0x40, 0x00, 0x24, 0x68, 0x68, 0x88, 0xAC, 0x88, 0x88, 0xB1, 0xFF, 0xFF    # fila 13
        .byte 0xFF, 0xFF, 0x24, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0xFF, 0xFF    # fila 14
        .byte 0xFF, 0xFF, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xFF, 0xFF    # fila 15
# <<< IMAGEN

# Paletas del filtro 3 (mapa de color). Quedan justo después de la imagen: 0x300-0x31F.
# Cada paleta tiene 8 colores RGB332, uno por cada brillo Y de 0 (oscuro) a 7 (claro).
# El color del píxel filtrado es  PALETAS[8*paleta + Y].
.data
PALETAS:
        .byte 0x00, 0x02, 0x43, 0xA1, 0xE0, 0xF0, 0xFC, 0xFF    # paleta 0 (botones 4,3 = 00): térmica
        .byte 0x00, 0x24, 0x44, 0x68, 0x8D, 0xB1, 0xD1, 0xF6    # paleta 1 (botones 4,3 = 01): sepia
        .byte 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF    # paleta 2 (botones 4,3 = 10): umbral blanco/negro
        .byte 0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C    # paleta 3 (botones 4,3 = 11): visión nocturna

.text
# =================================================================================
# PROGRAMA PRINCIPAL
# =================================================================================
        lw   $s7, 0x600($zero)      # $s7 = botones. Es la ÚNICA lectura de 0x600 en todo el programa
        andi $t0, $s7, 0x80         # aislar el bit 7: ¿cuál pantalla?
        bne  $t0, $zero, RGB        # bit 7 = 1 -> ir a dibujar en la pantalla RGB

# ---------------------------------------------------------------------------------
# Pantalla de 16 colores (bit 7 = 0): original a la izquierda, filtrada a la derecha.
# Dos ciclos anidados: para y = 0..15, para x = 0..15.
# ---------------------------------------------------------------------------------
        li   $s0, 0                 # y = 0 (primera fila)
C_FILA: li   $s1, 0                 # x = 0 (primera columna de esta fila)
C_COL:  sll  $t0, $s0, 4            # $t0 = 16*y  (correr 4 bits = multiplicar por 16)
        add  $t0, $t0, $s1          # $t0 = 16*y + x: desplazamiento del píxel dentro de la imagen
        lbu  $s2, IMAGEN($t0)       # $s2 = píxel (x, y). lbu: sin signo, para que 0xFF sea 255 y no -1
        sll  $s3, $s0, 5            # $s3 = 32*y  (la pantalla tiene 32 columnas)
        add  $s3, $s3, $s1          # $s3 = 32*y + x: desplazamiento del píxel dentro de la pantalla

        move $a0, $s2               # argumento: el píxel original
        jal  A_16COLORES            # $v0 = color 0-15 del píxel original
        sb   $v0, 0x400($s3)        # mitad izquierda: pantalla[0x400 + 32*y + x]

        move $a0, $s2               # argumento 1: otra vez el píxel original (sigue en $s2)
        move $a1, $s7               # argumento 2: los botones
        jal  APLICAR_FILTROS        # $v0 = píxel filtrado (RGB332)
        move $a0, $v0               # el resultado de una función es el argumento de la siguiente
        jal  A_16COLORES            # $v0 = color 0-15 del píxel filtrado
        sb   $v0, 0x410($s3)        # mitad derecha: 0x410 = 0x400 + 16, o sea la columna x + 16

        addi $s1, $s1, 1            # x = x + 1
        slti $t0, $s1, 16           # $t0 = 1 si x < 16
        bne  $t0, $zero, C_COL      # todavía quedan columnas en esta fila
        addi $s0, $s0, 1            # y = y + 1
        slti $t0, $s0, 16           # $t0 = 1 si y < 16
        bne  $t0, $zero, C_FILA     # todavía quedan filas
        j    FIN                    # listo: saltar por encima del resto hasta el final de .text

# ---------------------------------------------------------------------------------
# Pantalla RGB (bit 7 = 1): solo la imagen filtrada, ampliada 2 x 2 (32 x 32 píxeles).
# Aquí x, y recorren la PANTALLA (0..31); el píxel que se lee es (x/2, y/2) de la imagen,
# así cada píxel de la imagen sale en 2 columnas y en 2 filas seguidas.
# ---------------------------------------------------------------------------------
RGB:    sw   $zero, 0x604($zero)    # posición inicial (0, 0). Se escribe UNA sola vez
        li   $s0, 0                 # y de pantalla = 0
R_FILA: li   $s1, 0                 # x de pantalla = 0
R_COL:  srl  $t0, $s0, 1            # $t0 = y/2: fila de la imagen (correr 1 a la derecha = dividir por 2)
        sll  $t0, $t0, 4            # $t0 = 16*(y/2)
        srl  $t1, $s1, 1            # $t1 = x/2: columna de la imagen
        add  $t0, $t0, $t1          # $t0 = 16*(y/2) + x/2
        lbu  $a0, IMAGEN($t0)       # argumento 1: píxel (x/2, y/2) de la imagen
        move $a1, $s7               # argumento 2: los botones
        jal  APLICAR_FILTROS        # $v0 = píxel filtrado (RGB332)
        move $a0, $v0               # pasa a ser el argumento de A_RGB
        jal  A_RGB                  # $v0 = color 0x00RRGGBB
        sw   $v0, 0x608($zero)      # dibuja el píxel; el dispositivo avanza x solo (y baja de fila al final)

        addi $s1, $s1, 1            # x = x + 1 (solo para saber cuándo parar y qué píxel leer)
        slti $t0, $s1, 32           # $t0 = 1 si x < 32
        bne  $t0, $zero, R_COL      # todavía quedan columnas en esta fila
        addi $s0, $s0, 1            # y = y + 1
        slti $t0, $s0, 32           # $t0 = 1 si y < 32
        bne  $t0, $zero, R_FILA     # todavía quedan filas
        j    FIN                    # listo

# =================================================================================
# APLICAR_FILTROS: aplica los filtros activos en orden 1 -> 2 -> 3.
#   $a0 = píxel RGB332     $a1 = botones     ->     $v0 = píxel filtrado RGB332
# El resultado se va acumulando en $v0: cada filtro toma lo que dejó el anterior.
# Como llama a BRILLO con jal (que cambia $ra), primero guarda su propio $ra en la pila.
# Usa $a1 después de llamar a BRILLO: se puede porque BRILLO solo toca $t0, $t1 y $v0.
# =================================================================================
APLICAR_FILTROS:
        addi $sp, $sp, -4           # push: reservar una palabra en la pila ($sp baja a 0x3FC)
        sw   $ra, 0($sp)            #       y guardar ahí la dirección de retorno
        move $v0, $a0               # resultado parcial = píxel sin cambios (por si no hay filtros)

        andi $t0, $a1, 0x01         # ¿botón 0? (filtro 1: negativo)
        beq  $t0, $zero, F_GRIS     # apagado: saltar al siguiente filtro
        xori $v0, $v0, 0xFF         # NEGATIVO: invertir los 8 bits da 7-R, 7-G y 3-B a la vez

F_GRIS: andi $t0, $a1, 0x02         # ¿botón 1? (filtro 2: escala de grises)
        beq  $t0, $zero, F_MAPA     # apagado: saltar al siguiente filtro
        move $a0, $v0               # argumento: el píxel como va hasta ahora
        jal  BRILLO                 # $v0 = Y (0-7)
        sll  $t0, $v0, 5            # R' = Y en los bits 7-5
        sll  $t1, $v0, 2            # G' = Y en los bits 4-2
        or   $t0, $t0, $t1          # juntar R' y G'
        srl  $t1, $v0, 1            # B' = Y/2 (0-3) en los bits 1-0
        or   $v0, $t0, $t1          # píxel gris = R' | G' | B'

F_MAPA: andi $t0, $a1, 0x04         # ¿botón 2? (filtro 3: mapa de color)
        beq  $t0, $zero, F_FIN      # apagado: terminar
        andi $v0, $v0, 0xE0         # CAMBIO: dejar solo el rojo (borra G y B)

F_FIN:  lw   $ra, 0($sp)            # pop: recuperar la dirección de retorno
        addi $sp, $sp, 4            #      y liberar la palabra ($sp vuelve a 0x400)
        jr   $ra                    # volver a quien llamó, con el resultado en $v0

# =================================================================================
# BRILLO: Y = (R + 2G + 2B + 2) >> 2, sin mult ni div (solo sumas y corrimientos).
#   $a0 = píxel RGB332     ->     $v0 = Y (0 a 7)
# Función hoja: no llama a nadie, no necesita la pila. Solo modifica $t0, $t1 y $v0.
# =================================================================================
BRILLO: srl  $t0, $a0, 5            # bajar R a los bits 2-0
        andi $t0, $t0, 0x07         # $t0 = R (0-7)
        srl  $t1, $a0, 2            # bajar G a los bits 2-0
        andi $t1, $t1, 0x07         # $t1 = G (0-7)
        sll  $t1, $t1, 1            # $t1 = 2G (correr 1 a la izquierda = multiplicar por 2)
        add  $t0, $t0, $t1          # $t0 = R + 2G
        andi $t1, $a0, 0x03         # $t1 = B (0-3): ya está en los bits 1-0
        sll  $t1, $t1, 1            # $t1 = 2B (lleva el azul de 0-3 a 0-6)
        add  $t0, $t0, $t1          # $t0 = R + 2G + 2B
        addi $t0, $t0, 2            # + 2: para redondear en lugar de truncar
        srl  $v0, $t0, 2            # Y = suma / 4 (correr 2 a la derecha)
        jr   $ra                    # volver con Y en $v0

# =================================================================================
# A_16COLORES: píxel RGB332 -> color de la paleta de 16 colores (pantalla 0x400).
#   $a0 = píxel RGB332     ->     $v0 = color 0-15
#   bit 0 (rojo)   <- bit 7 del píxel (bit alto de R)
#   bit 1 (verde)  <- bit 4 del píxel (bit alto de G)
#   bit 2 (azul)   <- bit 1 del píxel (bit alto de B)
#   bit 3 (brillo) <- bit 6 OR bit 3 del píxel (segundo bit de R o de G)
# Mismo patrón para cada bit: correr hasta la posición destino, aislar con andi, juntar con or.
# =================================================================================
A_16COLORES:
        srl  $v0, $a0, 7            # el bit 7 del píxel baja al bit 0
        andi $v0, $v0, 0x01         # $v0 = rojo (bit 0)
        srl  $t0, $a0, 3            # el bit 4 del píxel baja al bit 1
        andi $t0, $t0, 0x02         # aislar el bit 1: verde
        or   $v0, $v0, $t0          # juntar
        sll  $t0, $a0, 1            # el bit 1 del píxel sube al bit 2
        andi $t0, $t0, 0x04         # aislar el bit 2: azul
        or   $v0, $v0, $t0          # juntar
        srl  $t0, $a0, 3            # el bit 6 del píxel baja al bit 3
        or   $t0, $t0, $a0          # OR con el píxel sin correr: su bit 3 ya está en el bit 3
        andi $t0, $t0, 0x08         # aislar el bit 3: brillo = bit 6 OR bit 3
        or   $v0, $v0, $t0          # juntar
        jr   $ra                    # volver con el color en $v0

# =================================================================================
# A_RGB: píxel RGB332 -> color 0x00RRGGBB = (R << 21) | (G << 13) | (B << 6).
#   $a0 = píxel RGB332     ->     $v0 = color de 24 bits
# Cada canal se aísla donde está y se corre hasta los bits altos de su byte.
# =================================================================================
A_RGB:  andi $t0, $a0, 0xE0         # R donde está: bits 7-5 (ya es R << 5)
        sll  $v0, $t0, 16           # 5 + 16 = 21: R queda en los bits 23-21 (byte RR)
        andi $t0, $a0, 0x1C         # G donde está: bits 4-2 (ya es G << 2)
        sll  $t0, $t0, 11           # 2 + 11 = 13: G queda en los bits 15-13 (byte GG)
        or   $v0, $v0, $t0          # juntar
        andi $t0, $a0, 0x03         # B donde está: bits 1-0
        sll  $t0, $t0, 6            # B queda en los bits 7-6 (byte BB)
        or   $v0, $v0, $t0          # juntar
        jr   $ra                    # volver con el color en $v0

FIN:                                # el programa termina cuando el PC llega al final de .text
