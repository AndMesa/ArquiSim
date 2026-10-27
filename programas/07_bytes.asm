# Programa 07 -- Bytes: lb, lbu y sb
#
#   lb   carga 1 byte y EXTIENDE EL SIGNO (copia el bit 7 hacia la izquierda)
#   lbu  carga 1 byte y rellena con CEROS
#   sb   guarda los 8 bits bajos de un registro
# Un carácter de texto ocupa 1 byte: en minúscula el bit 5 está en 1 ('a' = 0x61, 'A' = 0x41).
# Necesita: lb, lbu, sb (+ andi)

.data
TXT:    .asciiz "hola"              # 0x200: 'h' 'o' 'l' 'a' y un byte 0
NUM:    .byte 0x80                  # 0x205: un byte con el bit 7 en 1

.text
        addi $s0, $zero, TXT

# 1) Extensión de signo vs. extensión con ceros
        lb   $t0, NUM($zero)        # 0xFFFFFF80 (-128)
        lbu  $t1, NUM($zero)        # 0x00000080 (128)

# 2) Pasar a mayúsculas, un byte a la vez (apagar el bit 5)
        lbu  $t2, 0($s0)
        andi $t2, $t2, 0xDF
        sb   $t2, 0($s0)
        lbu  $t2, 1($s0)
        andi $t2, $t2, 0xDF
        sb   $t2, 1($s0)
        lbu  $t2, 2($s0)
        andi $t2, $t2, 0xDF
        sb   $t2, 2($s0)
        lbu  $t2, 3($s0)
        andi $t2, $t2, 0xDF
        sb   $t2, 3($s0)
