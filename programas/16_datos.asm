# Programa 16 -- El segmento .data: variables y arreglos en memoria
#
#   int notas[6] = {35, 42, 28, 50, 45, 39};   // notas x 10
#   int n = 6;
#   int maxima, suma;
#
#   maxima = notas[0];  suma = 0;
#   for (i = 0; i < n; i++) {
#       suma = suma + notas[i];
#       if (maxima < notas[i]) maxima = notas[i];
#   }
#
# Las variables viven en la RAM (segmento .data, desde 0x200), NO en registros:
# hay que traerlas con lw, operarlas en registros y devolver los resultados con sw.
# Cada etiqueta es una dirección: N($zero) es la palabra en la dirección de N.
# Necesita: lw, sw, slt, beq, j

.data
NOTAS:  .word 35, 42, 28, 50, 45, 39    # arreglo: 6 palabras desde 0x200
N:      .word 6                         # 0x218
MAXIMA: .word 0                         # 0x21C  (resultado)
SUMA:   .word 0                         # 0x220  (resultado)

.text
        addi $s0, $zero, NOTAS      # p = &notas[0]
        lw   $s1, N($zero)          # contador = n
        lw   $s2, 0($s0)            # maxima = notas[0]
        add  $s3, $zero, $zero      # suma = 0
CICLO:  beq  $s1, $zero, FIN
        lw   $t0, 0($s0)            # t0 = notas[i]
        add  $s3, $s3, $t0          # suma += notas[i]
        slt  $t1, $s2, $t0          # ¿maxima < notas[i]?
        beq  $t1, $zero, SIGUE      # lógica negativa: si no, saltar
        add  $s2, $t0, $zero        # maxima = notas[i]
SIGUE:  addi $s0, $s0, 4            # siguiente palabra (4 bytes)
        addi $s1, $s1, -1
        j    CICLO
FIN:    sw   $s2, MAXIMA($zero)     # guardar los resultados en .data
        sw   $s3, SUMA($zero)
