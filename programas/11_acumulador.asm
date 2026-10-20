# Programa 11 -- Máquina de acumulador:  f = (g + h) - (i + j)
# Igual que el programa 01, pero cada operación tiene UNA sola dirección:
# el otro operando y el resultado están siempre en el acumulador ACC.
# f -> $s0, g -> $s1, h -> $s2, i -> $s3, j -> $s4
# Necesita: mtacc, mfacc, add/sub de 1 dirección (adda, suba), addi de 1 dirección (addia)

.text
        addi  $s1, $zero, 10        # g = 10
        addi  $s2, $zero, 5         # h = 5
        addi  $s3, $zero, 3         # i = 3
        addi  $s4, $zero, 2         # j = 2

        mtacc $s1                   # ACC = g
        add   $s2                   # ACC = g + h
        sub   $s3                   # ACC = g + h - i
        sub   $s4                   # ACC = g + h - i - j = 10
        mfacc $s0                   # f = ACC

        addi  100                   # ACC = ACC + 100 = 110
