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
 * This build requires an external I2S module to work without
 * producing visual or audio lag.
 * 
 * Prereqs:
 *   - create "/mp3", "/vid" and "/json" folders on the MicroSD Card
 *   - install docker and start the provided docker image
 *   - run the docker script 'workflow.sh' to get your videos converted
 *   - when conversion is complete, choose between:
 *     1) having the container's working dir exposed by a web server
 *     2) copying the contents of /vid, /mp3 and /json folders from the 
 *        container's working dir to your MicroSD Card
 * 
 *   If using 1), some changes may be required in the code (such
 *   as updating the certificate chain and changing PLAYLIST_URL 
 *   and PLAYLIST_PATH.
 * 
 * Conversion settings:
 *   The docker utily provided with this projects makes the process
 *   of converting the video to an acceptable format for the M5Stack
 *   less painful.
 *   The mp4 format is required for the source file and audio is 
 *   mandatory for the conversion to complete.
 *   Audio is converted & recompressed to mp3 mono @32Kbps/22Khz
 *   Video is split into frames, each frame is split into 4 chunks, 
 *   then chunks are encoded as JPEG with variable compression rate.
 *   Using the Docker image, custom conversion settings are available
 *   such as video width and processed chunks per second.
 * 
 * 
 */


#include <M5Stack.h>               // https://github.com/m5stack/M5Stack/
#include "M5StackUpdater.h"        // https://github.com/tobozo/M5Stack-SD-Updater
#include <M5StackSAM.h>            // https://github.com/tomsuch/M5StackSAM
#include "certificates.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>           // https://github.com/bblanchon/ArduinoJson/
#include "AudioFileSourceSPIFFS.h" // https://github.com/earlephilhower/ESP8266Audio/
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

bool playing = true;
unsigned long currentMillis;
unsigned long droppedFrames = 0;
uint8_t playListSize = 1;
uint8_t videoNum = 0; // video index in playlist
const String PLAYLIST_URL = "https://phpsecu.re/m5stack/av-player/playlist.json";
const String PLAYLIST_PATH = "/json/playlist.json";

uint16_t pic[176][144];
uint16_t blockSize[2];
uint8_t* lsb[1], msb[1];
bool toggle = false;

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *spiffsfile;
AudioFileSourceSD *mp3file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;
M5SAM M5Menu;
HTTPClient http;
SDUpdater sdUpdater;

struct Video {
  String profileName = "";
  String videoFileName = "";
  String audioFileName = "";
  String thumbFileName = ""; // 160x160 JPEG file
  uint8_t audioSource = 0; // 0 = SD, 1 = SPIFFS, 2 = PROGMEM
  uint8_t videoSource = 0; // 0 = SD, 1 = SPIFFS, 2 = PROGMEM
  int videoFileSize = 0;
  int audioFileSize = 0;
  int thumbFileSize = 0;
  float framespeed = 30; // fps when the video was splitted
  int totalframes = 0; // 1516; // cyriak = 1516, frozen = 3822, chem = 2222
  uint16_t width;
  uint16_t height;
};

Video Playlist[16];


void debugVideo(Video tmpVideo) {
  Serial.println("[profileName]" + String(tmpVideo.profileName));
  Serial.println("[thumbFileName]" + String(tmpVideo.thumbFileName));
  Serial.println("[videoFileName]" + String(tmpVideo.videoFileName));
  Serial.println("[audioFileName]" + String(tmpVideo.audioFileName));
  Serial.println("[audioSource]" + String(tmpVideo.audioSource));
  Serial.println("[videoSource]" + String(tmpVideo.videoSource));
  Serial.println("[audioFileSize]" + String(tmpVideo.audioFileSize));
  Serial.println("[videoFileSize]" + String(tmpVideo.videoFileSize));
  Serial.println("[thumbFileSize]" + String(tmpVideo.thumbFileSize));
  Serial.println("[framespeed]" + String(tmpVideo.framespeed));
  Serial.println("[totalframes]" + String(tmpVideo.totalframes));
  Serial.println("[width]" + String(tmpVideo.width));
  Serial.println("[height]" + String(tmpVideo.height));
}


char* strToChar(String str) {
  int len = str.length() + 1;
  char* buf = new char[len];
  strcpy(buf, str.c_str());
  return buf;
}

static inline void fps(const int seconds){
  // Create static variables so that the code and variables can
  // all be declared inside a function
  static unsigned long lastMillis;
  static unsigned long frameCount;
  static unsigned int framesPerSecond;
  
  // It is best if we declare millis() only once
  unsigned long now = millis();
  frameCount ++;
  if (now - lastMillis >= seconds * 1000) {
    framesPerSecond = frameCount / seconds;
    M5.Lcd.setCursor(0,0);
    M5.Lcd.print(framesPerSecond);
    M5.Lcd.print(" FPS ---- ");
    M5.Lcd.print( String(droppedFrames) + " frames dropped");
    //Serial.println(framesPerSecond);
    frameCount = 0;
    lastMillis = now;
  }
}


void wget (String bin_url, String appName, const char* &ca) {
  //M5.Lcd.setCursor(0,0);
  //M5.Lcd.print(appName);
  Serial.println("Will download " + bin_url + " and save to SD as " + appName);
  http.begin(bin_url, ca);
  
  int httpCode = http.GET();
  if(httpCode <= 0) {
    Serial.println("[HTTP] GET... failed");
    http.end();
    //M5.Lcd.setCursor(0,0);
    //M5.Lcd.print("HTTP Get Failed");
    return;
  }
  if(httpCode != HTTP_CODE_OK) {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str()); 
    http.end();
    //M5.Lcd.setCursor(0,0);
    //M5.Lcd.print("HTTP Get Failed");
    return;
  }

  int len = http.getSize();
  if(len<=0) {
    Serial.println("Failed to read " + bin_url + " content is empty, aborting");
    http.end();
    //M5.Lcd.setCursor(0,0);
    //M5.Lcd.print("HTTP Get Failed");
    return;
  }
  int httpSize = len;
  uint8_t buff[512] = { 0 };
  WiFiClient * stream = http.getStreamPtr();

  File myFile = SD.open(appName, FILE_WRITE);
  if(!myFile) {
    Serial.println("Failed to open " + appName + " for writing, aborting");
    http.end();
    myFile.close();
    //M5.Lcd.setCursor(0,0);
    //M5.Lcd.print("SD Write Failed");
    return;
  }

  //M5.Lcd.setCursor(0,0);
  //M5.Lcd.print("HTTP Get Failed");

  while(http.connected() && (len > 0 || len == -1)) {
    sdUpdater.M5SDMenuProgress((httpSize-len)/10, httpSize/10);
    // get available data size
    size_t size = stream->available();
    if(size) {
      // read up to 128 byte
      int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
      // write it to SD
      myFile.write(buff, c);
      //Serial.write(buff, c);
      if(len-c >= 0) {
        len -= c;
      }
    }
    delay(1);
  }
  myFile.close();
  Serial.println("Copy done...");
  http.end();
}

bool loadPlaylist(bool forceDownload = false) {

  File file = SD.open(PLAYLIST_PATH);

  if(!file) {
    Serial.println("Unable to open playlist file " + PLAYLIST_PATH + ", aborting");
    return false;
  }
  
#if ARDUINOJSON_VERSION_MAJOR==6
  DynamicJsonDocument jsonBuffer;
  DeserializationError error = deserializeJson(jsonBuffer, file);
  if (error) {
    Serial.println("JSON Deserialization failed");
    return false;
  }
  JsonObject root = jsonBuffer.as<JsonObject>();
  if (!root.isNull())
#else
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(file);
  if (root.success())
#endif
  {

    playListSize = root["playlist_count"].as<uint16_t>();

    if(playListSize==0) {
      Serial.println("playlist empty!");
      return false;
    }
  
    String base_url = root["base_url"].as<String>();

    M5Menu.clearList();
    M5Menu.setListCaption("M5Tube");
  
    for(uint16_t i=0;i<playListSize;i++) {
  
      Playlist[i].profileName   = root["playlist"][i]["profileName"].as<String>();
      Playlist[i].thumbFileName = root["playlist"][i]["thumbFileName"].as<String>();
      Playlist[i].videoFileName = root["playlist"][i]["videoFileName"].as<String>();
      Playlist[i].audioFileName = root["playlist"][i]["audioFileName"].as<String>();
      Playlist[i].audioSource   = root["playlist"][i]["audioSource"].as<uint8_t>(); // 0 = SD, 1 = SPIFFS, 2 = PROGMEM
      Playlist[i].videoSource   = root["playlist"][i]["videoSource"].as<uint8_t>(); // 0 = SD, 1 = SPIFFS, 2 = PROGMEM
      Playlist[i].audioFileSize = root["playlist"][i]["audioFileSize"].as<int>();
      Playlist[i].videoFileSize = root["playlist"][i]["videoFileSize"].as<int>();
      Playlist[i].thumbFileSize = root["playlist"][i]["thumbFileSize"].as<int>();
      Playlist[i].framespeed    = root["playlist"][i]["framespeed"].as<float>(); // fps when the video was splitted
      Playlist[i].totalframes   = root["playlist"][i]["totalframes"].as<int>(); // 1516; // cyriak = 1516, frozen = 3822, chem = 2222
      Playlist[i].width         = root["playlist"][i]["width"].as<uint16_t>();
      Playlist[i].height        = root["playlist"][i]["height"].as<uint16_t>();
  
      debugVideo( Playlist[i] );
      
      if(SD.exists(Playlist[i].videoFileName.c_str())) {
        File tmpFile = SD.open(Playlist[i].videoFileName.c_str());
        if(tmpFile.size() != Playlist[i].videoFileSize) {
          tmpFile.close();
          Serial.println("Video file changed " + Playlist[i].videoFileName);
          wget (base_url + Playlist[i].videoFileName, Playlist[i].videoFileName, phpsecure_ca);
        } else {
          Serial.println("Video file already exists " + Playlist[i].audioFileName);
        }
      } else {
        if(forceDownload) {
          wget (base_url + Playlist[i].videoFileName, Playlist[i].videoFileName, phpsecure_ca);
        }
      }
      if(SD.exists(Playlist[i].audioFileName.c_str())) {
        File tmpFile = SD.open(Playlist[i].audioFileName.c_str());
        if(tmpFile.size() != Playlist[i].audioFileSize) {
          tmpFile.close();
          Serial.println("Audio file changed " + Playlist[i].audioFileName);
          wget (base_url + Playlist[i].audioFileName, Playlist[i].audioFileName, phpsecure_ca);
        } else {
          Serial.println("Audio file already exists " + Playlist[i].audioFileName);
        }
      } else {
        if(forceDownload) {
          wget (base_url + Playlist[i].audioFileName, Playlist[i].audioFileName, phpsecure_ca);
        }
      }
      if(SD.exists(Playlist[i].thumbFileName.c_str())) {
        File tmpFile = SD.open(Playlist[i].thumbFileName.c_str());
        if(tmpFile.size() != Playlist[i].thumbFileSize) {
          tmpFile.close();
          Serial.println("Thumb file changed " + Playlist[i].thumbFileName);
          wget (base_url + Playlist[i].thumbFileName, Playlist[i].thumbFileName, phpsecure_ca);
        } else {
          Serial.println("Thumb file already exists " + Playlist[i].thumbFileName);
        }
      } else {
        if(forceDownload) {
          wget (base_url + Playlist[i].thumbFileName, Playlist[i].thumbFileName, phpsecure_ca);
        }
      }
      M5Menu.addList(Playlist[i].profileName);

    }
    
    M5Menu.drawAppMenu("Video Player", "Stop", "Play", "Next");
    M5Menu.addList("Download"); // MenuID = M5Menu.getListID();
    M5Menu.setListID(0);
    M5Menu.showList();

    return true;
    
  } else {
    Serial.println("playlist not found or invalid json");
    return false;
  }
  
}


void getPlaylist() {
  if((WiFi.status() != WL_CONNECTED)) {
    Serial.println("Enabling WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(); // set SSID/PASS from another app (i.e. WiFi Manager) and reload this app
    unsigned long startup = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Establishing connection to WiFi..");
      if(startup + 10000 < millis()) {
        Serial.println("Resetting WiFi");
        WiFi.mode(WIFI_MODE_NULL);
        delay(150);
        WiFi.mode(WIFI_STA);
        WiFi.begin(); // set SSID/PASS from another app (i.e. WiFi Manager) and reload this app
        startup = millis();
      }
    }
  }
  
  wget (PLAYLIST_URL, PLAYLIST_PATH, phpsecure_ca);

  loadPlaylist(true);

}




void playVideo(Video &video) {
  switch(video.audioSource) {
    case 0:
      mp3file = new AudioFileSourceSD(video.audioFileName.c_str());
      id3 = new AudioFileSourceID3(mp3file);
    break;
    case 1:
      spiffsfile = new AudioFileSourceSPIFFS(video.audioFileName.c_str());
      id3 = new AudioFileSourceID3(spiffsfile);
    break;
    case 2:

    break;
    default:
      Serial.println("No source for audio, aborting");
      return;
    break;
  }

  out = new AudioOutputI2S(0, 0);
  out->SetPinout(26, 25, 22);
  out->SetOutputModeMono(true);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(id3, out);

  File file = SD.open(video.videoFileName);
  if(!file) {
    Serial.println("Unable to open video file " + video.videoFileName + ", aborting");
    return;
  }

  float framespeed = video.framespeed;
  int totalframes = video.totalframes; // 1516; // cyriak = 1516, frozen = 3822, chem = 2222
  const unsigned long started_at = millis();
  unsigned long framenumber = 0;
  unsigned long framelength = 1000/framespeed; // 200ms = expecting 5fps, 220ms = expecting 4.54fps
  int fileSize = id3->getSize();
  Serial.println("Audio size:" + String(fileSize));
  unsigned long expectedlength = framelength*totalframes;
  int expectedlengthinseconds = (int)(expectedlength/1000);
  Serial.println("Audio length:" + String(expectedlengthinseconds) + " seconds");
  int soundChunksPerImage = fileSize/(expectedlengthinseconds*framespeed) ;
  Serial.println("Audio chunks per image : " + String(soundChunksPerImage));
  int lastFilePos = 0;
  
  M5.Lcd.setCursor(0,0);
  M5.Lcd.print("FPS: ");
  M5.Lcd.println(video.framespeed);
  M5.Lcd.print(String(video.width*2) + "x" + String(video.height*2));
  
  uint8_t gridpos = 0;
  uint8_t gridsize = 4;
  uint16_t posx = (320 - (video.width*2)) / 2;
  uint16_t posy = (240 - (video.height*2)) / 2;
  
  while ( file.available() ) {
    framenumber++;
    unsigned long expected_time_frame = started_at + (framelength*framenumber);
    int expected_file_frame = soundChunksPerImage*(framenumber-1);
    file.read((uint8_t*) blockSize, 2);
    file.read((uint8_t*) pic, blockSize[0]);
    
    uint16_t x = posx;
    uint16_t y = posy;

    // only draw if video/audio are in sync
    if(abs(lastFilePos - expected_file_frame)  > soundChunksPerImage ) {
      mp3->loop();
      if ( expected_time_frame > millis() ) {
        switch(gridpos%gridsize) {
          case 0:
            // native pos
            fps(2);
          break;
          case 1:
            x += video.width;
          break;
          case 2:
            y += video.height;
          break;
          case 3:
            x += video.width;
            y += video.height;
          break;
          default:
            Serial.println("wut?");
          break;
        }
        M5.Lcd.drawJpg((uint8_t*) pic, blockSize[0], x, y);
      } else {
        Serial.println("Frame drop!");
        droppedFrames++;
      }
      gridpos++;
    } else {
      Serial.println("Sound drift");
      // skip next frame
      file.read((uint8_t*) blockSize, 2);
      file.read((uint8_t*) pic, blockSize[0]);
      framenumber++;
      gridpos+=2;
      // continue; // ?
      //expected_time_frame = started_at + (framelength*framenumber);
    }

    // play audio until timeframe window is expired
    while ( expected_time_frame > millis()) {
      if(mp3->isRunning())  {
        if (!mp3->loop()) {
          mp3->stop();
          break;
        }
      }
      if (digitalRead(BUTTON_A_PIN) == 0) {
        if(mp3->isRunning())  {
          mp3->stop();
        }
        return;
      }
    }
    lastFilePos = id3->getPos();
    //Serial.println(String(x) + "\t: " + String(y) +  "\t- " + String(expected_file_frame) + " / " + String(lastFilePos));
  }
  if(mp3->isRunning())  {
    mp3->stop();
  }
}



void setup() {
  M5.begin();
  Wire.begin();
  if (digitalRead(BUTTON_A_PIN) == 0) {
    Serial.println("Will Load menu binary");
    updateFromFS(SD);
    ESP.restart();
  }

  while(!SD.begin(TFCARD_CS_PIN)) {
    // TODO: make a more fancy animation
    unsigned long now = millis();
    toggle = !toggle;
    uint16_t color = toggle ? BLACK : WHITE;
    M5.Lcd.setCursor(10,100);
    M5.Lcd.setTextColor(color);
    M5.Lcd.print("Insert SD");
    delay(300);
  }

  M5.Lcd.clear();
  Serial.println("Started");

  M5Menu.clearList();
  M5Menu.setListCaption("Playlist");
  M5Menu.drawAppMenu("Video Player", "Stop", "Play", "Next");
  
  if(!loadPlaylist()) {
    getPlaylist();
  }
  if( Playlist[videoNum].thumbFileName!="" ) {
    M5.Lcd.drawJpgFile(SD, Playlist[videoNum].thumbFileName.c_str(), 160, 40, 160, 160, 0, 0, JPEG_DIV_NONE);
  }
  
}

void loop() {

  if(digitalRead(BUTTON_B_PIN) == 0) {
    Serial.println("B Pressed");
    if( playListSize>videoNum ) {
      playVideo(Playlist[videoNum]);
    } else {
      // download playList 
      getPlaylist();
    }
  }
  if(digitalRead(BUTTON_C_PIN) == 0) {
    Serial.println("C Pressed");
    if( playListSize>videoNum ) {
      videoNum++;
    } else {
      videoNum = 0;
    }
    if(playListSize==videoNum) {
      M5Menu.drawAppMenu("Video Downloader", "Sync", "Download", "Next");
    } else {
      M5Menu.drawAppMenu("Video Player", "Stop", "Play", "Next");
    }
    M5Menu.setListID(videoNum);
    M5Menu.showList();
    if( Playlist[videoNum].thumbFileName!="" ) {
      M5.Lcd.drawJpgFile(SD, Playlist[videoNum].thumbFileName.c_str(), 160, 40, 160, 160, 0, 0, JPEG_DIV_NONE);
    }
  }
  
  if(digitalRead(BUTTON_A_PIN) == 0) {
    Serial.println("A Pressed");
  }
  delay(150);
} 
