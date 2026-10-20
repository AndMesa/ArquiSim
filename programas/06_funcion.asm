# Programa 06 -- Llamada a función
#   int sumar3(int x) { return x + 3; }
#   s0 = sumar3(10);
# Argumento en $a0, retorno en $v0, dirección de retorno en $ra
# Necesita: jal, jr, j

.text
main:   addi $a0, $zero, 10         # argumento = 10
        jal  SUMAR3                 # $ra = PC + 4; saltar a SUMAR3
        add  $s0, $v0, $zero        # s0 = valor de retorno
        j    FIN

SUMAR3: addi $v0, $a0, 3            # $v0 = x + 3
        jr   $ra                    # volver a quien llamó
FIN:
