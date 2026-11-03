# Programa 03 -- or, xor y xori: unir, comparar e invertir bits
#
#   or   1 si ALGUNO de los dos bits es 1      (unir)
#   xor  1 si los dos bits son DISTINTOS       (comparar / invertir)
#   xori xor con una constante (extendida con ceros)
# Necesita: or, xor, xori

.text
        addi $t0, $zero, 0x0F0      # 0000 1111 0000
        addi $t1, $zero, 0x0FF      # 0000 1111 1111
        or   $s0, $t0, $t1          # 0x0FF: bits de uno u otro
        xor  $s1, $t0, $t1          # 0x00F: bits distintos
        xori $s2, $t0, 0xFFF        # 0xF0F: invierte los 12 bits bajos
        xor  $s3, $t0, $t0          # 0: x XOR x siempre es 0

# Intercambiar $t0 y $t1 sin un registro auxiliar
        xor  $t0, $t0, $t1
        xor  $t1, $t0, $t1
        xor  $t0, $t0, $t1          # ahora $t0 = 0x0FF y $t1 = 0x0F0
