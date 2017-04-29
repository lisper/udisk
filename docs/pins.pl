#!/usr/bin/perl

$h = -(50 * 2.54);
$v = (116 * 2.54) / 2;
$rot = 0;
$c = 0;
for ($i = 0; $i < 116; $i++) {
    if ($c++ > 116/4) {
	$c = 0;
	$rot += 90;
	$h += 2.54 * 50;
    }

    $signal = "IO$i";
    $v -= 2.54;
    if ($i != 2 && $i != 16 && $i != 73 && $i != 87) {
	print "Pin \'$signal\' I/O None Short R$rot Both 0 ($h $v)\n";
    }
}
