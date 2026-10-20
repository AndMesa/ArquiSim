# Programa 13 -- Botones: entrada mapeada en memoria y sondeo (polling)
#
# Los 8 botones son los bits 7..0 de la palabra en 0x600 (1 = activado).
# El programa los LEE una y otra vez (sondeo) y copia su estado a los
# primeros 8 píxeles de la pantalla: botón activado = píxel verde.
#
# Pruébelo en la interfaz:  bp LEER  y luego  c ; active botones con  bt 3
# y continúe con  c  para ver cómo el programa los detecta.
# Necesita: lw, andi, srl, sb, beq, j

.text
        addi $s0, $zero, 0x400      # base de la pantalla
        addi $s7, $zero, 50         # vueltas del ciclo de sondeo
LEER:   beq  $s7, $zero, FIN
        lw   $t0, 0x600($zero)      # leer los 8 botones
        addi $t6, $s0, 7            # píxel del botón 0 (x = 7)
        addi $t1, $zero, 8          # bits por revisar
BIT:    beq  $t1, $zero, SIGUE
        andi $t2, $t0, 1            # bit menos significativo
        add  $t3, $zero, $zero      # color negro...
        beq  $t2, $zero, PINTAR
        addi $t3, $zero, 10         # ...o verde brillante si está activado
PINTAR: sb   $t3, 0($t6)
        srl  $t0, $t0, 1            # siguiente botón
        addi $t6, $t6, -1           # píxel a la izquierda
        addi $t1, $t1, -1
        j    BIT
SIGUE:  addi $s7, $s7, -1
        j    LEER
FIN:
