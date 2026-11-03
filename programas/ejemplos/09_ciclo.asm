# Programa 09 -- Ciclo que recorre un arreglo
#   suma = 0;
#   for (i = 3; i >= 0; i--) suma = suma + A[i];
# Base de A -> $s0, suma -> $s1, puntero -> $t0
# Necesita: slt, lw, bne, j

.data
A:      .word 1, 2, 3, 4

.text
        addi $s0, $zero, A          # $s0 = &A[0]
        add  $s1, $zero, $zero      # suma = 0
        addi $t0, $s0, 12           # p = &A[3]  (3 x 4 bytes)
LOOP:   slt  $t1, $t0, $s0          # ¿p < &A[0]?
        bne  $t1, $zero, FIN        # si ya pasamos A[0], salir
        lw   $t2, 0($t0)            # $t2 = A[i]
        add  $s1, $s1, $t2          # suma += A[i]
        addi $t0, $t0, -4           # p-- (palabra anterior)
        j    LOOP                   # volver a evaluar la condición
FIN:
