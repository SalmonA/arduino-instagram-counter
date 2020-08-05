#include <MD_MAX72xx.h>
#include <SPI.h>
#include <stdlib.h>
#include <string.h>
#define PRINT(s, v) { Serial.print(F(s)); Serial.print(v); }


#include "InstagramStats.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "JsonStreamingParser.h"
// Used to parse the Json code within the library
// Available on the library manager (Search for "Json Streamer Parser")
// https://github.com/squix78/json-streaming-parser

/*
*
* CONFIGURATION ------------------------------------------------------------
*
*/


// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CLK_PIN   25  // or SCK
#define DATA_PIN  27  // or MOSI
#define CS_PIN    26  // or SS


char ssid[] = "SSID";             // your network SSID (name)
char password[] = "password"; // your network key
String userName = "username";    // from their instagram url
unsigned long delayBetweenChecks = 300000; //mean time between api requests (ms)

/*
 * -----------------------------------------------------------------------
 */

WiFiClientSecure client;
InstagramStats instaStats(client);
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Text parameters
#define CHAR_SPACING  1 // pixels between characters

// Global message buffers shared by Serial and Scrolling functions
#define BUF_SIZE  8
char message[BUF_SIZE];
bool newMessageAvailable = true;
char char_arr [100];
int instagramFollowers = 0;
int instagramLikes = 0;
unsigned long whenDueToCheck = 0;



void printText(uint8_t modStart, uint8_t modEnd, char *pMsg)
// Print the text string to the LED matrix modules specified.
// Message area is padded with blank columns after printing.
{
  uint8_t   state = 0;
  uint8_t   curLen;
  uint16_t  showLen;
  uint8_t   cBuf[8];
  int16_t   col = ((modEnd + 1) * COL_SIZE) - 1;

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  do     // finite state machine to print the characters in the space available
  {
    switch(state)
    {
      case 0: // Load the next character from the font table
        // if we reached end of message, reset the message pointer
        if (*pMsg == '\0')
        {
          showLen = col - (modEnd * COL_SIZE);  // padding characters
          state = 2;
          break;
        }

        // retrieve the next character form the font file
        showLen = mx.getChar(*pMsg++, sizeof(cBuf)/sizeof(cBuf[0]), cBuf);
        curLen = 0;
        state++;
        // !! deliberately fall through to next state to start displaying

      case 1: // display the next part of the character
        mx.setColumn(col--, cBuf[curLen++]);

        // done with font character, now display the space between chars
        if (curLen == showLen)
        {
          showLen = CHAR_SPACING;
          state = 2;
        }
        break;

      case 2: // initialize state for displaying empty columns
        curLen = 0;
        state++;
        // fall through

      case 3:	// display inter-character spacing or end of message padding (blank columns)
        mx.setColumn(col--, 0);
        curLen++;
        if (curLen == showLen)
          state = 0;
        break;

      default:
        col = -1;   // this definitely ends the do loop
    }
  } while (col >= (modStart * COL_SIZE));

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}


void getInstagramStatsForUser() {
  Serial.println("Getting instagram user stats for " + userName );
  InstagramUserStats response = instaStats.getUserStats(userName);
  Serial.println("Response:");
  Serial.print("Number of followers: ");
  Serial.println(response.followedByCount);
  Serial.print("Number of likes: ");
  Serial.println(response.likedByCount);
  instagramFollowers = response.followedByCount;
  instagramLikes =  response.likedByCount;
  }


void setup()
{

  Serial.begin(57600);
  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
  mx.begin();

  mx.control(MD_MAX72XX::INTENSITY, 1);
  
}

bool like = false;

void loop()
{
  unsigned long timeNow = millis();
  if ((timeNow > whenDueToCheck))
  {
    getInstagramStatsForUser();
    if (like)
    {
      sprintf(char_arr, "%s", "\x03");
      sprintf(char_arr+1, "%d", instagramLikes);
      printText(0, MAX_DEVICES-1, char_arr);
      whenDueToCheck = timeNow + (delayBetweenChecks/4);
    }
    else
    {
       sprintf(char_arr, "%s", "\x0ae");
       sprintf(char_arr+1, "%d", instagramFollowers);
       printText(0, MAX_DEVICES-1, char_arr);
       whenDueToCheck = timeNow + delayBetweenChecks;
    }
    like = !like;
  }
}
