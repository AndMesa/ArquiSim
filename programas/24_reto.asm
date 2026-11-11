# Programa 24 -- Reto: contar los números negativos de un arreglo
#
#   int A[8] = {5, -3, 12, -8, -1, 7, -20, 4};
#   int contar(int *p, int n) {          // cuántos elementos son negativos
#       int c = 0;
#       while (n != 0) { c = c + (*p < 0); p++; n--; }
#       return c;
#   }
#   RES = contar(A, 8);
#   for (i = 0; i < RES; i++) pixel(i, 0) = rojo;
#
# Argumentos en $a0 (dirección del arreglo) y $a1 (tamaño), retorno en $v0.
# Necesita: jal, jr, lw, sw, sb, slt, beq, j, add, addi

.data
A:      .word 5, -3, 12, -8, -1, 7, -20, 4     # 0x200: 8 palabras (4 negativas)
RES:    .word 0                                # 0x220: cuántos negativos hay

.text
main:   addi $a0, $zero, A          # argumento 1: dirección del arreglo
        addi $a1, $zero, 8          # argumento 2: tamaño
        jal  CONTAR                 # $v0 = cantidad de negativos
        sw   $v0, RES($zero)        # guardar el resultado en .data

# Pintar un píxel rojo por cada negativo, en la fila 0: pixel(i, 0) = 0x400 + i
        addi $t0, $zero, 0x400      # puntero al píxel (0, 0)
        add  $t1, $zero, $zero      # i = 0
        addi $t2, $zero, 1          # color rojo
PINTAR: beq  $t1, $v0, FIN          # ¿ya se pintaron todos?
        sb   $t2, 0($t0)            # pintar el píxel
        addi $t0, $t0, 1            # siguiente píxel (1 byte por píxel)
        addi $t1, $t1, 1            # i++
        j    PINTAR

# ---------------- int contar(int *p, int n) ----------------
CONTAR: add  $v0, $zero, $zero      # c = 0
CICLO:  beq  $a1, $zero, VOLVER     # si no quedan elementos, terminar
        lw   $t3, 0($a0)            # $t3 = *p
        slt  $t4, $t3, $zero        # $t4 = 1 si el elemento es negativo, 0 si no
        add  $v0, $v0, $t4          # c = c + $t4
        addi $a0, $a0, 4            # p++ (siguiente palabra: 4 bytes)
        addi $a1, $a1, -1           # n--
        j    CICLO
VOLVER: jr   $ra                    # volver a quien llamó
FIN:
