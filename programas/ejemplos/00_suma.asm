# Programa 00 -- Suma básica
# Usa solo add y addi: funciona con el proyecto base.

.text
        addi $t0, $zero, 5      # $t0 = 5
        addi $t1, $zero, 15     # $t1 = 15
        add  $s0, $t0, $t1      # $s0 = $t0 + $t1 = 20
        addi $s1, $s0, -3       # $s1 = 20 - 3 = 17
	
