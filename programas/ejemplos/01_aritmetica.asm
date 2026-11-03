# Programa 01 -- Aritmética:  f = (g + h) - (i + j)
# f -> $s0, g -> $s1, h -> $s2, i -> $s3, j -> $s4
# Necesita: sub

.text
        addi $s1, $zero, 10     # g = 10
        addi $s2, $zero, 5      # h = 5
        addi $s3, $zero, 3      # i = 3
        addi $s4, $zero, 2      # j = 2

        add  $t0, $s1, $s2      # $t0 = g + h
        add  $t1, $s3, $s4      # $t1 = i + j
        sub  $s0, $t0, $t1      # f = $t0 - $t1 = 10
        sub  $s5, $t1, $t0      # $s5 = -10
