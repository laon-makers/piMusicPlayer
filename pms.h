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
 * Filename: piMusicServer.h (pms.h)
 * Main header that helps to build piMusicSystem. It is currently
 * referenced by piMusicClient.c(pmc.c), piMusicServer.c(pms.c), and
 * ipStatus.c(is.c). 
 * History:
   <2017>
   Jan. 29: 1. piMusicServer.h has been replaced with pms.h.
 * *******************************************************************/

#ifndef PI_MUSIC_SERVER_H
#define PI_MUSIC_SERVER_H


#define RPI_DEMO_DAY_2017_06_17 // Define it for some sepcial function for the demo,
                                // for instnace, light control with the relay.


#define M_SVR_SLEEP_TIME        100000  // micro-seconds. must be smaller than 1,000,000.
#define M_SVR_DEEP_SLEEP_TIME   300000  // micro-seconds
//#define DEEP_SLEEP_TIME         1 
#define M_SVR_END_OF_SONG_CHECK_TIME    2  // in second.
#define M_SVR_MAX_PLAY_TIME_PER_SONG    (600/M_SVR_END_OF_SONG_CHECK_TIME) // 600 seconds for 10 minutes.
#define M_SVR_TIME_TO_CHECK_SONG_PLAYED (360/M_SVR_END_OF_SONG_CHECK_TIME) // 360 seconds for 6 minutes.

#define RESULT_OK           0
#define RESULT_NG           1


#define SHM_ID_MUSIC_SVR    0x333333
#define SHM_ID_IP_STATUS    0x999999
#define SHM_IP_STATUS_SIZE  (sizeof(ShmIpStatus)) 
#define SHM_SEVER_SIZE      10

#define PIMS_STATUS_RPT_DELIMITER '`' // This delimiter is for Android PIMS controller app.
                                      // It is used as delimiter in the app. when gets 
                                      // connection response from this PIMS.

// Following characters are information used on  shared memory fields.
#define PI_MUSIC_CMD_INIT   ' '
#define PI_MUSIC_PLAY_PLAY  '>'
#define PI_MUSIC_PLAY_STOP  '.'
#define PI_MUSIC_PLAY_PAUSE '|'
#define VOLUME_DEFAULT_MSB  '5' // Make sure this default volume is the same value as DFT_VOLUME.
#define VOLUME_DEFAULT_LSB  '0'
#define PI_MUSIC_PLAY_OFF   'f'
#define PI_MUSIC_SVR_ON     'S'
#define PI_MUSIC_SVR_OFF    'F'
#define PI_PLAYER_ON        'P'
#define PI_PLAYER_OFF       'F'
#define PI_MUSIC_PLAYER_ON  'M'
#define PI_MUSIC_PLAYER_OFF 'F'
#define PI_MUSIC_LOG_EN     'L'
#define PI_MUSIC_LOG_DIS    'D'


#define MAX_CMD_STR_LEN      50   // Max length of command string.
#define MAX_CMD_STR_LEN_SVR  120  // Max length of command string for pims server.
#define MAX_FNAME_STR_LEN_SVR 50  // Max length of file/folder name string for pims server.
// List of commands to use control pi music system and music player.
#define CMD_CTRL_CMD_HEAD   '*'   // This command head is alwasy used together with
                                  // following sub-command and used to indicate that
                                  // the followed sub-command is the command mainly for 
                                  // ipStatus application.
#define CMD_DEBUG_CMD_HEAD  '\?'
#define CMD_HOMEAUTO_CMD_HEAD 'H' // Home Automation Command Header
#define CMD_MUSIC_SVR_ON    '~'   // Dereferenced by ipStatus and piMusicServer

#define CMD_MUSIC_SVR_OFF   'q'   // Dereferenced by ipStatus and piMusicServer

                                  // Dereferenced by ipStatus and piMusicServer
#define CMD_LOG_EN          PI_MUSIC_LOG_EN        // 'L'
#define CMD_PAUSE           PI_MUSIC_PLAY_PAUSE    // '|'
#define CMD_FFWD            'f'
#define CMD_RWND            'r'
#define CMD_NEXT_SONG       'n'
#define CMD_PREV_SONG       'p'
#define CMD_GET_INFO        'i'

#define CMD_CONNECT         ':'  // Dereferenced by ipStatus and piMusicServer
#define CMD_PLAY            's'
#define CMD_PLAY_NEW_LIST   'S'
#define CMD_REPLAY          PI_MUSIC_PLAY_PLAY     // '>'
#define CMD_STOP            PI_MUSIC_PLAY_STOP     // '.'
#define CMD_RANDOM_PLAY     'P'
#define CMD_GET_PLAY_LIST   '-'  
#define CMD_GET_NEW_PLAY_LIST '='
#define CMD_GET_MUSIC_DIR   '/'
#define CMD_SET_MUSIC_DIR   '\\'
#define CMD_VOLUME          'v'
#define CMD_REBOOT          'R'  // This command following CMD_CTRL_CMD_HEAD is
                                 // used to get the piMusicServer machine rebooted.
#define CMD_SHUTDOWN        '0'  // This is number zero in a ASCII character; not an alphabet.
                                 // This command following CMD_CTRL_CMD_HEAD is used to get the
                                 // piMusicServer machine shutdown.

#define CMD_SET_STOP_COUNT  'd'

#define CMD_ARG_DELIMITOR   ' '
#define CMD_ARG_NO_SHUTDOWN 'N'
#define CMD_ARG_GET_SHM     'm'
#define CMD_ARG_GET_VER     'v'
#define CMD_ADD_NEW_FILES   'a'   // It gets missing playlists created to default music folder
                                  // which is the parent folder of music folders. 
#define CMD_CREATE_FILES    'c'   // It gets all playlists created/recreated to default music 
                                  // folder which is the parent folder of music folders.

#define PLAY_MODE_SINGLE    '1'   // Play only one song specified.
#define PLAY_MODE_ONCE      'O'   // Play songs in the play list only once.
#define PLAY_MODE_REPEAT    'R'   // Play each song in the play list once and repeat it.
#define PLAY_MODE_SHUFFLE   'S'   // Play songs in the play list randomly. Stop playing if each song was played.

#define CMD_HA_ARG_LIGHT_ON 'L'
#define CMD_HA_ARG_LIGHT_OFF 'l'


#define STOP_TIMER_MUSIC_STOP 0
#define STOP_TIMER_DISABLE   -1 
#define STOP_COUNTER_DISABLE -1
#define MASK_STOP_SHUTDOWN   0x0800
#define MASK_STOP_TIMER      0x00FF
#define MASK_STOP_COUNTER    0x00FF
#define STOP_SHUTDOWN_EN_INX 'S'

#define SHM_UPPER_DIGIT_OF_2  0
#define SHM_LOWER_DIGIT_OF_2  1
#define SHM_UPPER_DIGIT_OF_3  0
#define SHM_MIDDLE_DIGIT_OF_3 1
#define SHM_LOWER_DIGIT_OF_3  2

#define MAX_SONG_INDEX_NUM            255
#define DEFAULT_NEXT_SONG_NO_MSB      '0'
#define DEFAULT_NEXT_SONG_NO_MDB      '0'  // MDB: MiDdle Byte.
#define DEFAULT_NEXT_SONG_NO_LSB      '1'
#define DFT_NEXT_SONG_IDX              1
#define DEFAULT_CURRENT_DIR_NO_MSB    '0'
#define DEFAULT_CURRENT_DIR_NO_LSB    '1'
#define DFT_CURRENT_DIR_IDX            1
#define DEFAULT_PLAY_MODE             PLAY_MODE_ONCE
#define DFT_VOLUME                     50
#define DEFAULT_STOP_TIMER            STOP_TIMER_DISABLE
#define DEFAULT_STOP_COUNTER          STOP_COUNTER_DISABLE


// Note: 
//   Make sure to add default value into following Macro if you added new member into the structure ShmIpStatus.
#define SET_SHM_IP_STATUS_DEFAULT(ptr); {\
        ptr->cmd = PI_MUSIC_CMD_INIT;\
        ptr->playStatus = PI_MUSIC_PLAY_STOP;\
        ptr->volume[0] = VOLUME_DEFAULT_MSB;\
        ptr->volume[1] = VOLUME_DEFAULT_LSB;\
        ptr->musicSvr = PI_MUSIC_SVR_OFF;\
        ptr->piPlayer = PI_PLAYER_OFF;\
        ptr->musicPlayer = PI_MUSIC_PLAYER_OFF;\
        ptr->logEn = PI_MUSIC_LOG_DIS;\
        ptr->mode = DEFAULT_PLAY_MODE;\
        ptr->nextSong[0] = DEFAULT_NEXT_SONG_NO_MSB;\
        ptr->nextSong[1] = DEFAULT_NEXT_SONG_NO_MDB;\
        ptr->nextSong[2] = DEFAULT_NEXT_SONG_NO_LSB;\
        ptr->currentDir[0] = DEFAULT_CURRENT_DIR_NO_MSB;\
        ptr->currentDir[1] = DEFAULT_CURRENT_DIR_NO_LSB;\
        ptr->end = '\0';\
        ptr->vol = DFT_VOLUME;\
        ptr->nextIdx = DFT_NEXT_SONG_IDX;\
        ptr->dirIdx = DFT_CURRENT_DIR_IDX;\
        ptr->lastSong = MAX_SONG_INDEX_NUM;\
        ptr->stopTimer = DEFAULT_STOP_TIMER;\
        ptr->stopCounter = DEFAULT_STOP_COUNTER;\
    }

// Note: Make sure followings!!!
//  - that data for any member above the member 'end' of this structure is printerble values;
//    Only members of which values need be neither informed to Android piMusicPlayer app nor printed
//    when logging is enabled on file or console can be placed below the member 'end'.
//  - to add default value into the Macro SET_SHM_IP_STATUS_DEFAULT(ptr) if you added new member.
typedef struct ShmIpStatusTag {
    char cmd;
    char playStatus;
    char volume[2];
    char musicSvr;      // On/off status of piMusicServer program.
    char piPlayer;      // piPlayer script is no longer used since Jan. 16, 2017, instead
                        // it represents PlayMusic thread from the piMusicServer program.
    char musicPlayer;   // On/off status of mpg123 music player.
    char logEn;
    char mode;          // It is for play mode such as 'repeat', 'once', 'shuffle', etc.
                        // It has been added on Jan.24, 2017.
    char nextSong[3];   // BCD. make sure the number of total songs in the playlist.txt is less than 1,000. 
    char currentDir[2]; // BCD. make sure that the number of directory is less than 100.

    char end;           // Members above this line must have printable data as its values.
                        // Members which are not exposed to external app that receives member data as string
                        // should be placed below this line. It is because this member used as string terminal.
    signed char vol;    // Current volume. The same value as volume[] but in integer.
    unsigned char nextIdx;  // Next song indication. The same value as nextSong[] but in integer.
    unsigned char dirIdx;   // current directory index. The same value as currentDir[] but in integer.
    unsigned char lastSong; // Last song index number in the playlist.txt.
    int stopTimer;      // Stop timer in minute. Music will be being played until it reaches to 0.
                        // Set it to STOP_TIMER_DISABLE to disable this timer.
    int stopCounter;    // Count down until it reaches to 0 by which the music player stops.
                        // 12th bit from LSB is used to indicate whether shutdown is followed.
                        // if the bit is 1, then it must be shutdown once the counter reaches 0.
                        // Otherwise it just gets the music player stopped.
} ShmIpStatus;

#endif
