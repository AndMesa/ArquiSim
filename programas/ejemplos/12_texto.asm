# Programa 12 -- Texto: convertir "hola" a mayúsculas
# Cada letra ocupa 1 byte; en minúscula el bit 5 está en 1 ('a' = 0x61, 'A' = 0x41).
# Necesita: lbu, andi, sb, beq

.data
TXT:    .word 0x686F6C61            # 'h' 'o' 'l' 'a'  (big-endian: 'h' en la dirección menor)

.text
        addi $s0, $zero, TXT        # puntero al primer carácter
        addi $s1, $s0, 4            # fin del texto
LOOP:   beq  $s0, $s1, FIN
        lbu  $t0, 0($s0)            # cargar un carácter (1 byte)
        andi $t0, $t0, 0xDF         # apagar el bit 5 -> mayúscula
        sb   $t0, 0($s0)            # guardar el carácter
        addi $s0, $s0, 1            # siguiente byte (de 1 en 1)
        j    LOOP
FIN:
