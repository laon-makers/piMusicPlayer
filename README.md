This piMusicPlayer project is a sub-set of a Linux piMusicSystem running on Raspberry-pi.
The system consists of a Raspberry-pi with mounted LCD module and a smart phone.
An Android client application on the smart phone is used to remotely control piMusicPlayer on the Raspberry-pi.
You can also locally control the piMusicPlayer either with buttons on the LCD module or by running a bash script.
This piMusicPlayer project contains bash scripts and C source code.

The script pimscontrol is a bash script in the music folder.
The LCD module with mounted 5 buttons is a 16x2 module purchased on aliexpress.com.
Both LCD and buttons are controlled with separate project.
All of bash scripts and C source code for not only this project but also other projects 
including Android application and LED kernel driver were created by Gi Tae Cho.

If you use the system with source code which updated either on or after Jun. 18, 2017, 
you need following files and folders to make the system up running.


  /mnt/wsi                        ram file system
  /mnt/wsi/mp3fifo                fifo
  /home/pi/music/                 There shouldn't be any folder under 'music' folder unless folders are music folder.
  /home/pi/music/pimscontrol      bash script
  /home/pi/music/piMusicServer    music server
  /home/pi/music/piMusicClient    music client

  /home/pi/ledapp/ipStatus        monitoring ip status and work together pi music server and client. It is a separate repository 'ipStatus'
  /home/pi/h/ipAppStable          use it if 16x2 LCD module is applicable. It is a separate repository.
  /home/led/led.ko                led kernel module. It is a driver and a separate repository for the linux kernel.

  two shared memory
  piMusicSystem Android client application. This Android application on a smart phone allows you to control the music server remotely.


  
