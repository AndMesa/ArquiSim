# Programa 16 -- Pantalla mapeada en memoria (E/S)
#
# La pantalla de 32 x 16 píxeles ocupa 512 bytes a partir de 0x400: un byte por píxel.
#     dirección del píxel (x, y) = 0x400 + 32*y + x
# El procesador no "sabe" que hay una pantalla: solo escribe bytes con sb.
#
# Colores: 0 negro  1 rojo  2 verde  3 amarillo  4 azul  5 magenta  6 cian  7 blanco
#          8-15: las mismas versiones brillantes (9 = rojo brillante, 12 = azul brillante, ...)
# Necesita: sb, sll, beq, j

.text
        addi $s0, $zero, 0x400      # base de la pantalla
        addi $s1, $zero, 32         # ancho
        addi $s2, $zero, 16         # alto
        addi $t9, $zero, 12         # color del marco: azul brillante

# 1) bordes superior (y = 0) e inferior (y = 15)
        add  $t0, $zero, $zero      # x = 0
HORIZ:  beq  $t0, $s1, VERT
        add  $t1, $s0, $t0          # &pixel(x, 0)
        sb   $t9, 0($t1)
        sb   $t9, 480($t1)          # &pixel(x, 15) = &pixel(x, 0) + 15*32
        addi $t0, $t0, 1
        j    HORIZ

# 2) bordes izquierdo (x = 0) y derecho (x = 31)
VERT:   add  $t0, $zero, $zero      # y = 0
VLOOP:  beq  $t0, $s2, DIAG
        sll  $t1, $t0, 5            # 32*y  (desplazar 5 = multiplicar por 32)
        add  $t1, $s0, $t1          # &pixel(0, y)
        sb   $t9, 0($t1)
        sb   $t9, 31($t1)           # &pixel(31, y)
        addi $t0, $t0, 1
        j    VLOOP

# 3) diagonal roja: pixel(2y, y) para y = 1 .. 14
DIAG:   addi $t8, $zero, 9          # rojo brillante
        addi $t0, $zero, 1          # y = 1
        addi $t3, $zero, 15
DLOOP:  beq  $t0, $t3, FIN
        sll  $t1, $t0, 5            # 32*y
        sll  $t2, $t0, 1            # x = 2y
        add  $t1, $t1, $t2          # 32*y + x
        add  $t1, $s0, $t1          # 0x400 + 32*y + x
        sb   $t8, 0($t1)
        addi $t0, $t0, 1
        j    DLOOP
FIN:
