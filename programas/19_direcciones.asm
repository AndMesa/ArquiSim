# Programa 19 -- Instrucciones de 3, 2 y 1 dirección
# Calcula 5 + 7 + 9 de tres maneras. Mire en el IR que el binario
# siempre tiene los tres campos rd, rs y rt: solo cambia la sintaxis.
# Necesita: formas de 2 direcciones (add rd+rs,rt y addi rt+rs,imm en isa.txt) y add de 1 dirección (adda, usa ACC)

.text
        addi $t1, $zero, 5
        addi $t2, $zero, 7
        addi $t3, $zero, 9

# 3 direcciones: destino y dos fuentes explícitas
        add  $s0, $t1, $t2          # $s0 = 5 + 7
        add  $s0, $s0, $t3          # $s0 = 12 + 9 = 21

# 2 direcciones: el primer operando es destino y fuente
        add  $s1, $zero, $t1        # $s1 = 5
        add  $s1, $t2               # $s1 = $s1 + 7
        add  $s1, $t3               # $s1 = $s1 + 9 = 21
        addi $s1, 100               # $s1 = $s1 + 100 = 121

# 1 dirección: el acumulador ACC es un operando implícito (empieza en 0)
        add  $t1                    # ACC = ACC + 5
        add  $t2                    # ACC = ACC + 7
        add  $t3                    # ACC = ACC + 9 = 21
