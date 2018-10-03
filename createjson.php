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
 * Usage: php createjson.php [title] [fps]
 * Prereq: video.dat already rendered, and ./tmp folder still has the last chunks
 * Don't use this file directly, it'll be called by workflow.sh
 *
 */

(PHP_SAPI !== 'cli' || isset($_SERVER['HTTP_USER_AGENT'])) && die('cli only');
 
if(!isset($_SERVER["argv"])) die("\nno args\n");
if(!isset($_SERVER["argv"][1])) die("\nnot enough args (missing 1st arg title)\n");
if(!isset($_SERVER["argv"][2])) die("\nnot enough args (missing 2nd arg fps)\n");

$jsonObj = new STDClass;
$title = $_SERVER["argv"][1];
$title = preg_replace("/[^a-z0-9-_]+/i", "", $title);
echo "\nUsing title:$title\n";

$fps = $_SERVER["argv"][2];
$fps = (int)$fps;
if($fps<=0 || $fps>100) {
  die("\nUnrealistic fps $fps\n");
}


$jsonObj->profileName = ucwords($title);
$jsonObj->videoFileName = "/vid/$title.dat";
$jsonObj->audioFileName = "/mp3/$title.mp3";
$jsonObj->thumbFileName = "/jpg/$title.mp3";

$frames = glob("frames/*");

echo "Found ".count($frames)." frames\n";

//TODO: add JPEG thumbnail
$thumb = $frames[rand(0,count($frames))];
exec("convert $thumb -resize 160x160 -background black -gravity center -extent 160x160 jpg/$title.jpg");

if(!file_exists("jpg/$title.jpg")) {
  die("\nUnable to create thumbnail\n");
}

if(!file_exists('.'.$jsonObj->videoFileName)) {
  die("\nInvalid video filename\n");
}
if(!file_exists('.'.$jsonObj->audioFileName)) {
  die("\nInvalid audio filename\n");
}

$jsonObj->videoFileSize = (string)filesize( '.'.$jsonObj->videoFileName );
$jsonObj->audioFileSize = (string)filesize( '.'.$jsonObj->audioFileName );
$jsonObj->thumbFileSize = (string)filesize( '.'.$jsonObj->videoFileName );
$jsonObj->audioSource = "0";
$jsonObj->framespeed  = "$fps";
$jsonObj->totalframes = (string)count($frames);

$chunks = glob("tmp/*");
if(count($chunks)!=4) {
  die("\nChunk count fail\n");
}

list($width, $height, $type, $attr) = getimagesize($chunks[0]);

if($width<=0 || $width>320) {
  die("\nUnrealistic width: $width\n");
}
if($height<=0 || $height>240) {
  die("\nUnrealistic height: $height\n");
}

$jsonObj->width = (string)$width;
$jsonObj->height = (string)$height;
$output = json_encode($jsonObj, JSON_UNESCAPED_SLASHES);
echo $output;

if(file_exists('playlist.json')) {
  $playlist = json_decode( file_get_contents('playlist.json'), true );
} else {
  $playlist = array();
  $playlist['base_url'] = $_ENV['BASE_URL'];
  $playlist['playlist_count'] = 0;
  $playlist['playlist'] = array();
}

$merged = false;
$index = 0;

foreach($playlist['playlist'] as $index => $item) {
  if($item['profileName']==$jsonObj->profileName) {
    $playlist['playlist'][$index] = $jsonObj;
    $merged = true;
  }
}

if($merged===false) {
  $index++;
  $playlist['playlist'][$index] = $jsonObj;
  $playlist['playlist_count']++;
}

file_put_contents("playlist.json", json_encode($playlist, JSON_UNESCAPED_SLASHES |  JSON_PRETTY_PRINT));
