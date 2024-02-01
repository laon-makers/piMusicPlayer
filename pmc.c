/* Copyright 2024   Gi Tae Cho

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.*/
   
/* File Name: piMusicClient.c (pmc.c)
 * History:
   <2017>
   May  18: single click on 'play' works even when music server is off
            by the time when the button is pressed; enough delay time
            is the key to get this to work.            
   Jan. 29: 1. piMusicServer.h has been replaced with pms.h.
   <2016>
   Dec. 18, 1. saving ip address into local storage is available.
 *                2. Filename has been chaged to pmc.c to make typing easy.          
*/
//client.client
//server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <iostream>
#include <unistd.h>
#include <fstream>
using namespace std;
//#include "piMusicServer.h"
#include "pms.h"

//#define DBG_MSG_EN


/*
 Shared Memory Array Layout:
 [0]: command from piMusicClient. ' '= no command available.
 [1]: log en/disable. ' '=disable, 'l'=enable.
 [2]: piMusicServer status. ' '=off, 's'=on.
*/
#define PI_MUSIC_CMD      0
#define PI_MUSIC_LOG      1
#define PI_MUSIC_SVR      2
// The last element of the shared memory to put an end of string expression by asigning '\0'.
#define PI_MUSIC_END      3

//#define NULL 0
int CmdResponse(char * pCmd);
char* exec(const char* command, int delay = 0 );
void printMsg(bool bLog, const char * pStr, bool bForce = false);


fstream myfile;
ShmIpStatus * pShmIp;
char *pShmSvr;//, *pShmIp;
bool bLog = false;




/*********************************************************
*
*
*********************************************************/
int main (int argc, char * argv[])
{
    int shmid;
    key_t key;
    char *s;
    int i,j,k;
    char cmd[MAX_CMD_STR_LEN+1]; // add 1 for string terminal value '\0'.

    //system("if [ ! -f piMsClient.txt ]; then touch rmtCmd.txt fi");
#ifdef DBG_MSG_EN
    myfile.open ("/mnt/wsi/piMsClient.txt", ios::out | ios::app);
    myfile << "argc: "<< argc << " (";

    for(i = 1; i < argc; i++) {
        myfile << argv[i] << "\t";
    }
    myfile << ")\n";
    bLog = true;
#endif

    // the shared memory for ip/music status is supposed to be initially created by ipStatus app.
    key = SHM_ID_IP_STATUS;
    shmid = shmget(key, SHM_IP_STATUS_SIZE, IPC_CREAT | 0666);

    if(shmid < 0 ) {
        usleep(100);
        shmid = shmget(key, SHM_IP_STATUS_SIZE, IPC_CREAT | 0666);
        if(shmid < 0 ) {
            cout << "shmget error in getting ipStatus in piMusicClient!" << endl; // Need it to let Android know.  
            printMsg( bLog, "shmget error in getting ipStatus in piMusicClient!\n", true); 
            myfile.close(); // Need to close because it has been open by the last argument in the method immediate above.
            return(-1);
        }
    }

    pShmIp = (ShmIpStatus *)shmat(shmid, (void*)0, 0 );
    
    if(pShmIp == (ShmIpStatus*) -1) {
        usleep(100);
        pShmIp = (ShmIpStatus *)shmat(shmid, (void*)0, 0 );
    
        if(pShmIp == (ShmIpStatus *) -1) {
            cout << "shmat error in getting ipStatus in piMusicClient!" << endl;
            printMsg( bLog, "shmat error in getting ipStatus in piMusicClient!\n", true);
            myfile.close(); // Need to close because it has been open by the last argument in the method immediate above.
            return(-2);
        }
    }


#ifdef DBG_MSG_EN
    //myfile.open ("/mnt/wsi/piMsClient.txt", ios::out | ios::app);
    //*(pShmIp + PI_MUSIC_END) = '\0';
    myfile << "shm(ipStatus): "<< pShmIp->cmd << ", " << pShmIp->logEn << ", " << pShmIp->musicSvr << "\n";
#endif

    if( pShmIp->logEn == PI_MUSIC_LOG_EN ) {

        if( bLog == false ) {
            bLog = true; // Log enabled.
            //system(" if [ ! -f /mnt/wsi/piMsClient.txt ]; then echo > /mnt/wsi/piMsClient.txt; chmod +w /mnt/wsi/piMsClient.txt; fi");
            myfile.open ("/mnt/wsi/piMsClient.txt", ios::out | ios::app);
            //system("chmod +w /mnt/wsi/piMsClient.txt");
        }

        myfile << "shmid of ipStatus: " << shmid << "\nargc: " << argc << ", "; 

        for( i = 1; i < argc; i++) {
            myfile << argv[i] << ", ";
        }
        myfile << "\n";
    }

    if( argc > 1 ) {
        if( argv[1][0] == CMD_MUSIC_SVR_OFF ) {

            pShmIp->cmd = argv[1][0]; 
            //cout << "OK, quit" << endl; // It is sent to Android phone as a cmd response. Dec. 11, 2016: moved into CmdResponse(...).

        } else if ( ( argv[1][0] == CMD_PLAY ) || ( argv[1][0] == CMD_PLAY_NEW_LIST ) ) { 
            // added the 'else if' statement above in order to get the music system ready if it is not. 
            if(pShmIp->musicSvr != PI_MUSIC_SVR_ON) pShmIp->cmd = argv[1][0]; 
        }
    }

    if( argc > 2 ) {
        if( argv[1][0] == CMD_CTRL_CMD_HEAD ) {
            // if it is
            //    '~', it will get the piMusicServer started.
            //    'L', log message will be spit out to piMsClient.txt.
            pShmIp->cmd = argv[2][0]; 

            if( bLog == false ) {
                // bLog = true;
                //system(" if [ ! -f /mnt/wsi/piMsClient.txt ]; then echo > /mnt/wsi/piMsClient.txt && chmod +w /mnt/wsi/piMsClient.txt; fi");
                myfile.open ("/mnt/wsi/piMsClient.txt", ios::out | ios::app);
                //system("chmod +w /mnt/wsi/piMsClient.txt");
            }

            myfile << "argc: " << argc << ", "; 
            for( i = 1; i < argc; i++) {
                myfile << argv[i] << ", ";
            }
            myfile << "\n";
            pShmIp->end = '\0';
            myfile << "shm<ipStatus>: " << (char *)pShmIp << "\n"; 
            


	    if( argv[2][0] == CMD_LOG_EN ) {
                //if( bLog == false ) {
                //    myfile.close();
                //}
                bLog = true;

            } else {
                myfile.close();
                //if ( argv[2][0] == 'i' ) {
                    //Dec. 11, 2016: Following is requred to avoid exception in Androiod which
                    //  reads stream input whenever it send cmmand to piMusicClient.
                    //cout << "Session connected" << endl; //Dec. 11, 2016: Moved into CmdResponse(...).
                //}
                CmdResponse(&(pShmIp->cmd));

                return 0;
            }
        } else if( argv[1][0] == CMD_HOMEAUTO_CMD_HEAD ) {
            CmdResponse(argv[1]);

            return 0;
        }
    }
    
    //myfile  << endl;
    //myfile << ", " << argv[0] << " ";
    

#ifdef DBG_MSG_EN
    printMsg( bLog, "It is about to send cmd to piMusicServer\n" );
#endif

    // this shared memory for server command buffer is supposed to be initally 
    // created by either this app or pi music system client depending on which
    // one is called following command first.
    key = SHM_ID_MUSIC_SVR;
    shmid = shmget(key, SHM_SEVER_SIZE, IPC_CREAT | 0666);

    if(shmid < 0 ) {
        usleep(100);
        shmid = shmget(key, SHM_SEVER_SIZE, IPC_CREAT | 0666);

        if(shmid < 0 ) {
            cout << "shmget error in getting piMusicServer in piMusicClient!" << endl;
            if( bLog == false ) { 
                //system(" if [ ! -f /mnt/wsi/piMsClient.txt ]; then echo > /mnt/wsi/piMsClient.txt && chmod +w /mnt/wsi/piMsClient.txt; fi");
                myfile.open ("/mnt/wsi/piMsClient.txt", ios::out | ios::app);
                //system("chmod +w /mnt/wsi/piMsClient.txt");
            }
            myfile << "shmget error(" << shmid << ") in getting piMusicServer in piMusicClient!\n"; 
            myfile.close();
            return(-3);
        }
    }

    pShmSvr = (char *)shmat(shmid, (void*)0, 0 );
    
    if(pShmSvr == (char*) -1) {
        usleep(100);
        pShmSvr = (char *)shmat(shmid, (void*)0, 0 );
    
        if(pShmSvr == (char*) -1) {
            cout << "shmat error in getting piMusicServer in piMusicClient!" << endl;
            printMsg( bLog, "shmat error in getting piMusicServer in piMusicClient!\n", true);
            myfile.close();
            return(-4);
        }
    }
#if 1
    // 200170516: added following statments to get the music started with single play commmand.
    if( (argv[1][0] == CMD_PLAY) || (argv[1][0] == CMD_PLAY_NEW_LIST) ) { //'s' or 'S'
        if(pShmIp->musicSvr != PI_MUSIC_SVR_ON) {

#ifdef DBG_MSG_EN
            printMsg( bLog, "piMusicServer is Off.\n" );
#endif
            //pShmIp->cmd = argv[1][0];
            i = 0;
            while( i < 50) {
                usleep(500000);
                if(pShmIp->musicSvr == PI_MUSIC_SVR_ON) break;
                i++;
            }

            if( i >= 50 ) {
                cout << "_NOK, piMusic Server Off Yet" << endl;
#ifdef DBG_MSG_EN
                printMsg( bLog, "piMusicServer is Off yet.\n" );
#endif
                usleep(100);
                return(-10);
            }
            // to wait the music server finish its initialization.
            i = 0;
            while( i < 50) {
                usleep(500000);
                s = exec("pidof mpg123");
                if( (s != NULL) && (*s != '\0') ) {
                    break;
                }
                i++;
            }
 
            if( i >= 50) {
                cout << "_NOK, mp3 player off Yet" << endl;
#ifdef DBG_MSG_EN
                printMsg( bLog, "mp3 Player is Off yet.\n" );
#endif
                usleep(100);
                return(-21);
            }
            usleep(500000); // 20170518: This time dely must be here in order to
                            //   wait the music server finish its initialization, especially
                            //   mpg123 stablization.
#ifdef DBG_MSG_EN
            printMsg( bLog, "piMusicServer is On.\n" );
#endif
        }
    }
#endif

    // Put the command and arguments into one single array of a buffer 'cmd'.
    k = 0;
    for(i = 1; i < argc; i++){
        int j = 0;
        if( i > 1 ) { // Put the delimiter ' ' between a command and following argument or between arguments.
            cmd[k++] =  CMD_ARG_DELIMITOR;
            if( k >= MAX_CMD_STR_LEN ) break;
        }

        while( argv[i][j] != '\0' ) { 
            cmd[k++] = argv[i][j++];
            if( k >= MAX_CMD_STR_LEN ) break;
        }
    }
  
    cmd[k] =  '\0';
    s = pShmSvr;

    for( i = 0; cmd[i] != '\0'; i++ ) {
        *(s++) = cmd[i];
    }

    *s = '\0';

    //CmdResponse((char *) pShmSvr);
    CmdResponse(cmd);

    if( bLog == true ) {
        myfile << "shmid of piMusicServer: " << shmid << "\n"; 
        myfile << "shm(server): " << pShmSvr << "\n"; 
        myfile.close();
    }
    cmd[0] = 0;
    return 0;
}


/*********************************************************
*
*
*********************************************************/
int CmdResponse(char * pCmd)
{
    char * s;
    char str[80];
    int cnt, i, j, k;

    switch(*pCmd) {
    case CMD_PAUSE: //'|'
        cout << "_OK, pause" << endl; 
        break;
    case CMD_GET_MUSIC_DIR:
        //system("clear; cd /home/pi/music; ls -d */ | tr -d '/'; if [ -f /mnt/wsi/songs.txt ]; then rm /mnt/wsi/songs.txt; fi;");
        //system("ls -d */ | tr -d \'/\'");
        system("cat /home/pi/music/dirlist.lst");
        break;

    case CMD_SET_MUSIC_DIR:
        cout << "_OK, new music directory being set";
        break;

    case CMD_GET_NEW_PLAY_LIST: //'='
#if 0       
        str[0] = 'l';
        str[1] = 's';
        str[2] = ' ';
        cnt = 3;
        while(*(++pCmd) != '\0') {
            str[cnt++] = *(pCmd);
        }
                                   //12345678 112345678 212345678 312345678 412345678
        memcpy((char *)&(str[cnt]), "/*.mp3 | tr -d '/' > /mnt/wsi/songs.txt", 41);
        system(str);
        usleep(500000);
        system("clear; cat /mnt/wsi/songs.txt");
        break;
#endif       
    case CMD_GET_PLAY_LIST: //'-'
        //s = exec("ls /home/pi/music/melon0615/*.mp3 | cut -d'/' -f6 > /mnt/wsi/songs.txt");
        //system("if [ -f /mnt/wsi/songs.txt ]; then cat /mnt/wsi/songs.txt; else ls /home/pi/music/melon0615/*.mp3 | cut -d'/' -f6 > /mnt/wsi/songs.txt ; cat /mnt/wsi/songs.txt; fi");
        //system("cat /mnt/wsi/songs.txt");
        //system("clear; if [ -f /mnt/wsi/songs.txt ]; then cat /mnt/wsi/songs.txt; else ls /home/pi/music/" + playListDir[selectedPlayListDir] + "/*.mp3 | cut -d'/' -f6 > /mnt/wsi/songs.txt ; cat /mnt/wsi/songs.txt; fi");
        //system("if [ ! -f /mnt/wsi/songs.txt ]; then ls " + &(pCmd+1) + "/*.mp3 | tr -d '/' > /mnt/wsi/songs.txt ; cat /mnt/wsi/songs.txt; fi;");
        system("clear; cat /mnt/wsi/songs.txt");
        break;
    case CMD_FFWD: //'f'
        cout << "_OK, fast forward cmd received" << endl; 
        break;
    case CMD_RWND: //'r'
        cout << "_OK, rewind" << endl; 
        break;
    case CMD_NEXT_SONG: //'n'
        cout << "_OK, next song" << endl; 
        break;
    case CMD_PREV_SONG: //'p'
        cout << "_OK, previous song" << endl; 
        break;
    case CMD_GET_INFO: //'i' The shared memory for the server is not available yet with this command.
    case CMD_CONNECT:  //':' The shared memory for the server is not available yet with this command.
        //cout << "_OK, session connected";
        cout << "_OK," << PIMS_STATUS_RPT_DELIMITER << "" << (char *) pCmd << "" << 
               ((((pShmIp->stopCounter & MASK_STOP_SHUTDOWN) > 0) &&
                  (pShmIp->stopCounter > STOP_COUNTER_DISABLE) ) ? STOP_SHUTDOWN_EN_INX : 'F') <<
             " ";
        break;

    case CMD_MUSIC_SVR_ON: //'~' If this command was received with the leading '*' command 
                           //    which is CMD_CTRL_CMD_HEAD, then the shared memory for the server is not
                           //    available yet with this command.
        cnt = 0;
        while( cnt < 20) {
            usleep(200000);
            //s = exec("pidof piPlayer");
            //if( (s != NULL) && (*s != '\0') ) {
            if( pShmIp->musicSvr == PI_MUSIC_SVR_ON ) {
                break;
            }
            cnt++;
        }
 
        if( cnt >= 20) {
            cout << "_NOK, piPlayer Off Yet" << endl;
        } else {
            cout << "_OK, piPlayer On" << endl;
        }
                    
        break;
    case CMD_PLAY:          // 's':
        //system("echo +l > /sys/class/ledControl/set/led"); //2017.6.16: Relay/Light On
#if 0
    case CMD_PLAY_NEW_LIST: // 'S': // 20170516: added.
        // 200170516: added following statments to get the music started with single play commmand.
        //if( (argv[1][0] == CMD_PLAY) || (argv[1][0] == CMD_PLAY_NEW_LIST) ) { //'s' or 'S'
        cnt = 0;
        while( cnt < 20) {
            if(pShmIp->musicSvr == PI_MUSIC_SVR_ON) break;
            usleep(200000);
            cnt++;
        }

        if( cnt >= 20 ) {
            cout << "_NOK, piMusic Server Off Yet" << endl;
#ifdef DBG_MSG_EN
            printMsg( bLog, "piMusicServer is Off yet.\n" );
#endif
            break;
        } else if ( cnt > 0 ) { // piMusicServer has just gotten started by '~' command from ipStatus.c(is.c).
                                // Therefore you need to send original command again here.
            *pShmSvr = *pCmd;
            *(pShmSvr + 1) = '\0';
        }
#endif
    case CMD_RANDOM_PLAY: //'P':
    case 'T':
        cnt = 0;
        while( cnt < 10) {
            usleep(200000);
            s = exec("pidof mpg123");
            if( (s != NULL) && (*s != '\0') ) {
                break;
            }
            cnt++;
        }
 
        if( cnt >= 10) {
            cout << "_NOK, mp3 player off Yet" << endl;
        } else {
            cout << "_OK, mp3 player on" << endl;
        }
                    
        break;

    case CMD_REPLAY: //'>': // Playback
        cout << "_OK, playback" << endl;
        break;

    case CMD_STOP: //'.': // Stop
#ifdef RPI_DEMO_DAY_2017_06_17
        system("echo -l > /sys/class/ledControl/set/led"); //2017.6.16: Relay/Light Off
#endif
        cout << "_OK, stop" << endl;
        break;

    case CMD_SET_STOP_COUNT:
        s = pCmd + 1;
        cnt = i = j = 0;

        while( *s != '\0' ) {
            if( *s != CMD_ARG_DELIMITOR ) {
                if( *s != CMD_ARG_NO_SHUTDOWN )
                    j = MASK_STOP_SHUTDOWN;
                else s++;
 
                while( *s == CMD_ARG_DELIMITOR ) s++;
 
                if( (*s >= '0') && (*s <= '9') ) {
                    cnt = (*(s++) - '0');

                    if( (*s >= '0') && (*s <= '9') ) {
                        cnt *= 10;
                        cnt += (*(s++) - '0');

                        if( (*s >= '0') && (*s <= '9') ) {
                            cnt *= 10;
                            cnt += (*(s++) - '0');
                        }
                    }
                } else if ( *s == '-' ) cnt = STOP_COUNTER_DISABLE; // value less than 0 is considered as disable.
                break;

            } else {
                s++;                
                if( (i++) > 10 ) {
                    break;
                }
            }
        }

        if( cnt == 0 ) {
            cout << "_NOK, wrong command format!";
            printMsg(bLog, "Error! Wrong command format.");
            printMsg(bLog, pCmd); 
        } else {
            if( cnt > 0 ) cnt += j;
            else cnt = STOP_COUNTER_DISABLE;
            pShmIp->stopCounter = cnt;
            if( pShmIp->stopCounter != cnt ) pShmIp->stopCounter = cnt; // to make sure it was set correctly in case the shared memory was modified by the music player while the same place was being modified by this instance.
            cout << "_OK, stopCounter=%x", cnt;
        }
        break;

    case CMD_MUSIC_SVR_OFF: //'q':

#ifdef RPI_DEMO_DAY_2017_06_17
        system("echo -l > /sys/class/ledControl/set/led"); //2017.6.16: Relay/Light Off
#endif
        cnt = 0;
        while( cnt < 15) {
            s = exec("pidof mpg123");
            if( (s != NULL) && (*s != '\0') ) {
                break;
            }
            usleep(20000);
            cnt++;
        }

        if( cnt >= 15) {
            cout << "_NOK, mp3 player is still on" << endl;
            break;
        }

        while( cnt < 20) {
            //s = exec("pidof piPlayer");
            //if( (s != NULL) && (*s != '\0') ) {
            if( pShmIp->musicSvr == PI_MUSIC_SVR_OFF ) {
                break;
            }
            usleep(20000);
            cnt++;
        }
 
        if( cnt >= 20) {
            cout << "_NOK, piPlayer still on" << endl;
        } else {
            cout << "_OK, both mp3 and piPlayer off" << endl;
        }
                
        break;

    case CMD_VOLUME: //'v':
        cout << "_OK, volume" << endl;
        break;

    case CMD_DEBUG_CMD_HEAD: // This command is mainly for human being. Use CMD_GET_INFO for a program.
       switch ( (char) *(pCmd+2) ) {
       case CMD_ARG_GET_SHM: 
           cout << "Shared Memory Server:\t" << pShmSvr <<
                   "\nShared Memory IpStatus:\n\t" << "cmd:\t" << pShmIp->cmd << 
                   "\n\tplayStatus:\t" << pShmIp->playStatus << 
                   "\n\tvolume(%):\t" << pShmIp->volume[0] << pShmIp->volume[1] << 
                   "\n\tmusicSvr:\t" << pShmIp->musicSvr <<
                   "\n\tpiPlayer:\t" << pShmIp->piPlayer << 
                   "\n\tmusicPlayer:\t" << pShmIp->musicPlayer <<
                   "\n\tlogEn:\t\t" << pShmIp->logEn << 
                   "\n\tplay mode:\t" << pShmIp->mode <<
                   "\n\tnextSong idx:\t" << pShmIp->nextSong[0] << pShmIp->nextSong[1] << pShmIp->nextSong[2] <<
                   "\n\tcurrent Dir:\t" << pShmIp->currentDir[0]<< pShmIp->currentDir[1] <<
                   "\n\tlastSong:\t" << (int) pShmIp->lastSong << 
                   "\n\tstopTimer (shutdown/minutes):\t";
           if( pShmIp->stopTimer <= STOP_TIMER_DISABLE ) {
               cout << "N / -1";
           } else {
               cout << ((pShmIp->stopTimer & MASK_STOP_SHUTDOWN) > 0 ? 'Y':'N') << " / " <<
                       (pShmIp->stopTimer & MASK_STOP_TIMER);
           }

           cout << "\n\tstopCounter (shutdown/count):\t";

           if( pShmIp->stopCounter <= STOP_COUNTER_DISABLE ) {
               cout << "N / -1\n";
           } else {
               cout << ((pShmIp->stopCounter & MASK_STOP_SHUTDOWN) > 0 ? 'Y':'N') << " / " <<
                       (pShmIp->stopCounter & MASK_STOP_COUNTER) << "\n";
           }

           cout << exec( "if [ \"$(pidof ipAppStable)\" != \"\" ]; then echo -e \"\\tipAppStable:\\tOn\"; else echo -e \"\\tipAppStable:\\tOff\"; fi;" );
           cout << exec( "if [ \"$(pidof ipStatus)\" != \"\" ]; then echo -e \"\\tipStatus:\\tOn\"; else echo -e \"\\tipStatus:\\tOff\"; fi" );
           break;
       case CMD_ARG_GET_VER:
           cout << "Pims client build: " << __DATE__ << ":" << __TIME__ << "\t\t";
           break;
       default:
           cout << "Unknown command: " << pCmd << "\n";
           break;
       }

       break;        
    case CMD_ADD_NEW_FILES:
        cout << "_OK, Update song list files" << endl;
        break;

    case CMD_CREATE_FILES:
        cout << "_OK, Create song list files" << endl;
        break;

    case CMD_REBOOT: //'R':
        cout << "_OK, Reboot" << endl;
        break;
    case CMD_SHUTDOWN: //'0': this is the numer zero in ascii character;not alphabet.
        cout << "_OK, Shutdown" << endl;
        break;




    case CMD_HOMEAUTO_CMD_HEAD:
        pCmd +=2;
        switch(*pCmd) {
        case CMD_HA_ARG_LIGHT_ON:
            system("echo +l > /sys/class/ledControl/set/led");
            cout << "_OK, Light (relay) On" << endl;
            break;
        case CMD_HA_ARG_LIGHT_OFF:
            system("echo -l > /sys/class/ledControl/set/led");
            cout << "_OK, Light (relay) Off" << endl;
            break;
        default:

            cout << "_NOK, Unknown HomeAuto command!" << endl;
            break;
        }
    }

    return 0;

}




// *******************************************************
/*********************************************************
*
*
*********************************************************/
void printMsg(bool bLog, const char * pStr, bool bForce) 
{
    if( bLog == true ) {
        myfile << pStr;// << "\n";
    } else if ( bForce == true ) {
        myfile.open ("/mnt/wsi/piMsClient.txt", ios::out | ios::app);
        myfile << pStr;// << "\n";
        //myfile.close(); // Commented out to have the caller close it.
    }
    /*
     else {
        cout << pStr << endl; 
    }*/
    //return bLog;
}


/*********************************************************
*
* Calling function must free the returned result.
*
*********************************************************/
char* exec(const char* command, int delay) {
  FILE* fp;
  char* line = NULL;
  // Following initialization is equivalent to char* result = ""; and just
  // initializes result to an empty string, only it works with
  // -Werror=write-strings and is so much less clear.
  //char* result = (char*) calloc(1, 1);
  size_t len = 0;
  int i;

  fflush(NULL);
  fp = popen(command, "r");
  if (fp == NULL) {
    printf("Cannot execute command:\n%s\n", command);
    return NULL;
  }

  if( delay > 0 ) sleep(delay);
  else {
      for( i=0; i<200; i++) len++;
  }

  len = 0;

  getline(&line, &len, fp);
  fflush(fp);
  if (pclose(fp) == -1 ) {
    perror("Cannot close stream.\n");
  }
  //return result;
  return line;
}



