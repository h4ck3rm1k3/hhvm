<?php
/*
 Prototype: string fgets ( resource $handle [, int $length] );
 Description: Gets a line from file pointer
*/

// include the file.inc for common test funcitons
include ("file.inc");

$file_modes = array("w+", "w+b", "w+t",
                    "a+", "a+b", "a+t",
                    "x+", "x+b", "x+t"); 

$file_content_types = array("numeric", "text", "text_with_new_line", "alphanumeric");

echo "*** Testing fgets() : usage variations ***\n";

$filename = dirname(__FILE__)."/fgets_variation4.tmp";

foreach($file_modes as $file_mode) {
  echo "\n-- Testing fgets() with file opened using mode $file_mode --\n";

  foreach($file_content_types as $file_content_type) {
    echo "-- File content type : $file_content_type --\n";

    /* create files with $file_content_type */
    $file_handle = fopen($filename, $file_mode);
    $data = fill_file($file_handle, $file_content_type, 50);

    if ( !$file_handle ) {
      echo "Error: failed to open file $filename!";
      exit();
    }

    echo "-- fgets() with file pointer pointing at EOF --\n";
    // seek to end of the file and try fgets()
    var_dump( fseek($file_handle, 0, SEEK_END) ); // set file pointer to eof
    var_dump( ftell($file_handle) );  // ensure that file pointer is at eof
    var_dump( feof($file_handle) );  // expected false

    var_dump( fgets($file_handle) ); // try n read a line, none expected
    var_dump( ftell($file_handle) ); // file pointer position
    var_dump( feof($file_handle) ); // ensure thta file pointer is at eof

    //close file
    fclose($file_handle);

    // delete file
    delete_file($filename);
  } // file_content_type loop
} // file_mode loop

echo "Done\n";
?>
