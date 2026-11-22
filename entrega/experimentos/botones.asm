# Pregunta 4: los botones (0x600) son de solo lectura
# En la interfaz: active antes algunos botones (por ejemplo  bt 0  y  bt 2) y ejecute paso a paso.
.text
        lw   $t0, 0x600($zero)      # $t0 = botones antes
        addi $t1, $zero, 0xFF       # valor que se intenta escribir
        sw   $t1, 0x600($zero)      # intento de escribir una palabra en los botones
        sb   $t1, 0x603($zero)      # intento de escribir solo el byte bajo
        lw   $t2, 0x600($zero)      # $t2 = botones después: igual que $t0
