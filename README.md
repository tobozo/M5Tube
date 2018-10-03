# M5Tube

M5Tube is a video player for M5Stack.

![image](https://user-images.githubusercontent.com/1893754/46413569-8be9be80-c721-11e8-8547-5c1d063c1d8c.png)

M5Tube is also a video conversion utility, it can be used locally or in a Docker image.

![image](https://user-images.githubusercontent.com/1893754/46411749-dddc1580-c71c-11e8-8f6a-5fa7a5527877.png)


Software Stack prerequisites
----------------------------
  - [M5Stack.h](https://github.com/m5stack/M5Stack/)
  - [M5StackUpdater](https://github.com/tobozo/M5Stack-SD-Updater/) and its [folder structure](https://github.com/tobozo/M5Stack-SD-Updater/releases)
  - [M5StackSAM](https://github.com/tomsuch/M5StackSAM/)
  - [ArduinoJson](https://github.com/bblanchon/ArduinoJson/)
  - [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio/)


Hardware Stack prerequisites
----------------------------
  - M5Stack
  - External DAC (e.g PCM5102) connected to the I2S
  - MicroSD Card

Installation
------------
  - either:
    1) create "/mp3", "/vid" and "/json" folders on the MicroSD Card 
    2) use the [SD-Apps-Folder](https://github.com/tobozo/M5Stack-SD-Updater/releases) folder structure from the M5Stack-SD-Updater
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
  The conversion utily provided with this projects makes the process of converting the video to an acceptable format for the M5Stack much less painful.
  The mp4 format is required for the source file and audio is mandatory for the conversion to complete.
  After the audio is converted and recompressed to mp3 mono @32Kbps/22Khz, the video is split into frames, and each frame is split into 4 chunks.
  Only one of these 4 chunks is kept, converted into JPEG with variable compression rate, and stacked into a file.
  Custom conversion settings are available such as video width and processed chunks per second.

Player functionalities
----------------------
  - select video
  - play video
  - stop video
  - synchronize/download playlist
  - synchronize/download video/audio
  - load the [M5Stack-SD-Menu](https://github.com/tobozo/M5Stack-SD-Updater)

All videos are played from the SD and can't be paused (only stopped but PR's accepted).

The sync/download is optional but requires to have a web server handling TLS and exposing the video/audio/json files, some editing in the .ino file, and the gathering of the TLS certificate chain for certificates.h.

Any successful sync operation will overwrite the local files.
  
Credits
-------
  - [Masa Hage](https://github.com/MhageGH) (motivation)
  - [Tixlegeek](https://github.com/tixlegeek) (art director)
  - [Cyriak](https://www.youtube.com/user/cyriak) (materials for the demo video)
