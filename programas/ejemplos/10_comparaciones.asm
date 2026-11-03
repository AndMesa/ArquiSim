# Programa 10 -- Comparar con y sin signo: slt, sltu y slti
#
# El mismo patrón de bits 0xFFFFFFFF es -1 con signo, pero el número más
# grande posible sin signo. Por eso slt y sltu pueden dar respuestas distintas.
# Necesita: sltu, slti, beq (+ slt)

.text
        addi $t0, $zero, -1         # 0xFFFFFFFF
        addi $t1, $zero, 1
        slt  $s0, $t0, $t1          # con signo:   -1 < 1            -> 1
        sltu $s1, $t0, $t1          # sin signo:   0xFFFFFFFF < 1    -> 0
        slti $s2, $t1, 5            # 1 < 5                          -> 1
        slti $s3, $t0, -5           # -1 < -5                        -> 0

# Contar cuántas comparaciones dieron 1 (beq salta las que dieron 0)
        add  $s4, $zero, $zero
        beq  $s0, $zero, C1
        addi $s4, $s4, 1
C1:     beq  $s1, $zero, C2
        addi $s4, $s4, 1
C2:     beq  $s2, $zero, C3
        addi $s4, $s4, 1
C3:     beq  $s3, $zero, FIN
        addi $s4, $s4, 1
FIN:
