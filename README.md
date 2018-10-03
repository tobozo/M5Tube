# M5Tube

... is a video player for M5Stack ...

![image](https://user-images.githubusercontent.com/1893754/46413569-8be9be80-c721-11e8-8547-5c1d063c1d8c.png)

... and a video conversion utility in a Docker image

![image](https://user-images.githubusercontent.com/1893754/46411749-dddc1580-c71c-11e8-8f6a-5fa7a5527877.png)


Prereqsuisites
--------------
  - create "/mp3", "/vid" and "/json" folders on the MicroSD Card
  - either:
    1) install docker and start the provided docker image
    2) install ffmpeg + imagemagick + php locally
  - run the script 'workflow.sh' to get your videos converted
  - when conversion is complete, choose between:
    1) having the container's working dir exposed by a web server
    2) copying the contents of /vid, /mp3 and /json folders from the 
       container's working dir to your MicroSD Card

Conversion settings
-------------------
  The utily provided with this projects makes the process of converting the video to an acceptable format for the M5Stack
  less painful.
  The mp4 format is required for the source file and audio is mandatory for the conversion to complete.
  Audio is converted & recompressed to mp3 mono @32Kbps/22Khz, video is split into frames, each frame is split into 4 chunks.
  Chunks are encoded as JPEG with variable compression rate and stacked into a file.
  Custom conversion settings are available such as video width and processed chunks per second.

Player functionalities
----------------------
  - select video
  - play video
  - stop video
  - synchronize/download playlist
  - synchronize/download video/audio

All videos are played from the SD and can't be paused (only stopped but PR's accepted).

The sync/download is optional but requires to have a web server handling TLS and exposing the video/audio/json files, some editing in the .ino file, and the gathering of the TLS certificate chain for certificates.h.

Any successful sync operation will overwrite the local files.


Software Stack (ESP32)
----------------------
  [M5Stack.h](https://github.com/m5stack/M5Stack/)
  [M5StackUpdater](https://github.com/tobozo/M5Stack-SD-Updater/)
  [M5StackSAM](https://github.com/tomsuch/M5StackSAM/)
  [ArduinoJson](https://github.com/bblanchon/ArduinoJson/)
  [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio/)
  
Credits
-------
