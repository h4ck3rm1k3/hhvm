<?php

$i = 0;
 print ++$i;
 print "\t";
 print (array('b' => 1)!==true) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==true) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = true;
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== true	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==false) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==false) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = false;
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== false	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==1) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==1) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = 1;
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== 1	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==0) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==0) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = 0;
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== 0	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==-1) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==-1) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = -1;
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== -1	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!=='1') ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !=='1') ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = '1';
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== '1'	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!=='0') ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !=='0') ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = '0';
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== '0'	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!=='-1') ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !=='-1') ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = '-1';
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== '-1'	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==null) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==null) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = null;
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== null	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==array()) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==array()) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = array();
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== array()	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==array(1)) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==array(1)) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = array(1);
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== array(1)	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==array(2)) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==array(2)) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = array(2);
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== array(2)	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==array('1')) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==array('1')) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = array('1');
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== array('1')	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==array('0' => '1')) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==array('0' => '1')) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = array('0' => '1');
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== array('0' => '1')	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==array('a')) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==array('a')) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = array('a');
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== array('a')	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==array('a' => 1)) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==array('a' => 1)) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = array('a' => 1);
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== array('a' => 1)	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==array('b' => 1)) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==array('b' => 1)) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = array('b' => 1);
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== array('b' => 1)	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==array('a' => 1, 'b' => 2)) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==array('a' => 1, 'b' => 2)) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = array('a' => 1, 'b' => 2);
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== array('a' => 1, 'b' => 2)	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==array(array('a' => 1))) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==array(array('a' => 1))) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = array(array('a' => 1));
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== array(array('a' => 1))	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!==array(array('b' => 1))) ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !==array(array('b' => 1))) ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = array(array('b' => 1));
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== array(array('b' => 1))	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!=='php') ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !=='php') ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = 'php';
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== 'php'	";
 print "\n";
 print ++$i;
 print "\t";
 print (array('b' => 1)!=='') ? 'Y' : 'N';
 $a = 1;
 $a = 't';
 $a = array('b' => 1);
 print ($a !=='') ? 'Y' : 'N';
 $b = 1;
 $b = 't';
 $b = '';
 print (array('b' => 1)!==$b) ? 'Y' : 'N';
 print ($a !==$b) ? 'Y' : 'N';
 print "\t";
 print "array('b' => 1) !== ''	";
 print "\n";

