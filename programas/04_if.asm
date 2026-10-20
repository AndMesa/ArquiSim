# Programa 04 -- if-else con lógica negativa
#   if (i == j) f = g + h;  else f = g - h;
# f -> $s0, g -> $s1, h -> $s2, i -> $s3, j -> $s4
# Necesita: bne, j, sub

.text
        addi $s1, $zero, 10         # g = 10
        addi $s2, $zero, 3          # h = 3
        addi $s3, $zero, 7          # i = 7
        addi $s4, $zero, 7          # j = 7   (pruebe cambiándolo)

        bne  $s3, $s4, ELSE         # si i != j, saltar al else
        add  $s0, $s1, $s2          # f = g + h
        j    FIN                    # saltar el else
ELSE:   sub  $s0, $s1, $s2          # f = g - h
FIN:
