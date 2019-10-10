addiu	$t1, $zero, 1
addiu	$t2, $zero, 2

test:
bne 	$t1, $t2, noteq
addiu	$t4, $zero, 2

noteq:
addiu	$t3, $zero, 6
lui	$t2, 0x0040
ori	$t2, $t2, 0x1000
sw	$t3, 4($t2)
lw	$t5, 4($t2)
bne	$t1, $t1, test
addi	$t1, $t2, 20