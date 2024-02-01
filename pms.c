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

/*********************************************************************
 * Filename: piMusicServer.c (pms.c)
 * Main function that helps users to control the Raspberry Pi and to
 * get IP address.
 *
 * Revision history:
 * <2017>
   May  18: single click on 'play' works even when music server is off
            by the time when the button is pressed; enough delay time
            is the key to get this to work.            
 * May. 16: Unmute issue has been resolved.
 * Apr. 30: Volume up/down is supported.
 * Feb. 06: some command characters have been changed and applied 
 *          in all source files including pims client in Android.
 * Jan. 29: 1. piMusicServer.h has been replaced with pms.h.
 * Jan. 16: 1. Functions in piPlayer are being migrated into this file.
 * <2016>
 * Dec. 18: 1. saving ip address into local storage is available.
 *          2. filename has been changed ot pms.c to make typing easy.
 * *******************************************************************/

//#define DEBUG_MSG_EN
#ifdef DEBUG_MSG_EN
//# define DBG_PRINT_CMD
#endif

#include <iostream>
#include <fstream>
#include <unistd.h>  // Include for sleep(...)/usleep;
#include <stdio.h>
#include <stdlib.h>  // Include for system(...);
#include <sys/select.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstring>
//#include <cstdlib>
#include <pthread.h>
//#include "piMusicServer.h"
#include "pms.h"
using namespace std;

#define MASK_MP3FIFO_MISSING    0x01
#define MASK_PLYSTATUS_MISSING  0x02
#define MASK_PLAYLIST_MISSING   0x04
#define MASK_SONGS_MISSING      0x08
#define MASK__MISSING    0x0


// *****************************
char* exec(const char* command, int idx = 0, int delay = 0);

int kbhit(void);
//void printMsg(bool bLog, const char * pStr);
void printMsg(bool bLog, const char * pStr, bool bForce = false);

void PiPlayerOnOff(ShmIpStatus * shm, bool bOn, int start = 1, int end = MAX_SONG_INDEX_NUM);
int GetPiMusicSystemReady (void);
int PlayNextSong( int idx, bool bForceTo = false);
void * SendCmdToPlayer(void * pCmd);
//int PiPlayerCmd(char * pCmd);
int PiPlayerCmd( char * pCmd, bool bBusyCheck = true );
void * PlayMusic(void * shm);
int  GetNextSongIdxInShm(void * shm);
int SetNextSong(void * shm, int idx, bool bForceTo = false);
void SetToNextSong(void * shm);
void SetToSongBeingPlayed(void * shm);

int CreateSongListFiles(void);
int AddNewSongListFiles(void);

unsigned char GetTotalSongsInPlayList(void);
unsigned char GetAllPimsFilesReady(void);

fstream myfile;
static pthread_t piPlayerId = 0, piPlayerCmd = 0;
static bool bCmdThdBusy = false; // Command threaad busy
static bool bPiPlayerBusy = false;
ShmIpStatus * pShmIp;

/*********************************************************
*
*
*********************************************************/
int main(int argc, char *argv[])
{
    int sTime, i, tmp;
    char ch;
    int shmIpId, shmSvrId;
    key_t key;
    char *pShmSvr;
    char *s;
    bool bPlayer = false;
    bool bLog = false;
    char cmd[MAX_CMD_STR_LEN_SVR + 1];
    char fName[MAX_FNAME_STR_LEN_SVR + 1]; // File or foulder name must not be greater than 50.

    // the shared memory for ip/music status is supposed to be initially created by ipStatus app.
    key = SHM_ID_IP_STATUS;
    shmIpId = shmget(key, SHM_IP_STATUS_SIZE, IPC_CREAT | 0666);

    if (shmIpId < 0) {
        usleep(100);
        shmIpId = shmget(key, SHM_IP_STATUS_SIZE, IPC_CREAT | 0666);
        if (shmIpId < 0) {
            cout << "shmget error in getting ipStatus in piMusicServer!" << endl; // Need it to let Android know.  
            printMsg(bLog, "shmget error in getting ipStatus in piMusicServer!\n", true);
            myfile.close(); // Need to close because it has been open by the last argument in the method immediate above.
            return(-1);
        }
    }

    pShmIp = (ShmIpStatus *)shmat(shmIpId, (void*)0, 0);

    if (pShmIp == (ShmIpStatus*)-1) {
        usleep(100);
        pShmIp = (ShmIpStatus *)shmat(shmIpId, (void*)0, 0);

        if (pShmIp == (ShmIpStatus *)-1) {
            cout << "shmat error in getting ipStatus in piMusicServer!" << endl;
            printMsg(bLog, "shmat error in getting ipStatus in piMusicServer!\n", true);
            myfile.close(); // Need to close because it has been open by the last argument in the method immediate above.
            return(-2);
        }
    }


    pShmIp->musicSvr = PI_MUSIC_SVR_ON;

    if (pShmIp->logEn == PI_MUSIC_LOG_EN) {

        if (bLog == false) {
            bLog = true; // Log enabled.
            //system(" if [ ! -f /mnt/wsi/piMsClient.txt ]; then echo > /mnt/wsi/piMsClient.txt; chmod +w /mnt/wsi/piMsClient.txt; fi");
            myfile.open("/mnt/wsi/piMsServer.txt", ios::out | ios::app);
            //system("chmod +w /mnt/wsi/piMsClient.txt");
        }
    }

    // this shared memory for server command buffer is supposed to be initally 
    // created by either this app or pi music system client depending on which
    // one is called following command first.
    key = SHM_ID_MUSIC_SVR;
    shmSvrId = shmget(key, SHM_SEVER_SIZE, IPC_CREAT | 0666);

    if (shmSvrId < 0) {
        usleep(100);
        shmSvrId = shmget(key, SHM_SEVER_SIZE, IPC_CREAT | 0666);

        if (shmSvrId < 0) {
            myfile.open("/mnt/wsi/piMsServer.txt", ios::out | ios::app);
            printMsg(true, "shmget error in piMusicServer");
            myfile.close();
            pShmIp->musicSvr = PI_MUSIC_SVR_OFF;
            return(-3);
        }
    }

    pShmSvr = (char *)shmat(shmSvrId, (const void *)0, 0);

    if (pShmSvr == (char*)-1) {
        usleep(100);
        pShmSvr = (char *)shmat(shmSvrId, (const void *)0, 0);

        if (pShmSvr == (char*)-1) {
            myfile.open("/mnt/wsi/piMsServer.txt", ios::out | ios::app);
            printMsg(true, "shmget error in piMusicServer");
            myfile.close();
            pShmIp->musicSvr = PI_MUSIC_SVR_OFF;
            return(-4);
        }
    }
    *pShmSvr = '\0'; //init.

    //system("if [ ! -e '/mnt/wsi/mp3fifo' ];then mkfifo -m +rw '/mnt/wsi/mp3fifo' ; fi");
    //system("if [ ! -f '/mnt/wsi/plyStatus' ]; then touch /mnt/wsi/plyStatus; chmod a+w /mnt/wsi/plyStatus; fi");
    //system("if [ ! -f '/mnt/wsi/playlist.txt' ]; then ls /home/pi/music/favorite/*.mp3 > '/mnt/wsi/playlist.txt'; fi");

    s = exec("pidof mpg123");
    if ((s != NULL) && (*s != '\0')) {
        bPlayer = true;
        //pShmIp->piPlayer = PI_PLAYER_ON;
    } else {
        //pShmIp->piPlayer = PI_PLAYER_OFF;
    }

    if (argc > 1) {
        //if(( argv[1][0] == CMD_CTRL_CMD_HEAD ) || ( argv[1][0] == CMD_CONNECT )) {
        if ((argv[1][0] == CMD_CTRL_CMD_HEAD) || (argv[1][0] == CMD_MUSIC_SVR_ON) || (argv[1][0] == CMD_CONNECT)) {
            // The command passed in with the launch of this app. is given to this app itself
            // by placing in the server shared memory.
            *pShmSvr = argv[1][0];
            *(pShmSvr + 1) = '\0';
        }
    }

    sTime = M_SVR_SLEEP_TIME;
    cout << "piMusicServer gets started.." << endl;
    //sleep(2);

    //************************* Pi Music Server **************************
    while (true) {

        if (*pShmSvr != '\0') { // there is new command in the server shared memory.
#ifdef DEBUG_MSG_EN
            cout << "shm{server}: " << *pShmSvr << ", " << *(pShmSvr + 1) << endl;
#endif 
            if (bLog == true) {
                myfile << "cmd{svr}: " << *pShmSvr;
                i = 1;
                while (*(pShmSvr + i) != '\0') {
                    if (*(pShmSvr + i) == CMD_ARG_DELIMITOR) myfile << ", ";
                    else myfile << *(pShmSvr + i);
                    i++;
                }
                myfile << "\n";
                // If it is not closed than 'tail -f /mnt/wsi/piMsServer.txt' will update only 
                // when you disable the logging with the command "/home/pi/music/piMusicClient '*' 'l'".
                myfile.close();
                usleep(100000);
                myfile.open("/mnt/wsi/piMsServer.txt", ios::out | ios::app);
            }

            //               's'                       't'
            if ((*pShmSvr != CMD_PLAY) && (*pShmSvr != CMD_PLAY_NEW_LIST) && (*pShmSvr != CMD_STOP)) {
                if ((bPlayer == true) || (pShmIp->piPlayer == PI_PLAYER_ON)) {
                    s = exec("pidof mpg123");
                    if ((s != NULL) && (*s != '\0')) {
                        bPlayer = true;
                        //pShmIp->piPlayer = PI_PLAYER_ON;

                    } else {
                        s = exec("pidof mpg123");
                        if ((s != NULL) && (*s != '\0')) {
                            bPlayer = true;
                            //pShmIp->piPlayer = PI_PLAYER_ON;

                        } else {
                            bPlayer = false;
                            //pShmIp->piPlayer = PI_PLAYER_OFF;
                        }
                    }
                }
            }



            ///**********************************************************************************************

            switch (*pShmSvr) {

            case CMD_CTRL_CMD_HEAD: //'*':
                s = pShmSvr + 1;

                while (*s != '\0') {
                    if (*s == CMD_ARG_DELIMITOR) { // ' '
                        s++;
                    } else if (*s == CMD_LOG_EN) { //'l'
                        if (bLog == false) {
                            bLog = true; // Log enabled.
                            myfile.open("/mnt/wsi/piMsServer.txt", ios::out | ios::app);
                            myfile << "Log enabled!\n";

                            // If it is not closed than 'tail -f /mnt/wsi/piMsServer.txt' will update only 
                            // when you disable the logging with the command "/home/pi/music/piMusicClient '*' 'l'".
                            myfile.close();
                            usleep(100000);
                            myfile.open("/mnt/wsi/piMsServer.txt", ios::out | ios::app);
                            pShmIp->logEn = PI_MUSIC_LOG_EN;

                        } else {
                            myfile << "Log disabled!\n";
                            bLog = false;
                            myfile.close();
                            pShmIp->logEn = PI_MUSIC_LOG_DIS;
                        }
                        break;
                    }
                }

                break;

            case CMD_PAUSE: //'|'
                if (bPlayer == true) {

                    if (pShmIp->playStatus == PI_MUSIC_PLAY_PLAY) {
                        pShmIp->playStatus = PI_MUSIC_PLAY_PAUSE;
                        system("echo \'pause\' >> /mnt/wsi/mp3fifo");
                    }
                } else {
                    printMsg(bLog, "player is not available");
                    pShmIp->playStatus = PI_MUSIC_PLAY_STOP;
                    //pShmIp->piPlayer = PI_PLAYER_OFF;
                }
                break;
            case CMD_FFWD: //'f'
            case CMD_NEXT_SONG: //'n'
                if (bPlayer == true) {
                    //system("echo 'k' >> /mnt/wsi/mp3fifo");
                    //system("echo 'jump +300s' >> /mnt/wsi/mp3fifo");
                    system("echo \'stop\' >> /mnt/wsi/mp3fifo"); // This 'stop' command cause the song in the shared memory nextSong[] to be played.

                    //usleep(1000);

                    //system("echo 'play' >> /mnt/wsi/mp3fifo");

                    pShmIp->playStatus = PI_MUSIC_PLAY_PLAY;
                } else {
                    printMsg(bLog, "player is not available");
                    pShmIp->playStatus = PI_MUSIC_PLAY_STOP;
                }
                break;

            case CMD_RWND:      //'r'
            case CMD_PREV_SONG: //'p'
                if (bPlayer == true) {
                    //system("echo 'j -1s' >> /mnt/wsi/mp3fifo");
                    //SetNextSong( pShmIp, -1 );
                    if ((GetNextSongIdxInShm(pShmIp)) > 2) { // If current nextSong[] value is 2, the 1st song is being played. Therefore it shouldn't change the next song index neiter stop music.
                        SetNextSong(pShmIp, -2); // -2 because nextSong[] in the shared memory has the next song index number already.
                        system("echo \'stop\' >> /mnt/wsi/mp3fifo"); // This 'stop' command cause the song in the shared memory nextSong[] to be played.
                        pShmIp->playStatus = PI_MUSIC_PLAY_PLAY;
                    }
                } else {
                    printMsg(bLog, "player is not available");
                    pShmIp->playStatus = PI_MUSIC_PLAY_STOP;
                }
                break;

            case CMD_GET_INFO: //'i'
            case CMD_CONNECT:  //':'
                break;
                //            if( bPlayer == false ) system("mpg123 &");
                //            break;
            case CMD_MUSIC_SVR_ON: //'~'

                GetPiMusicSystemReady();
                /*
                s = exec("pidof piPlayer");
                if( (s == NULL) || (*s == '\0') ) {
                    if( *pShmSvr == CMD_CONNECT ) { //':'
                        system("cd /home/pi/music ; ./piPlayer ':' &");
                    } else {
                        system("cd /home/pi/music ; ./piPlayer '~' &");
                    }
                    *pShmSvr = '\0';
                    sleep(3);

                    s = exec("pidof piPlayer");

                    if( (s != NULL) && (*s != '\0') ) {
                        pShmIp->piPlayer = PI_PLAYER_ON;
                    }

                } else {
                    pShmIp->piPlayer = PI_PLAYER_ON;
                }
                */
                break;

            case CMD_SET_MUSIC_DIR: //'\' 
                //system("echo 'stop' >> /mnt/wsi/mp3fifo");
                //if( bPlayer == true ) system("echo \'q\' > /mnt/wsi/mp3fifo");
                //if( bPlayer == true ) system("echo \'p\' > /mnt/wsi/mp3fifo");
                //pShmIp->playStatus = PI_MUSIC_PLAY_PAUSE;


                tmp = 0;
                s = pShmSvr + 1;
                if (*s != '0') {
                    if (*s == CMD_ARG_DELIMITOR) s++;
                    while (*s != '\0') {
                        if (tmp < MAX_FNAME_STR_LEN_SVR) {
                            fName[tmp++] = *s;
                        } else {
                            system("if [ -f /mnt/wsi/pims.log ]; then "\
                                " echo \"Music directory name is too long - $(date)!\" >> /mnt/wsi/pims.log; " \
                                "fi");
                            break;
                        }
                        s++;
                    }
                }
                fName[tmp] = '\0';

#if 1
                // Copy given song list file to playlist.txt in /mnt/wsi folder.
                           //012345678 112345678 21 2345678 312345678 412345678
                memcpy(cmd, "cp -fs /home/pi/music/\" ", 30);
                //012345678 112345678 212345678 312345678 412345678
                memcpy((char *)&(cmd[23]), fName, tmp);
                //0123 45678 112345678 212345678 312345678 412345678
                memcpy((char *)&(cmd[23 + tmp]), ".lst\" /mnt/wsi/playlist.txt", 40);
                system(cmd);

#else                
                // Print song list from the given folder.
                           //012345678 112345678 212345678 312345678 412345678
                memcpy(cmd, "ls /home/pi/music/", 30);
                //012345678 112345678 212345678 312345678 412345678
                memcpy((char *)&(cmd[18]), fName, tmp);
                //012345678 112345678 212345678 312345678 412345678
                memcpy((char *)&(cmd[18 + tmp]), "/*.mp3 > /mnt/wsi/playlist.txt", 40);
                system(cmd);
#endif                

                // Update the last song index in the shared memory.
                                   //012345678 112345678 212345678 3123456 78  412345678
                //memcpy((char *)cmd, "wc -l /mnt/wsi/playlist.txt | cut -d \' \' -f1", 50);
                //s = exec((const char *) cmd);

                pShmIp->lastSong = GetTotalSongsInPlayList();

                // Update current directory index number in the shared memory.
                                   //012345678 112345678 212345678 312345678 4 12345678
                memcpy((char *)cmd, "cat /home/pi/music/dirlist.lst | grep -n \" ", 50);
                memcpy((char *)&cmd[42], fName, tmp);

                // 0123456 78  11 2345678 212345678 312345678 412345678
                memcpy((char *)&cmd[42 + tmp], "\" | cut -d \':\' -f1", 20);

                s = exec((const char *)cmd);
                i = 0;
                if ((s != NULL) && (*s != '\0')) {

                    if ((*s >= '0') && (*s <= '9')) {
                        ch = *s;
                        i++;
                        s++;
                        if ((*s >= '0') && (*s <= '9')) {
                            i++;
                        }// else if ( *s != '\0' ) i = 0;
                    }
                }

                if (i == 2) {
                    pShmIp->currentDir[SHM_UPPER_DIGIT_OF_2] = ch;
                    pShmIp->currentDir[SHM_LOWER_DIGIT_OF_2] = *s;
                } else if (i == 1) {
                    pShmIp->currentDir[SHM_UPPER_DIGIT_OF_2] = '0';
                    pShmIp->currentDir[SHM_LOWER_DIGIT_OF_2] = ch;
                } else {
                    pShmIp->currentDir[SHM_UPPER_DIGIT_OF_2] = DEFAULT_CURRENT_DIR_NO_MSB;
                    pShmIp->currentDir[SHM_LOWER_DIGIT_OF_2] = DEFAULT_CURRENT_DIR_NO_LSB;
                    //pShmIp->lastSong =  GetTotalSongsInPlayList();
                    system("if [ -f /mnt/wsi/pims.log ]; then "\
                        " echo \"Music directory index number is too big - $(date)!\" >> /mnt/wsi/pims.log; " \
                        "fi");
                }

                //usleep(500000);
                system("cat /mnt/wsi/playlist.txt | cut -d \'/\' -f6 > /mnt/wsi/songs.txt");

                //if( PlayNextSong(ShmIpStatus * shm, 1) == 0) bPlayer = true;
                //else bPlayer = false;
                /*pShmIp->musicPlayer = PI_MUSIC_PLAYER_OFF;
                pShmIp->playStatus = PI_MUSIC_PLAY_STOP;
                pShmIp->piPlayer = PI_PLAYER_OFF;*/
                break;

            case CMD_PLAY_NEW_LIST: //'S'
            case CMD_PLAY: //'s'

                //system("cd /home/pi/music && mpg123 -R -@/home/pi/music/melon0615/playlist_melon0615.txt --fifo /mnt/wsi/mp3fifo > /dev/null &");
                //system("cd /home/pi/music && mpg123 -R -s --fifo /mnt/wsi/mp3fifo > /dev/null &");
                //system("cd /home/pi/music && mpg123 -qR --fifo /mnt/wsi/mp3fifo > /mnt/plyStatus &");
                //system("cd /home/pi/music && echo 'loadlist playlist_melon0615.txt'  >> /mnt/wsi/mp3fifo");
                //sleep(5);
                //system("sleep 3");
                /*
                s = exec("pidof piPlayer");
                if( (s == NULL) || (*s == '\0') ) {
                    system("cd /home/pi/music ; ./piPlayer '~' &");
                    // *pShmSvr = '\0';
                    cout << "piPlayer is not available. It has been launched." << endl;
                    sleep(3);

                    continue;

                } else {
                    if ( *(pShmSvr+1) == '\0') {

                    } else if ( *(pShmSvr+2) == '\0' ) {

                         char cmd[41];
                         memcpy( cmd, "cd /home/pi/music && ./piPlayer  ", 40);
                         cmd[32] = *(pShmSvr+2);
                         //cmd[33] = ' ';
                         //cmd[34] = '5';
                         //cmd[35] = '0';
                         //cmd[36] = '0';
                         cmd[33] = ' ';
                         cmd[34] = '&';
                         cmd[35] = '\0';
                         system(cmd);

                    } else {

                         char cmd[41];
                         memcpy( cmd, "cd /home/pi/music && ./piPlayer  ", 40);
                         cmd[32] = *(pShmSvr+2);
                         cmd[33] = ' ';
                         cmd[34] = *(pShmSvr+3);
                         cmd[35] = ' ';
                         cmd[36] = '&';
                         cmd[37] = '\0';
                         system(cmd);
                    }
                }
                */
            {
#if 0
                char * dir;
                cmd[79] = '\0';
                s = exec("pidof piPlayer");
                //s = exec("pidof mpg123");
                if ((s != NULL) && (*s != '\0')) {

                    //system("echo 'q' > /mnt/wsi/mp3fifo");
                    //bPlayer = false;
                    //sleep(1);
                    //s = exec("pidof piPlayer");

                    //if( (s != NULL) && (*s != '\0') ) {
                    system("killall piPlayer");
                    //}
                    usleep(500000);
                }
#endif                    
                s = pShmSvr + 1;
                if (*s == CMD_ARG_DELIMITOR) s++;
#if 0
                if (*pShmSvr == CMD_PLAY_NEW_LIST) {
                    int idx;
                    idx = *s - '0';

                    printMsg(bLog, "Play with new list");

                    if (*(++s) == CMD_ARG_DELIMITOR) s++;
                    else {
                        idx *= 10;
                        idx += (*s - '0');

                        if (*(++s) == CMD_ARG_DELIMITOR) s++;
                    }

                    //dir = exec("ls /home/pi/music/*/ | tr -d '/'", idx);
                    //if( *dir == '\0' ) dir = exec("ls /home/pi/music/*/ | tr -d '/'", 0);
                    dir = exec("ls -d /home/pi/music/*/ | cut -d '/' -f5", idx);
                    if (*dir == '\0') dir = exec("ls -d /home/pi/music/*/ | cut -d '/' -f5", 0);
                    if (*dir == '\0') break;

                    //            012345678 112345678 212345678 3123
                    memcpy(cmd, "ls /home/pi/music/", 40); // 18 in case *dir is '\0';

                    printMsg(bLog, cmd);

                    i = 18;
                    while ((*dir != '\0') && (*dir != '\n')) {
                        cmd[i++] = *(dir++);

                        if (i > 47) break;
                    }
                    cmd[i] = '\0';
                    printMsg(bLog, cmd);

                    //                        012345678 112345678 212345678 3123
                    memcpy((char *)(cmd + i), "/*.mp3 > /mnt/wsi/playlist.txt", 40);

                    system(cmd);

                    printMsg(bLog, cmd);
                    sleep(1);
                }
#endif

#if 0
                //            012345678 112345678 212345678 3123
                memcpy(cmd, "cd /home/pi/music ; ./piPlayer  ", 40);

                i = 31;

                while (*s != '\0') {
                    //if( *s != ' ' ){
                    cmd[i++] = *s;
                    //}
                    s++;
                    if (i > 47) break;
                }

                cmd[i++] = ' ';
                cmd[i++] = '&';
                cmd[i] = '\0';
                system(cmd);
#endif
                if (pShmIp->musicPlayer == PI_MUSIC_PLAYER_OFF) {
                    //system("cd /home/pi/music ; ./piPlayer '~'");
                    if ((GetPiMusicSystemReady()) < 0) {
                        bPlayer = false;
                        pShmIp->playStatus = PI_MUSIC_PLAY_STOP;
#ifdef RPI_DEMO_DAY_2017_06_17
                        system("echo -l > /sys/class/ledControl/set/led"); //2017.6.16: Relay/Light Off
#endif
                        break;
                    }

                }

                i = 0;
                if ((*s != '\0') && (*s >= '0') && (*s <= '9')) {
                    do {
                        i *= 10;
                        i += (*s - '0');
                        s++;
                    } while ((*s != '\0') && (*s >= '0') && (*s <= '9'));
                } else i = 1;

                //PiPlayerOnOff( pShmIp, true, i);
                if (PlayNextSong(i, true) >= 0) {
                    bPlayer = true;
#ifdef RPI_DEMO_DAY_2017_06_17
                    system("echo +l > /sys/class/ledControl/set/led"); //2017.6.16: Relay/Light ON
#endif
                } else bPlayer = false;
                //if( bLog == true ) myfile << cmd << "\n";

                //pShmIp->playStatus = PI_MUSIC_PLAY_PLAY;
                //pShmIp->piPlayer = PI_PLAYER_ON;
            }
            break;

            case CMD_RANDOM_PLAY: //'P'
                cout << "Paly All" << endl;
                //system("mpg123 -R --fifo /mnt/wsi/mp3fifo > /dev/null &");
                system("mpg123 -qR --fifo /mnt/wsi/mp3fifo > /mnt/wsi/plyStatus &");

                system("sleep 3");

                if (*(pShmSvr + 1) == '\0') {
                    system("cd /home/pi/music && ./piPlay &");
                } else if (*(pShmSvr + 2) == '\0') {
                    //            012345678 112345678 212345678 3123
                    memcpy(cmd, "cd /home/pi/music && ./piPlay  ", 40);
                    tmp = 30;
                    cmd[tmp++] = *(pShmSvr + 2);
                    /*cmd[33] = ' ';
                    cmd[34] = '5';
                    cmd[35] = '0';
                    cmd[36] = '0';*/
                    cmd[tmp++] = ' ';
                    cmd[tmp++] = '&';
                    cmd[tmp++] = '\0';
                    system(cmd);
                } else {
                    //            012345678 112345678 212345678 3123
                    memcpy(cmd, "cd /home/pi/music && ./piPlay  ", 40);
                    tmp = 30;
                    cmd[tmp++] = *(pShmSvr + 2);
                    cmd[tmp++] = ' ';
                    cmd[tmp++] = *(pShmSvr + 3);
                    cmd[tmp++] = ' ';
                    cmd[tmp++] = '&';
                    cmd[tmp++] = '\0';
                    system(cmd);
                }
                break;

            case CMD_REPLAY: //'>' // Playback
                if (bPlayer == true) {
                    if (pShmIp->playStatus == PI_MUSIC_PLAY_PAUSE) {
                        pShmIp->playStatus = PI_MUSIC_PLAY_PLAY;
                        system("echo \'pause\' >> /mnt/wsi/mp3fifo");
                    }
                } else {
                    printMsg(bLog, "player is not available");
                    pShmIp->playStatus = PI_MUSIC_PLAY_STOP;
                    //pShmIp->piPlayer = PI_PLAYER_OFF;
                }
                break;

            case CMD_STOP: //'.' // Stop
                //system("echo 'stop' >> /mnt/wsi/mp3fifo");
#ifdef RPI_DEMO_DAY_2017_06_17
                system("echo -l > /sys/class/ledControl/set/led"); //2017.6.16: Relay/Light Off
#endif
                if (bPlayer == true) system("echo \'q\' > /mnt/wsi/mp3fifo");
                else printMsg(bLog, "player is not available");

                pShmIp->musicPlayer = PI_MUSIC_PLAYER_OFF;
                //pShmIp->piPlayer = PI_PLAYER_OFF;
                pShmIp->playStatus = PI_MUSIC_PLAY_STOP;
                bPlayer = false;
                break;

            case CMD_SET_STOP_COUNT: break; // This command is taken care by piMusicClient.

            case 'T':
                if (bPlayer == true) system("mpg123 -R --fifo /mnt/wsi/mp3fifo > /dev/null &");
                else printMsg(bLog, "player is not available");
                break;

            case CMD_MUSIC_SVR_OFF: //'q'
#ifdef RPI_DEMO_DAY_2017_06_17
                system("echo -l > /sys/class/ledControl/set/led"); //2017.6.16: Relay/Light Off
#endif
                if (bPlayer == true) system("echo \'q\' > /mnt/wsi/mp3fifo");
                else printMsg(bLog, "player is not available");
                sleep(2);
                s = exec("pidof mpg123");
                if ((s != NULL) && (*s != '\0')) {
                    system("killall -9 mpg123");

                    s = exec("pidof mpg123");
                    if ((s != NULL) && (*s != '\0')) {
                        system("sudo killall -9 mpg123");
                    } else {
                        pShmIp->musicPlayer = PI_MUSIC_PLAYER_OFF;
                        pShmIp->playStatus = PI_MUSIC_PLAY_STOP;
                    }
                } else {
                    pShmIp->musicPlayer = PI_MUSIC_PLAYER_OFF;
                    pShmIp->playStatus = PI_MUSIC_PLAY_STOP;
                }

                bPlayer = false;

                /*sleep(2);

                s = exec("pidof piPlayer");

                if( (s != NULL) && (*s != '\0') ) {
                    system("killall -9 piPlayer");
                } else {
                    pShmIp->piPlayer = PI_PLAYER_OFF;
                }*/

                for (i = 0; i < 6; i++) {

                    if (pShmIp->piPlayer == PI_PLAYER_OFF) break;
                    usleep(500000);
                }

                *(pShmSvr) = '\0';

                pShmIp->musicSvr = PI_MUSIC_SVR_OFF;

                return 0;

            case CMD_VOLUME: //'v'
            {

                s = pShmSvr + 1;
                //            01234 5678 112345 678 212345678 312345678 4
                memcpy(cmd, "echo \'volume    \' >> /mnt/wsi/mp3fifo ", 40);

                tmp = 13;
                i = 0;

                while (*s != '\0') {
                    if (*s != CMD_ARG_DELIMITOR) {
                        if ( ((*s > '0') && (*s <= '9')) || (*s == '-') || (*s == '+') ) {
                            
                            cmd[tmp] = *(s++);
                            cmd[tmp + 1] = '%';

                            // check if the 2nd chracter in the 2n arguments is number in ASCII.
                            if ((*s >= '0') && (*s <= '9')) { 

                                cmd[tmp + 1] = *s;

                                if( cmd[tmp] == '-' ) { // it is mute command.
                                    // checi if it is already mute. If yes, then unmute instead.
                                    if( pShmIp->volume[SHM_UPPER_DIGIT_OF_2] == '-' ) { // it is already mute.
                                        cmd[tmp] = (pShmIp->vol / 10) + '0';
                                        cmd[tmp+1] = (pShmIp->vol % 10) + '0';
                                    }
                                }
                                cmd[tmp + 2] = '%';

                            } else if ( (*s == '+') || (*s == '-') ) {
                                
                                //cout << "vol1: " << (int)(pShmIp->vol) << endl;

                                if (*s == '+') {
                                    if (pShmIp->vol < 95) pShmIp->vol += 5;
                                    else pShmIp->vol = 99;
                                } else {
                                    if (pShmIp->vol > 95) pShmIp->vol = 95;
                                    else if (pShmIp->vol >= 5) pShmIp->vol -= 5;
                                    else pShmIp->vol = 0;
                                }

                                //cout << "vol2: " << (int)(pShmIp->vol) << endl;

                                if (pShmIp->vol < 0) {
                                    cmd[tmp] = '0';                                    
                                } else if (pShmIp->vol < 10) {
                                    cmd[tmp] = pShmIp->vol + '0';                                    
                                }  else {
                                    cmd[tmp] = (pShmIp->vol / 10) + '0';
                                    cmd[tmp+1] = (pShmIp->vol % 10) + '0';                                    
                                    cmd[tmp + 2] = '%';
                                }                            

                            }

                            if (bPlayer == true) {
                                system(cmd);
                                if (cmd[tmp + 1] == '%') {
                                    pShmIp->volume[SHM_UPPER_DIGIT_OF_2] = '0';
                                    pShmIp->volume[SHM_LOWER_DIGIT_OF_2] = cmd[tmp];
                                } else {
                                    pShmIp->volume[SHM_UPPER_DIGIT_OF_2] = cmd[tmp];
                                    pShmIp->volume[SHM_LOWER_DIGIT_OF_2] = cmd[tmp + 1];
                                }
                            } else {
                                printMsg(bLog, "The player \'mpg123\' is not available!");
                                //pShmIp->piPlayer = PI_PLAYER_OFF;
                                pShmIp->playStatus = PI_MUSIC_PLAY_STOP;
                            }
                            break;
                        } else {
                            printMsg(bLog, "Error! the volume you typed is not a number!");
                        }
                        break;
                    }

                    s++;
                    if (i++ > 10) {
                        printMsg(bLog, "Error! Wrong volume format has been typed!");
                        break;
                    }
                }
            }
            break;

            case CMD_ADD_NEW_FILES:
                AddNewSongListFiles();
                break;

            case CMD_CREATE_FILES:
                CreateSongListFiles();
                break;

            case CMD_DEBUG_CMD_HEAD:
                if (*(pShmSvr + 2) == CMD_ARG_GET_VER) {
                    cout << "Pims server build: " << __DATE__ << ":" << __TIME__ << "\t\t";
                }
                break;

            case '-':
                if (*(pShmSvr + 3) == 'h') {
                    system("echo \'********** How to use *********\'");
                    system("echo \'* List all songs: \"./ml.sh -l\", where \'l\' is the first letter of \'list\'");
                    system("echo \'* Play all song: ./ml.sh");

                    system("echo \'* Play song #3 to #10: ./ml.sh 3 10\'");

                    system("echo \'* Play song #5 only: ./ml.sh 5 5\'");

                    system("echo \'\'");


                    system("echo \'************* Keys ************\'");
                    system("echo \'* exit: Ctrl-c twice quickly.\'");
                    system("echo \'        Note: Ctrl-z doesn\t\'");
                    system("echo \'              kill processes.\'");
                    system("echo \'* previous song: Up arrow key\'");
                    system("echo \'* next song: Down arrow key\'");
                    system("echo \'* rewind: Left arrow key\'");
                    system("echo \'* fastforwrd: Right arrow key\'");
                    system("echo \'* volume cmd: amixer sset PCM,0 90%\'");
                    system("echo \'******************************\'");
                    system("echo \'\'");

                } else if (*(pShmSvr + 3) == 'l') {
                    system("echo \'********** Play List *************\'");
                }

                break;
            }

            *(pShmSvr) = '\0';
            *(pShmSvr + 1) = '\0';
        }

        usleep(sTime);
    }
    cout << "exiting piMusicServer" << endl;

    if (bLog == true) myfile.close();

    pShmIp->musicSvr = PI_MUSIC_SVR_OFF;

    //destructor called just before program exit
    return 0;
}




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
*********************************************************/
char* exec(const char* command, int idx, int delay) {
    FILE* fp;
    char* line = NULL;
    // Following initialization is equivalent to char* result = ""; and just
    // initializes result to an empty string, only it works with
    // -Werror=write-strings and is so much less clear.
    //char* result = (char*) calloc(1, 1);
    size_t len = 0;
    int i;
#ifdef DBG_PRINT_CMD
    if( pShmIp->logEn == PI_MUSIC_LOG_EN ) {
        printMsg( true, command);
    }
#endif
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

    for( i = 0, len = 0; i <= idx; i++ ) {
        getline(&line, &len, fp);
    }
    fflush(fp);

    if (pclose(fp) == -1) {
        perror("Cannot close stream.\n");
    }
    //return result;
    return line;
}



/*********************************************************
*
*
* Thread to pass bash command to the named fifo 'mp3fifo'.
* This function must be called by a thread only.
* Do not call this function from non-thread function!
*********************************************************/
void * SendCmdToPlayer(void * pCmd)
{
    char * pcmd = (char * ) pCmd;
    system(pcmd);
    //piPlayerCmd = 0;
    bCmdThdBusy = false;
    pthread_exit(NULL);
}
 
/*********************************************************
*
*
* It initiates a thread which will send a bash command to
* the named fifo, 'mp3fifo'.
* This function is good for any task that might be blocking
* or taking time to complete! 
*********************************************************/
int PiPlayerCmd( char * pCmd, bool bBusyCheck )
{
    int r;
    pthread_t id;

    if( pShmIp->logEn == PI_MUSIC_LOG_EN ) {
        printMsg( true, (const char *) pCmd);
    }

    if( bBusyCheck == true ) {
        for ( int i=0; i<25; i++) {
            //if( piPlayerCmd == 0 ) break;
            if( bCmdThdBusy == false ) break;
            else usleep(100000);
        }
    }

    if( bCmdThdBusy == false ) {
        //piPlayerCmd = 1;
        bCmdThdBusy = true;
        r = pthread_create( &piPlayerCmd, NULL, SendCmdToPlayer, (void *) pCmd );
        if(r != 0) {
            printf("Unable to create a thread to play music!\n");
            return (-1);
        }
        return 0;
    } 
   
    return (-1);
}


/*********************************************************
*
* This function get the Pi Music System ready.
* Such as mp3fifo ready, plyStatus file ready, and
* mp3 player ready.
* It takes time to get the mp3 player up and running.
* Therefore it should be called with special care.
**********************************************************/
int GetPiMusicSystemReady (void)
{
    int r, i;
    char cmd[40];

    r = PiPlayerCmd( (char*)  
       "if [ ! -e \'/mnt/wsi/mp3fifo\' ]; then"\
       "    mkfifo -m +rw /mnt/wsi/mp3fifo;"\
       "    if [ \"$?\" != \"0\" ]; then"\
       "        echo \'Fail to open /mnt/wsi/mp3fifo\';"\
       "        exit 1;"\
       "    fi; "\
       "fi;");
    //r = PiPlayerCmd( (char*) "echo 'fifo_start'; if [ ! -e '/mnt/wsi/mp3fifo' ]; then mkfifo -m +rw '/mnt/wsi/mp3fifo'; if [ \"$?\" != \"0\" ]; then  echo 'Fail to open /mnt/wsi/mp3fifo';  exit 1; fi; fi; echo 'fifo_end';");

    if ( r >= 0 ) {
            
        r = PiPlayerCmd( (char *)  
           "if [ ! -f \'/mnt/wsi/plyStatus\' ]; then"\
           "    touch /mnt/wsi/plyStatus;"\
           "    chmod a+w /mnt/wsi/plyStatus;"\
           "    if [ $? -ne 0 ]; then"\
           "        echo \'Fail to handle /mnt/wsi/plyStatus\';"\
           "        exit 1;"\
           "    fi;"\
           "fi;");

        if ( r < 0 ) {
            r -= 1;
        } else {

	    /*r = PiPlayerCmd( (char *)  
	       "echo 'mpg_start'; ply=\"$(pidof mpg123)\";"\
	       "if [ \"$ply\" == \"\" ]; then "\
	       "    $(mpg123 -qR --fifo '/mnt/wsi/mp3fifo' > /mnt/wsi/plyStatus &); "\
               "else"\
               "   echo \"$ply\";"\
	       "fi; echo 'mpg_end';");*/

            char * s = exec("pidof mpg123");

            if( (s == NULL) || (*s == '\0') ) {
                usleep(500000);

	        r = PiPlayerCmd( (char *)  "mpg123 -qR --fifo /mnt/wsi/mp3fifo > /mnt/wsi/plyStatus &");

            }

            //system("if [ ! -f \'/mnt/wsi/playlist.txt\' ]; then ls /home/pi/music/favorite/*.mp3 > /mnt/wsi/playlist.txt; fi;");
            //system("if [ ! -f \'/mnt/wsi/playlist.txt\' ]; then cp -fs /home/pi/music/playlist.lst /mnt/wsi/playlist.txt; fi;");

            if( r < 0) {
                r -= 2;
            } else {
                char * s;
                int i;
#if 1
                GetAllPimsFilesReady();
#else
		pShmIp->currentDir[SHM_UPPER_DIGIT_OF_2] = DEFAULT_CURRENT_DIR_NO_MSB;
                pShmIp->currentDir[SHM_LOWER_DIGIT_OF_2] = DEFAULT_CURRENT_DIR_NO_LSB;
                pShmIp->lastSong =  GetTotalSongsInPlayList();
#endif

                for( i = 0; i < 40; i++ ) {
                    s = exec("pidof mpg123");
                    if( (s != NULL) && (*s != '\0') ) {
                        pShmIp->musicPlayer = PI_MUSIC_PLAYER_ON;
                        pShmIp->musicSvr = PI_MUSIC_SVR_ON;
                        r = PiPlayerCmd( (char *) "echo \'SILENCE\' > /mnt/wsi/mp3fifo");
                        break;
                    }
                    usleep(200000);
                }

                if ( i >= 40 ) {
                    pShmIp->musicPlayer = PI_MUSIC_PLAYER_OFF;
                    pShmIp->musicSvr = PI_MUSIC_SVR_OFF;
                    r -= 3;
                } else {
                   usleep(100000);
                   s = exec( "cat /mnt/wsi/plyStatus | grep \'@silence\'");
                   if((s == NULL) || ( *s == '\0' )) PiPlayerCmd( (char *) "echo \'SILENCE\' >> /mnt/wsi/mp3fifo");

                    //            012345678 11 2345 678 212345678 312345678 4
                    memcpy( cmd, "echo volume \'  %\' >> /mnt/wsi/mp3fifo", 40);
                    cmd[13] = pShmIp->volume[0];
                    cmd[14] = pShmIp->volume[1];
                    system(cmd);
                }
            }
        }
    }

    /*
    PiPlayerCmd("echo $(date) > /mnt/wsi/plyStatus");

    PiPlayerCmd( (char *)  
       "cd /mnt/wsi; "
       "file='/mnt/wsi/playlist.txt'; "
       "if [ ! -f \"$file\" ]; then \
       "    ls /home/pi/music/favorite/*.mp3 > \"$file\"; "
       "fi");
    */

    return r;
}

/*********************************************************
*
*
* This thread pass in the indexed song name to the named
* pipe 'mp3fifo', so that it gets the song played with
* the mp3 player 'mpg123'. The song index number is located
* in a field of the shared memory ShmIpStatus. 
* This function must be called by thread only.
*********************************************************/
void * PlayMusic(void * shm)
{
    int r, ix, excnt;
    int cnt, shdwn;
    ShmIpStatus * pShmIp = (ShmIpStatus *) shm;


    if( pShmIp->musicPlayer == PI_MUSIC_PLAYER_OFF ) {

        pShmIp->piPlayer = PI_PLAYER_OFF;
        pShmIp->playStatus = PI_MUSIC_PLAY_STOP; 
        //piPlayerId = 0;
        bPiPlayerBusy = false;
        pthread_exit( (void *) (-1) );
    } else {

        if(  pShmIp->stopCounter > STOP_COUNTER_DISABLE ) {
            cnt =  pShmIp->stopCounter & MASK_STOP_COUNTER;
            shdwn = pShmIp->stopCounter & MASK_STOP_SHUTDOWN;
        } else {
            cnt = MAX_SONG_INDEX_NUM;
            shdwn = 0; 
        }

        pShmIp->piPlayer = PI_PLAYER_ON;
        pShmIp->playStatus = PI_MUSIC_PLAY_PLAY;

                    //01234 5678 112345678 212345678 312345678  412345678 512345678 612345678
        char ldM[] = "echo \'loadlist     /mnt/wsi/playlist.txt\' >> /mnt/wsi/mp3fifo";
        char * st;
        excnt = 0;
        while( ( pShmIp->musicPlayer == PI_MUSIC_PLAYER_ON ) && 
               ( cnt > 0 ) && 
               (excnt < M_SVR_MAX_PLAY_TIME_PER_SONG) ) {

            //if [ "$(pidof mpg123)" != "" ]; then
            ldM[15] = pShmIp->nextSong[0];
            ldM[16] = pShmIp->nextSong[1];        
            ldM[17] = pShmIp->nextSong[2];

            ix = SetNextSong( shm, 1 );
            //printf("ix:%d, %s\n", ix, ldM);
            // if ix == 1, then the scurrent song index in ldM[] is 0. Therefore musich must be stopped.
            if( ( ix <= 1) || (ix > pShmIp->lastSong ) ) {
                system("echo \'q\' >> /mnt/wsi/mp3fifo");
                pShmIp->musicPlayer = PI_MUSIC_PLAYER_OFF;
                system("if [ -f /mnt/wsi/pims.log ]; then "\
                       " echo \"Music stopped $(date): unable to set to the indexed song!\" >> /mnt/wsi/pims.log; " \
                       "fi");
                break;
            }
            system("echo > /mnt/wsi/plyStatus");

            if( ldM[15] == '0' ) {
                ldM[15] = ' ';

                if( ldM[16] == '0' ) {
                    ldM[16] = ' ';
                    if( ldM[17] == '0' ) {
                        ldM[17] = '1';
                    }
                }
            }

            system(ldM);

            excnt = 0;
            do {
               //TODO:: Apr. 1, 2017: Stop increamenting 'excnt' when player is paused by user. Reset 'excnt' when current song is restarted or changed to new song by user. Creating a flag in one of the shared memeory in order to let this thread know any changes which are in connection with this counter changes would be a good solution.
               if( (excnt++) > M_SVR_TIME_TO_CHECK_SONG_PLAYED ) { // check if there is no song get started in 6 minutes since the last play attempt.
                   st = exec( "cat /mnt/wsi/plyStatus | grep \'@P 2\'"); // Check if playing got started.
                   if( (st == NULL) || ( *st == '\0' ) || (excnt >  M_SVR_MAX_PLAY_TIME_PER_SONG) ) {
                       excnt = M_SVR_MAX_PLAY_TIME_PER_SONG;
                       system("if [ -f /mnt/wsi/pims.log ];"\
                              "  then echo \"Music stopped $(date): two long play time!\" >> /mnt/wsi/pims.log;" \
                              "fi");
                       break;
                   }
               }
 
               sleep(M_SVR_END_OF_SONG_CHECK_TIME); // 2 seconds.
               st = exec( "cat /mnt/wsi/plyStatus | grep \'@P 0\'"); // Check if current song ends.
            } while ( ((st == NULL) || ( *st == '\0' )) && (pShmIp->musicPlayer == PI_MUSIC_PLAYER_ON) );

#ifdef RPI_DEMO_DAY_2017_06_17
            system("echo -l > /sys/class/ledControl/set/led"); //2017.6.16: Relay/Light Off
#endif
         
            if(  pShmIp->stopCounter > STOP_COUNTER_DISABLE ) {
                if( (pShmIp->stopCounter & MASK_STOP_COUNTER) != cnt ) { // new stopCounter was set by user.
                    cnt =  pShmIp->stopCounter & MASK_STOP_COUNTER;
                    shdwn = pShmIp->stopCounter & MASK_STOP_SHUTDOWN;
                }

                if( excnt >= M_SVR_MAX_PLAY_TIME_PER_SONG ) cnt = 1; // Get it caused to either shutdown or stop further playing.

                if( cnt > 0) {
                    cnt--;
                    pShmIp->stopCounter = shdwn + cnt;
                } else cnt = 0;
            }  
        }
#ifdef RPI_DEMO_DAY_2017_06_17
        system("echo -l > /sys/class/ledControl/set/led"); //2017.6.16: Relay/Light Off
#endif
        if( pShmIp->musicPlayer != PI_MUSIC_PLAYER_ON ) {

            system("if [ -f /mnt/wsi/pims.log ];"\
                   "  then echo \"Music stopped $(date): no player found!\" >> /mnt/wsi/pims.log;" \
                   "fi");
        } 
        pShmIp->piPlayer = PI_PLAYER_OFF;
        //piPlayerId = 0;
        bPiPlayerBusy = false;

        if( cnt == 0 ) {
            system("cd /home/pi/music && sudo ./piMusicClient \'q\'");
            pShmIp->stopCounter = STOP_COUNTER_DISABLE;

            system("if [ -f /mnt/wsi/pims.log ];"\
                   "  then echo \"Music stopped $(date): stop counter reached to 0!\" >> /mnt/wsi/pims.log;" \
                   "fi");

            if( shdwn > 0 ) {
                sleep(1);
                //system("sudo shutdown -h now");
                system("sudo shutdown -h +1"); // '+1' means halt after 1 minute.
            }
        } else if ( excnt >= M_SVR_MAX_PLAY_TIME_PER_SONG ) { 
            system("cd /home/pi/music && sudo ./piMusicClient \'q\'");
        } 

        pShmIp->playStatus = PI_MUSIC_PLAY_STOP;

        pthread_exit(NULL);
    }
}


/*********************************************************
*
*
* This function gets the next song played.
* It starts the thread, 'PlayMusic' to get the music played.
* As soon as the indexed song gets played by the thread 'PlayMusic',
* It returns the song index number to the caller of this function.
* However, it doesn't play if the pi music system is not ready.
*********************************************************/
//int PlayNextSong( ShmIpStatus * shm, int idx )
int PlayNextSong( int idx, bool bForceTo )
{
    int r = 0;

    //ShmIpStatus * pShmIp = (ShmIpStatus *) shm;
    
    if( pShmIp->musicPlayer == PI_MUSIC_PLAYER_ON ) {
        
        idx = SetNextSong( pShmIp, ((bForceTo == true)?idx:idx-1), bForceTo); // nextSong[] in the shared memory has next song index already.
 
        if( bPiPlayerBusy == false) {
            r = pthread_create( &piPlayerId, NULL, PlayMusic, (void *) pShmIp );
            if(r != 0) {
                printf("Unable to create a thread to play music!\n");
                r = -3;
            }
        } else r = -2;
    } else r = -1;

    return (( r < 0 )? r : idx); 
}

/*********************************************************
*
*
* This function is a replace of the bash script 'piPlayer'.
* It starts the thread, 'PlayMusic' to get the music played.
*********************************************************/
void PiPlayerOnOff(ShmIpStatus * shm, bool bOn, int start, int end)
{
    int r;

    ShmIpStatus * pShmIp = (ShmIpStatus *) shm;
    
    if( bOn == true ) {
        
        GetPiMusicSystemReady();

        SetNextSong( shm, start );
 
        if( bPiPlayerBusy == false) {
            r = pthread_create( &piPlayerId, NULL, PlayMusic, (void *) shm );
            if(r != 0) {
                printf("Unable to create a thread to play music!\n");
                exit(-1);
            }
        }
    }  
}


/*********************************************************
*
*
*********************************************************/
int GetNextSongIdxInShm(void * shm)
{
    int i;
    ShmIpStatus * pShmIp = (ShmIpStatus *) shm;
    
    i = pShmIp->nextSong[2] - '0';
    i += (pShmIp->nextSong[1] - '0') * 10;
    i += (pShmIp->nextSong[0] - '0') * 100;

    //i--;

    //if ( i == 0 ) i = 1;

   return i;
}


/*********************************************************
*
* It sets next song to play by updating nextSong[] in the
* shared memory. If the index number passed in is greater
*
* Aargs:
* void * shm: pointer to the shared memory ShmIpStatus.
* int idx: relative or absolute song index number depending
*    on following boolean value.
* bool bForceTo:
*    - false: use 'idx' as relative next song index; it
*             gets the 'idx + current song index' set to
*             the shared memory 'nextSong[]'. It is the
*             default value.
*    - true:  use 'idx' as absolute next song index; it
*             gets the 'idx' set to 'nextSong[]'.
*            
* Returns: int: song index number which is set to nextSong[].
*
* Usage:
* * get the current song played again: idx = 0, bForce = false.
* * get the song following current song played: idx = 1, bForce = false.
* * get the 10th song in the playlist played: idx = 10, bFource = true.
*
*********************************************************/
int SetNextSong(void * shm, int idx, bool bForceTo)
{
    int i;
    char r[3];
    ShmIpStatus * pShmIp = (ShmIpStatus *) shm;
    
    if( bForceTo == false ) {
        i = GetNextSongIdxInShm( shm );
        //printf("current: %d, add: %d\n", i, idx);
        if( (MAX_SONG_INDEX_NUM - (i + idx)) <= 0 ) idx = 0;
        else idx += i;
    }

    i = idx;

    r[2] = (char) (( i % 10 ) + '0' );
    i /= 10;
    r[1] = (char) (( i % 10 ) + '0' );
    r[0] = (char) (( i / 10 ) + '0' );

    pShmIp->nextSong[0] = r[0];
    pShmIp->nextSong[1] = r[1];
    pShmIp->nextSong[2] = r[2];

    //printf("nextSong[]: %s\n", &(pShmIp->nextSong[0] ));

    return idx;
}



/*********************************************************
*
*
*********************************************************/
void SetToNextSong(void * shm)
{
    ShmIpStatus * pShmIp = (ShmIpStatus *) shm;

    // Increase next song index by 1. It it is "999", set it to "000".
    if( pShmIp->nextSong[2] < '9' ) pShmIp->nextSong[2] += 1;
    else {
        pShmIp->nextSong[2] = '0';
        if( pShmIp->nextSong[1] < '9') pShmIp->nextSong[1] += 1;
        else {
            pShmIp->nextSong[1] = '0';
            if( pShmIp->nextSong[0] < '9') pShmIp->nextSong[0] += 1;
            else { 
                pShmIp->nextSong[0] = '0';
                pShmIp->nextSong[2] = '1';
            }
        }
    }
}


/*********************************************************
*
*
*********************************************************/
void SetToSongBeingPlayed(void * shm)
{
    ShmIpStatus * pShmIp = (ShmIpStatus *) shm;

    // Increase next song index by 1. It it is "999", set it to "000".
    if( pShmIp->nextSong[2] > '0' ) pShmIp->nextSong[2] -= 1;
    else {
        pShmIp->nextSong[2] = '9';
        if( pShmIp->nextSong[1] > '0') pShmIp->nextSong[1] -= 1;
        else {
            pShmIp->nextSong[1] = '9';
            if( pShmIp->nextSong[0] > '0') pShmIp->nextSong[0] -= 1;
            else { 
                pShmIp->nextSong[0] = '9';
            }
        }
    }
}

/*********************************************************
*
*
*********************************************************/
int CreateSongListFiles(void)
{
    system("cd /home/pi/music; ls -d */ | tr -d \'/\' > /home/pi/music/dirlist.lst");
    system("cat /home/pi/music/dirlist.lst | "\
           "while read line ; "\
           "do"\
           " ls /home/pi/music/$line/*.mp3 > /home/pi/music/$line.lst; "\
           " if [ ! -s /home/pi/music/$line.lst ]; then rm /home/pi/music/$line.lst; fi; "\
           "done");
    system("cp -fs /home/pi/music/dirlist.lst /mnt/wsi/dirlist.lst");
    //system("cp -f /home/pi/music/\"$(sed -n \'1p\' < /mnt/wsi/dirlist.lst).lst\" /home/pi/music/playlist.lst");
    //system("cp -fs /home/pi/music/playlist.lst /mnt/wsi/playlist.txt");
    //system("cd /home/pi/music; cp -fs \"$(sed -n \'  p\' < dirlist.lst).lst\" /mnt/wsi/playlist.txt");
    GetAllPimsFilesReady();
    return 0;
}

/*********************************************************
*
*
*********************************************************/
int AddNewSongListFiles(void)
{
    system("cd /home/pi/music; ls -d */ | tr -d \'/\' > /home/pi/music/dirlist.lst");
    system("cat /home/pi/music/dirlist.lst |"\
           "while read line ; "\
           "do"\
           " if [ ! -f /home/pi/music/$line.lst ]; then"\
           "   ls /home/pi/music/$line/*.mp3 > /home/pi/music/$line.lst;"\
           "   if [ ! -s /home/pi/music/$line.lst ]; then rm /home/pi/music/$line.lst; fi;"\
           " fi; "\
           "done");
    system("cp -fs /home/pi/music/dirlist.lst /mnt/wsi/dirlist.lst");
    //system("cp -f /home/pi/music/\"$(sed -n \'1p\' < /mnt/wsi/dirlist.lst).lst\" /home/pi/music/playlist.lst");
    //system("cp -fs /home/pi/music/playlist.lst /mnt/wsi/playlist.txt");
    //system("cd /home/pi/music; cp -fs \"$(sed -n \'  p\' < dirlist.lst).lst\" /mnt/wsi/playlist.txt");
    GetAllPimsFilesReady();
    return 0;
}



// *******************************************************
/*********************************************************
*
*
* This is a function which returns the number of song in
* the playlist.txt.
*********************************************************/
unsigned char GetTotalSongsInPlayList(void)
{
    int i;
    char * s;

    s = exec("wc -l /mnt/wsi/playlist.txt | cut -d \' \' -f1");
    i = 0; 
    if( (s != NULL) && (*s != '\0') ) {
        while (*s != '\0') {
            if( (*s >= '0') && (*s <= '9') ) {
                i *= 10;
                i += (*s - '0');
                s++;
            } else {
                //i = 0;
                break;
            }
        }
    }

    if( (i == 0) || (i > MAX_SONG_INDEX_NUM) ) {
        i = MAX_SONG_INDEX_NUM; 
        system("if [ -f /mnt/wsi/pims.log ]; then "\
               " echo \"Fail to count the number of song in the playlist - $(date)!\" >> /mnt/wsi/pims.log; " \
               "fi");
    }
    return ((unsigned char) i );
}



/*********************************************************
*
*
* This function will get all nessary files for PIMS ready.
* Such as plyStatus, playlist.txt, dirlist.lst, songs.txt,
* and even mp3fifo.
*********************************************************/
unsigned char GetAllPimsFilesReady(void)
{
    unsigned char rlt = 0, i;
    char ch;
    char * s;

    s = exec( (char*)  
        "if [ ! -e \'/mnt/wsi/mp3fifo\' ]; then"\
        "    mkfifo -m +rw /mnt/wsi/mp3fifo;"\
        "    if [ \"$?\" != \"0\" ]; then"\
        "        echo \'Fail to open /mnt/wsi/mp3fifo\' >> /mnt/wsi/error.log;"\
        "        exit 1;"\
        "    fi; "\
        "fi; "\
        "echo \'OK\';");

    if( (s == NULL) || ( *s == '\0') ) {
        rlt = MASK_MP3FIFO_MISSING;
    }

           
    s = exec( (char *)  
        "if [ ! -f \'/mnt/wsi/plyStatus\' ]; then"\
        "    touch /mnt/wsi/plyStatus;"\
        "    chmod a+w /mnt/wsi/plyStatus;"\
        "    if [ $? -ne 0 ]; then"\
        "        echo \'Fail to handle /mnt/wsi/plyStatus\' >> /mnt/wsi/error.log;"\
        "        exit 1;"\
        "    fi;"\
        "fi; "\
        "echo \'OK\'");

    if( (s == NULL) || ( *s == '\0') ) {
        rlt += MASK_PLYSTATUS_MISSING;
    }


    s = exec( "if [ -f \'/mnt/wsi/playlist.txt\' ]; then echo \'Y-\'; else echo \'N-\'; fi;" );

    if( (s == NULL) || ( *s == '\0') ) {
        s = exec( "if [ -f \'/mnt/wsi/playlist.txt\' ]; then echo \'Y-\'; else echo \'N-\'; fi;" );
    }
    
    if ( s == NULL ) {
        system( "echo \'Fail to execute System() from pims Server.\' >> /mnt/wsi/error.log");
    }

    if( (s==NULL) || (*s == 'N') ) {
                    //012345678 112345678 21 2345678 31 2345 678 412345678
        char cmd[] = "cp -fs /home/pi/music/\"$(sed -n \'  p\' < /home/pi/music/dirlist.lst).lst\" /mnt/wsi/playlist.txt";
        cmd[33] = pShmIp->currentDir[SHM_UPPER_DIGIT_OF_2];
        cmd[34] = pShmIp->currentDir[SHM_LOWER_DIGIT_OF_2];
        system((const char *) cmd);

        s = exec( "if [ -f \'/mnt/wsi/playlist.txt\' ]; then echo \'Y-\'; else echo \'N-\'; fi;" );

        if( (s == NULL) || ( *s == '\0') ) {
            s = exec( "if [ -f \'/mnt/wsi/playlist.txt\' ]; then echo \'Y-\'; else echo \'N-\'; fi;" );
        }
        if ( (*s != 'Y') || ( *(s+1) != '-') ) {
            if ( s == NULL ) {
                system( "echo \'Fail to execute System() from pims Server.\' >> /mnt/wsi/error.log");
            } else {
               system("echo \'Fail to create /mnt/wsi/playlist.txt\' >> /mnt/wsi/error.log;");
            }
            rlt += MASK_PLAYLIST_MISSING;
        }
    }



    if( (rlt & MASK_PLAYLIST_MISSING) == 0 ) { // There is playlist.txt.
        s = exec( (char *)  
            "if [ ! -f \'/mnt/wsi/songs.txt\' ]; then"\
            "    cat /mnt/wsi/playlist.txt | cut -d \'/\' -f6 > /mnt/wsi/songs.txt;"\
            "    if [ $? -ne 0 ]; then"\
            "        echo \'Fail to create /mnt/wsi/songs.txt\' >> /mnt/wsi/error.log;"\
            "        exit 1;"\
            "    fi;"\
            "fi; "\
            "echo \'OK\'");

        if( (s == NULL) || ( *s == '\0') ) {
            rlt += MASK_SONGS_MISSING;
        }

        pShmIp->lastSong =  GetTotalSongsInPlayList();

        // Update current directory index number in the shared memory.
        s = exec("cd /home/pi/music; "\
                 "cat dirlist.lst | grep -n \"$(ls -l /mnt/wsi/playlist.txt | cut -d \'/\' -f8 | cut -d \'.\' -f1)\" | cut -d \':\' -f1;");

        i = 0;
        if( (s != NULL) && (*s != '\0') ) {
            if( (*s >= '0') && (*s <= '9') ) {
                ch = *s;
                i++;                       
                s++;
                if( (*s >= '0') && (*s <= '9') ) {
                    i++;
                }// else if ( *s != '\0' ) i = 0;
            }
        }

        if( i == 2 ) { 
            pShmIp->currentDir[SHM_UPPER_DIGIT_OF_2] = ch;
            pShmIp->currentDir[SHM_LOWER_DIGIT_OF_2] = *s;
        } else if ( i == 1 ) {
            pShmIp->currentDir[SHM_UPPER_DIGIT_OF_2] = '0';
            pShmIp->currentDir[SHM_LOWER_DIGIT_OF_2] = ch;
        } else {
            pShmIp->currentDir[SHM_UPPER_DIGIT_OF_2] = DEFAULT_CURRENT_DIR_NO_MSB;
            pShmIp->currentDir[SHM_LOWER_DIGIT_OF_2] = DEFAULT_CURRENT_DIR_NO_LSB;
        //pShmIp->lastSong =  GetTotalSongsInPlayList();
            system("if [ -f /mnt/wsi/pims.log ]; then "\
                   " echo \"Music directory index number is too big - $(date)!\" >> /mnt/wsi/pims.log; " \
                   "fi");
        }
    }

    return rlt;
}


/*********************************************************
*
*
*********************************************************/
#ifdef USE_KBHIT 
int kbhit(void)
{
  struct timeval tv;
  fd_set read_fd;

  // Do not wait at all, not even a microsecond
  tv.tv_sec=0;
  tv.tv_usec=0;

  // Must be done first to initialize read_fd 
  FD_ZERO(&read_fd);

  // Makes select() ask if input is ready:
  // 0 is the file descriptor for stdin 
  FD_SET(0,&read_fd);

  /; The first parameter is the number of the
  //  largest file descriptor to check + 1.
  if(select(1, &read_fd,NULL, /*No writes*/NULL, /*No exceptions*/&tv) == -1)
    return 0;  // An error occured

  /*  read_fd now holds a bit map of files that are
   * readable. We test the entry for the standard
   * input (file 0). */
  
if(FD_ISSET(0,&read_fd))
    /* Character pending on stdin */
    return 1;

  /* no characters were pending */
  return 0;
}
#endif
