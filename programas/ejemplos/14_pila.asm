# Programa 14 -- La pila: una función que llama a otra
#
#   int doble(int x)          { return x + x; }
#   int doble_mas_uno(int x)  { return doble(x) + 1; }
#   s0 = doble_mas_uno(20);
#
# doble_mas_uno usa jal, que SOBRESCRIBE $ra. Por eso, antes de llamar a doble,
# guarda su propio $ra en la pila (push) y lo recupera al final (pop).
# Observe la ventana PILA: $sp baja a 0x3FC y allí queda guardado $ra.
# Necesita: jal, jr, sw, lw, addi, add, j

.text
main:   addi $a0, $zero, 20         # argumento
        jal  DOBLE_MAS_UNO
        add  $s0, $v0, $zero        # s0 = 41
        j    FIN

DOBLE_MAS_UNO:
        addi $sp, $sp, -4           # push: reservar una palabra en la pila
        sw   $ra, 0($sp)            #       y guardar la dirección de retorno
        jal  DOBLE                  # esta llamada cambia $ra
        addi $v0, $v0, 1
        lw   $ra, 0($sp)            # pop: recuperar la dirección de retorno
        addi $sp, $sp, 4            #      y liberar la palabra
        jr   $ra

DOBLE:  add  $v0, $a0, $a0          # v0 = x + x
        jr   $ra
FIN:
