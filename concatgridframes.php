<?php
/*
 * 
 * M5Tube : video player for M5Stack with conversion support
 * as seen on https://www.youtube.com/watch?v=ccOSyEdZdiw
 * https://github.com/tobozo/M5Tube
 *
 * Copyright (2018) tobozo
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files ("M5Stack SD Updater"), to deal in the Software without 
 * restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following 
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 *
 * Usage: php concatgridframes.php
 * Prereq: ./frames/ subfolder is full of jpeg frames
 * Don't use this file directly, it'll be called by workflow.sh
 *
 */

(PHP_SAPI !== 'cli' || isset($_SERVER['HTTP_USER_AGENT'])) && die('cli only');
 
$maxChunkSize = 1024; // TODO: dynamyze this
$totalSize = 0;
$totalChunks = 0;
@mkdir("tmp");
$frames = glob("frames/*");
$vid = fopen("video.dat", "w");
$gridpos = 0;
$max = count($frames);
$warns = array();

foreach($frames as $frame) {
  exec("rm -f tmp/*");
  $frameNum = preg_replace("/[^0-9]+/", "", $frame);
  $i = $gridpos%4;
  $avgQuality = 80;
  $acceptable = false;
  
  while(!$acceptable) {
    exec("convert $frame -crop 50%x50% +repage -quality $avgQuality tmp/$frameNum.%d.jpg");
    $acceptable = true;
    $chunkSize = filesize("tmp/$frameNum.$i.jpg");
    if( $chunkSize > $maxChunkSize) {
      $avgQuality -= 10;
      $acceptable = false;
      if($avgQuality < 10) {
        $avgQuality = 10;
        $acceptable = true;
        $warns[] = "$frame exceeds max $maxChunkSize : $chunkSize";
      }
    }
  }
  
  $content = file_get_contents("tmp/$frameNum.$i.jpg");
  $size = strlen($content);
  $totalChunks++;
  $totalSize += $size;
  // int to 16 bits
  $int1 = $size & 65535;
  $int2 = ($size>>16) & 65535;
  // 16 to 8 bits
  $int3 = $int1 & 255;
  $int4 = ($int1>>8) & 255;
  fwrite($vid, chr($int3)); // block size MSB
  fwrite($vid, chr($int4)); // block size LSB
  fwrite($vid, $content, $size);
  $progress = round( ($gridpos*100) / $max );
  echo "[$avgQuality][$progress%][($size, $int1, $int2, $int3, $int4)] ".$frame."\n";
  $gridpos++;
}

echo "Avg chunk size : ".floor( $totalSize / $totalChunks )."\n";
