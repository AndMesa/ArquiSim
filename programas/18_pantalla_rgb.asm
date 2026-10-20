# Programa 18 -- Pantalla RGB con registros de comando
#
# Esta pantalla NO ocupa memoria del procesador: tiene su propia memoria interna.
# El procesador le habla con solo dos registros de E/S:
#   0x610 POSICION   (y << 16) | x
#   0x614 COLOR      0x00RRGGBB: escribirlo dibuja el píxel en POSICION y avanza x
#                    (autoincremento: al final de la fila pasa a la siguiente)
#
# Compare con la pantalla de 0x400 (programa 09): allá cada píxel es un byte de
# memoria; aquí un píxel es un COMANDO enviado al dispositivo.
#
# Dibuja un degradado de 32 x 32:  rojo = 8*x,  verde = 8*y,  azul = 0x80
# Necesita: sw, sll, or, ori, beq, j

.text
        sw   $zero, 0x610($zero)    # POSICION = (0, 0): una sola vez
        add  $s1, $zero, $zero      # y = 0
        addi $s3, $zero, 32         # alto
        addi $s4, $zero, 32         # ancho
FILA:   beq  $s1, $s3, FIN
        add  $s0, $zero, $zero      # x = 0
COL:    beq  $s0, $s4, SIGFILA
        sll  $t0, $s0, 19           # rojo  = 8*x en los bits 23..16  (x << 3 << 16)
        sll  $t1, $s1, 11           # verde = 8*y en los bits 15..8   (y << 3 << 8)
        or   $t0, $t0, $t1
        ori  $t0, $t0, 0x80         # azul  = 0x80
        sw   $t0, 0x614($zero)      # COLOR: dibuja y avanza (autoincremento)
        addi $s0, $s0, 1
        j    COL
SIGFILA:
        addi $s1, $s1, 1
        j    FILA
FIN:
