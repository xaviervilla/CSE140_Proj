.text
	lui	$t0,	0x1001
	ori	$t0,	0x0000
	addiu	$t1,	$zero, 1
	sw	$t1,	0($t0)
	addiu	$t1,	$zero, 2
	sw	$t1,	4($t0)
	addiu	$t1,	$zero, 3
	sw	$t1,	8($t0)
	addiu	$t1,	$zero, 4
	sw	$t1,	12($t0)
	addiu	$t1,	$zero, 5
	sw	$t1,	16($t0)