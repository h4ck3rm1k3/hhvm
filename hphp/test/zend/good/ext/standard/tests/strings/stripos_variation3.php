<?php
/* Prototype  : int stripos ( string $haystack, string $needle [, int $offset] );
 * Description: Find position of first occurrence of a case-insensitive string
 * Source code: ext/standard/string.c
*/

/* Test stripos() function by passing multi-line heredoc string for haystack and 
 *  with various needles & offsets
*/

echo "*** Testing stripos() function: with heredoc strings ***\n";
echo "-- With heredoc string containing multi lines --\n";
$multi_line_str = <<<EOD
Example of string
spanning multiple lines
using heredoc syntax.
EOD;
var_dump( stripos($multi_line_str, "ing", 0) );
var_dump( stripos($multi_line_str, "ing", 15) );
var_dump( stripos($multi_line_str, "ing", 22) );
var_dump( stripos($multi_line_str, "") );
var_dump( stripos($multi_line_str, " ") );

echo "*** Done ***";
?>
