addiu	$t1, $zero, 1
addiu	$t2, $zero, 2
addu	$t3, $t2, $t1
subu	$t3, $t3, $t1
sll	$t4, $t1, 2
srl	$t5, $t4, 2
and	$t6, $t2, $t3
andi 	$t0, $t3, 66
addiu	$t7, $zero, 5
addiu	$t8, $zero, 10
or 	$t9, $t7, $t8
lui	$t1, 0x0001
ori	$s0, $t1, 0x1110
slt	$s1, $t1, $t2
slt	$s2, $t2, $t1