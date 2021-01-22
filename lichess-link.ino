

/*
Lichess - link V.1.0.0. - ALPHA

This sketch connects an Arduino Uno Wifi Rev2 to the lichess api 
and allows you to play a correspondance or random opponent chess game 
on your arduino using a LCD display shield with input buttons. 

Please see Github page for more thorough documentation : 
https://github.com/Kzra/Lichess-Link

Acknowledgements: 
Several chunks of code were repurposed from 
the WiFiNiNA SSLClient example 
and from the Jsonhttpclient Arduino Json example.

Copyright MIT (C) Ezra Kitson 2021.
*/

//Libraries
#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include "arduino_secrets.h" 
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <ArduinoJson.h>

//Process Secret Data (Type Secret Data into arduino_secrets.h)
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
char token[] = SECRET_TOKEN; // your lichess API token

// lcd shield variables
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
#define OFF 0x0
#define ON 0x1 //backlight on 

//user input variables and constants
char uci[] = {' ',' ',' ',' ',0};
boolean select = false; 
int coordIndex = 0;
int firstCoord = 0;
int secondCoord = 0;
int thirdCoord = 0;
int fourthCoord = 0;
int alphaNumIndex[37] = { 0 };
int clockTime = 120; 
int clockIncrement = 30; 
int colourIndex =0;
char corrUser[16] = {'\0'};
const char columns[]= {' ','a','b','c','d','e','f','g','h'};
const char rows[] = {' ','1','2','3','4','5','6','7','8'};
const char alphaNumeric[] = {'\0','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9'};

// Wifi variables
int keyIndex = 0;            // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;
char server[] = "lichess.org";    // name address for lichess (using DNS)
WiFiSSLClient client; // WIFISSLClient always connects via SSL (port 443 for https)

//lichess variables 
const char* username;
const char* currentGameID;
const char* previousGameID;
const char* lastMove;
const char* myColour; 
const char* opponentName;
const char* opponentColour;
const char* moveError;
const char* winner;
const char* endStatus;
const char* challengeID;  
const char* preMove;
const char* selectedColour;
boolean myTurn = false;
boolean moveSuccess = false;
boolean gameInit = true;
boolean challengeSent = false;
const int delayTime = 6000; //the time to wait for lichess.org to respond to an http request

// loop variables
boolean MODE0 = true; 
boolean MODE1 = false;
int modeIndex = 0;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // turn on LCD backlight 
  lcd.setBacklight(ON);
  // display welcome message
  lcd.setCursor(0,0);
  lcd.print("Lichess Link");
  lcd.setCursor(0,1);
  lcd.print("v 1.0.0.");
  delay(1000);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Wifi Failed");
    // don't continue
    while (true);
  }
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  lcd.setCursor(0,0);
  lcd.print("Attempting wifi");
  lcd.setCursor(0,1);
  lcd.print("connection ...");
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to wifi");
  // update lcd display
  printWiFiStatus();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Wifi good!");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Contacting"); 
  lcd.setCursor(0,1);
  lcd.print("lichess.org ...");
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 443)) {
     Serial.println("connected to server in setup");
     // SETUP API: MAKE A REQUEST TO DOWNLOAD THE CURRENT USER'S LICHESS USERNAME
     client.println("GET /api/account HTTP/1.1");
     client.println("Host: lichess.org");
     // Include an authorisation header with the lichess API token
     client.print("Authorization: Bearer ");
     client.println(token);
     delay(delayTime); //delay to allow a response
     //process the request and parse the header
     processHTTP();
     // Allocate the JSON document
     // Use arduinojson.org/v6/assistant to compute the capacity.
     DynamicJsonDocument doc(2048);
     // Parse JSON object
     DeserializationError error = deserializeJson(doc, client);
     if (error) {
        // this is due to an error in the HTTP request
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("HTTP failed... ");
        lcd.setCursor(0,1);
        lcd.print("On Setup ");
        return;
     }
     // Extract values
     Serial.println(F("Response:"));
     // lichess username
     username = doc["username"];
     Serial.println(username);
     //close request
     client.println("Connection: close");
     client.println();
     if (username!= NULL){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Connected!");
        lcd.setCursor(0,1);
        lcd.print("User: ");
        lcd.print(username);
        delay(1000);
     } 
    //we were unable to connect to the server  
    }else{
    lcd.setCursor(0,0);
    lcd.print("Server failed... ");
    lcd.setCursor(0,1);
    lcd.print("on setup ");
    }
}
  
void loop() {
// MODE 0: SEARCH FOR EXISTING OPEN GAMES
// IF THERE IS A GAME IN PLAY EXTRACT THE LAST MOVE, YOUR COLOUR AND THE TURN INFORMATION
// IF NOT WAIT FOR USER INPUT TO INITIATIATE MODE 1 OR 2
  if (MODE0 == true){ 
   // Make a HTTP request:
  if (client.connect(server, 443)) {
     Serial.println("connected to server for ongoing game");
     //keep the request so lichess knows you are there
     client.println("GET /api/account/playing HTTP/1.1");
     client.println("Host: lichess.org");
     client.print("Authorization: Bearer ");
     client.println(token);
     delay(delayTime); //delay to allow a response
     processHTTP();
     DynamicJsonDocument doc(2048);
     DeserializationError error = deserializeJson(doc, client);
     if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("HTTP error");
          lcd.setCursor(0,1);
          lcd.print("MODE0 INIT");
          return;
     }
     client.stop();
     // Extract values
     JsonObject nowPlaying_0 = doc["nowPlaying"][0];
     JsonObject nowPlaying_0_opponent = nowPlaying_0["opponent"];
     Serial.println(F("Response:"));
     currentGameID = nowPlaying_0["gameId"];
     Serial.println(currentGameID);
     // if there is an ongoing game
     if (currentGameID != NULL) {
       Serial.println("Current Game ID is not null!");
       // extract information from the output json
       myTurn = nowPlaying_0["isMyTurn"];
       lastMove = nowPlaying_0["lastMove"];
       myColour = nowPlaying_0["color"];
       opponentName = nowPlaying_0_opponent["username"]; 
       if (strcmp(myColour, "black") == 0){
        opponentColour = "white";
       } else {
        opponentColour = "black";
       }
       // the next if statement checks whether a previous game has just ended 
       // and if so resets the gameInit variable
       // this applies if there are multiple games running at the same time 
       if (strcmp(currentGameID,previousGameID) != 0){
        gameInit = true;
       }
       // if this is the first time connecting to this game ID
       // update the lcd display with game information
       if (gameInit == true){
           lcd.clear();
           lcd.setCursor(0,0);
           lcd.print("Ongoing game ");
           lcd.setCursor(0,1);
           lcd.print("ID: ");
           lcd.print(currentGameID);
           // if a user has queued a premove there is no time to delay 
           if (preMove == NULL){
            delay(3000);
           } 
           lcd.clear();
           lcd.setCursor(0,0);
           lcd.print("vs ");
           lcd.print(opponentName);
           lcd.setCursor(0,1);
           gameInit = false;
           // if the user makes a premove and is playing white, the last move needs to be displayed 
           if (preMove != NULL and strcmp(myColour, "black") == 0){
             lcd.print("white: ");
             lcd.print(lastMove);
           }else {
             lcd.print("colour: ");
             lcd.print(myColour);
             previousGameID = nowPlaying_0["gameId"];
             // reset correspondance variables
             challengeSent = false;
             // if a user has queued a premove there is no time to delay 
             if (preMove == NULL){
              delay(3000);
             } 
           }
       }
       // send a POST request with the chosen move if it is your turn
       if (myTurn == true){
           // user inputs a move (unless its a premove)
           if (preMove == NULL){
           lcd.clear();
           lcd.setCursor(0,0);
           lcd.print(opponentColour);
           lcd.print(": ");
           lcd.print(lastMove);
           lcd.setCursor(0,1);
           lcd.print("Input: ");
           coordIndex = 0; //important to reset coordindex as its shared 
           while (select == false){
            moveInput();
           }
           select = false;
           }
           lcd.clear();
           lcd.setCursor(0,0);
           lcd.print("Sending move");
           lcd.setCursor(0,1);
           lcd.print("information ... ");
           if (client.connect(server, 443)) {
               Serial.println("connected to server to send move information");
               client.print("POST /api/board/game/");
               client.print(currentGameID);
               if (preMove != NULL) {
                    client.print("/move/");
                    client.print(preMove);
                    preMove = NULL;
               }else {   
                   if (strcmp(uci,"g g ") == 0){
                    client.print("/resign"); 
                   } else if (strcmp(uci,"d d ") == 0){
                     client.print("/draw/yes"); 
                   } else {
                      client.print("/move/");
                    client.print(uci);
                   }
               }
               client.println(" HTTP/1.1");
               client.println("Host: lichess.org");
               client.print("Authorization: Bearer ");
               client.println(token);
               delay(delayTime); //delay to allow response time
               processHTTP(); // if the move is invalid it will get picked up in this function as a 400 Bad request
               StaticJsonDocument<48> doc;
               DeserializationError error = deserializeJson(doc, client);
               if (error) {
                    Serial.print(F("deserializeJson() failed: "));
                    Serial.println(error.f_str());
                    lcd.clear();
                    lcd.setCursor(0,0);
                    lcd.print("HTTP failed...");
                    return;
               }
               client.stop();
               //determine whether the move was successgul 
               moveSuccess = doc["ok"];
               if (moveSuccess == true){
                Serial.println("move success!");
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Move successful!");
               }
               else {
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Invalid move!");                
               }
          }
         // if it's not my turn
         }else {
          Serial.print(opponentName);
          Serial.println("s turn");
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Your move: ");
          lcd.print(lastMove);
          lcd.setCursor(0,1);
          lcd.print(opponentColour);
          lcd.print("'s turn.");
         }
       /// if there is no ongoing game    
       } else {
         //reset MODE0 if we aren't waiting for a challenge accept
         if (challengeSent != true){
          MODE0 = false;
         }
       }
  }
 } 
 //take user to main menu to choose between MODE1 or MODE2 
   else {
   if(gameInit == false){
   // this means that an initialised game has ended and there were no other games in play
   // so we must check for the reason and display it
    if (client.connect(server, 443)) {
       Serial.println("connected to server for finished game");
       Serial.println(previousGameID);
       client.print("GET /api/board/game/stream/");
       client.print(previousGameID);
       client.println(" HTTP/1.1");
       client.println("Host: lichess.org");
       client.print("Authorization: Bearer ");
       client.println(token);
       delay(delayTime); //delay to allow a response
       processHTTP();
       // there is a single byte (223) that precedes the board state response 
       // and will cause arduino json to give an error 
       // the code below will ignore the preceeding byte
       while (client.available()){
        char p = client.peek();
        if (p == '{'){
        break;
        } else {
        char c = client.read();
        Serial.print(c);
        }
       }
       StaticJsonDocument<1536> doc;
       DeserializationError error = deserializeJson(doc, client);
       if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("HTTP failed ");
            lcd.setCursor(0,1);
            lcd.print("at end game.");
            return;
       }
       //close request
       client.println("Connection: close");
       client.println();
       client.stop();
       // Extract values
       JsonObject state = doc["state"];
       winner = state["winner"];
       endStatus = state["status"]; 
       Serial.println(F("Response:"));
       Serial.println(winner);
       Serial.println(endStatus);
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("Winner: ");
       lcd.print(winner);
       lcd.setCursor(0,1);
       lcd.print("by: ");
       lcd.print(endStatus);
       while (select == false)
       {
         uint8_t buttons = lcd.readButtons(); 
         if (buttons & BUTTON_SELECT){
          select = true;
         }  
       }
       select = false;
       gameInit = true;
    }
   }
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("Create game: ");
   while (select == false){
    modeSelect();
   }
   select = false;
   if (MODE1 == true){
   // MODE 1: CORRESPONDANCE 
   // PLAYER CAN ISSUE A CHALLENGE TO ANOTHER USER 
   // ONCE CHALLENGE IS ACCEPTED RETURN TO MODE 0
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("Enter username:");
   coordIndex = 0;
   while (select == false){
    alphaNumSelect();
   }
   select = false;
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("Sending ");
   lcd.setCursor(0,1);
   lcd.print("Challenge...");
   if (client.connect(server, 443)) {
      Serial.println("connected to server to send challenge");
      client.print("POST /api/challenge/");
      client.print(corrUser);
      client.println(" HTTP/1.1");
      client.println("Host: lichess.org");
      client.print("Authorization: Bearer ");
      client.println(token);
      delay(delayTime); //delay to allow a response
      processHTTP();
      StaticJsonDocument<1536> doc;
      DeserializationError error = deserializeJson(doc, client);
      if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("HTTP failed ");
            lcd.setCursor(0,1);
            lcd.print("at corr create.");
            return;
       }
       //close request
       client.println("Connection: close");
       client.println();
       client.stop();
       // Extract values
       JsonObject challenge = doc["challenge"];
       challengeID = challenge["id"];
       // This is returned if the challenge ID was sent succesfully
       if (challengeID != NULL) {
         lcd.clear();
         lcd.setCursor(0,0);
         lcd.print("Challenge sent ");
         lcd.setCursor(0,1);
         lcd.print("ID: ");
         lcd.print(challengeID);
         challengeID = NULL;
         challengeSent = true;
         MODE0 = true; 
       }  
   }
   }else {
    // MODE 2: RANDOM MATCH 
    // PLAYER CAN INSTANTLY START A CASUAL MATCH WITH A RANDOM  OPPONENT
    // USER DEFINES TIME AND CLOCK INCREMENT
    // ONE MATCH IS FOUND RETURN TO MODE 0
    lcd.clear();
    while (select == false){
      clockSelect();
    }
    select = false;
    lcd.clear();
    while (select == false){
      colourSelect();
    }
    select = false;
    lcd.setCursor(0,1);
    lcd.print("Input: ");
    coordIndex = 0;
    while (select == false){
      moveInput();
    }
    select = false;
    preMove = uci;
    lcd.clear(); 
    lcd.setCursor(0,0);
    lcd.print("Posting");
    lcd.setCursor(0,1);
    lcd.print("Game Request.");
    String postData;
    String stringOne = String("time=") + clockTime;
    String stringTwo = String("&increment=") + clockIncrement;
    String stringThree = String("&color=") + selectedColour;
    postData = stringOne + stringTwo + stringThree;
    Serial.println(postData);
    if (client.connect(server, 443)){
      Serial.println("connected to server to send challenge");
      client.println("POST /api/board/seek HTTP/1.1");
      client.println("Host: lichess.org");
      client.print("Authorization: Bearer ");
      client.println(token);
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(postData.length()); 
      client.println();
      client.println(postData);
      processHTTP();
      // as long as the connection is active the seek will be public
      // we can tell when a game is found by when the connection drops
      // the simplest way i have found of identifying this 
      // is when client.println() returns 0 
      boolean game_found = false;
      while (game_found == false){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Searching ... ");
        if (client.println() == 0){
          game_found = true;
          lcd.setCursor(0,1);
          lcd.print("Game found.");
        }
      }
    }
    // Return to MODE0 and connect to the game that we just found 
    MODE0 = true;
  }
  }
}

/////////// FUNCTIONS //////////////////
////////////////////////////////////////
     
void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void processHTTP(){
     // this function processes http data before it is read by arduino json
     if (client.println() == 0) {
       Serial.println(F("Failed to send request"));
       return;
     }
     // Check HTTP status
     char status[32] = {0};
     client.readBytesUntil('\r', status, sizeof(status));
     // It should be "HTTP/1.0 200 OK" 
     if (strcmp(status + 9, "200 OK") != 0) {
      Serial.print(F("Unexpected response: "));
      Serial.println(status);
      if (strcmp(status + 9, "400 Bad Request") == 0) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Bad Request!");
      delay(2000);
      }
     else { 
      return;
     }
     }
     // Skip HTTP headers
     char endOfHeaders[] = "\r\n\r\n";
     if (!client.find(endOfHeaders)) {
      Serial.println(F("Invalid response"));
      return;
     }
}

void moveInput(){
  //// This function allows users to input a move during a game 
  // in UCI format +
  // "d d " offers a draw
  // "g g " resigns
  uint8_t buttons = lcd.readButtons(); 
  if (buttons & BUTTON_LEFT) {
    coordIndex --;
    if (coordIndex == -1){
      coordIndex = 3;
    }
  }
  if (buttons & BUTTON_RIGHT) {
    coordIndex ++;
    if (coordIndex == 4){
      coordIndex = 0;
    }
  }
  if (buttons & BUTTON_UP) {
    switch(coordIndex){
      case 0:
      firstCoord ++;
      if(firstCoord == 9){
        firstCoord = 0;
      }
      break;
      case 1:
      secondCoord ++;
      if(secondCoord == 9){
        secondCoord = 0;
      }
      break;
      case 2:
      thirdCoord ++;
      if(thirdCoord == 9){
        thirdCoord = 0;
      }
      break;
      case 3:
      fourthCoord ++;
      if(fourthCoord == 9){
        fourthCoord = 0;
      }
      break;
    }
  } 
  if (buttons & BUTTON_DOWN) {
    switch(coordIndex){
      case 0:
      firstCoord --;
      if(firstCoord == -1){
        firstCoord = 8;
      }
      break;
      case 1:
      secondCoord --;
      if(secondCoord == -1){
        secondCoord = 8;
      }
      break;
      case 2:
      thirdCoord --;
      if(thirdCoord == -1){
        thirdCoord = 8;
      }
      break;
      case 3:
      fourthCoord --;
      if(fourthCoord == -1){
        fourthCoord = 8;
      }
      break;
    }
  }
  if (coordIndex == 0){
     lcd.setCursor(7,1);
     lcd.print("_");
     delay(35);
     //lcd.print(columns[firstCoord]); 
  } else if (coordIndex == 1) {
     lcd.setCursor(8,1);
     lcd.print("_");
     delay(35);
     //lcd.print(rows[secondCoord]); 
  } else if (coordIndex == 2){
     lcd.setCursor(9,1);
     lcd.print("_");
     delay(35);
     //lcd.print(columns[thirdCoord]); 
  } else if (coordIndex == 3){
     lcd.setCursor(10,1);
     lcd.print("_");
     delay(35);
     //lcd.print(rows[fourthCoord]); 
  }
  uci[0] = columns[firstCoord];
  uci[1] = rows[secondCoord];
  uci[2] = columns[thirdCoord];
  uci[3] = rows[fourthCoord];  
  lcd.setCursor(7,1);
  lcd.print(uci);
  if (buttons & BUTTON_SELECT){
    select = true;
  }
  delay(100);
}

void modeSelect(){
  // this function allows users to select correspondance or random game
  lcd.setCursor(0,1);
  uint8_t buttons = lcd.readButtons(); 
  if (buttons & BUTTON_RIGHT){
    modeIndex ++;
    if (modeIndex == 2){
      modeIndex = 0;
    }
  }
 if (buttons & BUTTON_LEFT){
    modeIndex --;
    if (modeIndex == -1){
      modeIndex = 1;
    }
  }

 if (modeIndex == 1){
   MODE1 = false;
   lcd.print("2 Random game   ");
 } else {
   MODE1 = true; 
   lcd.print("1 Correspondance");
 } 
 if (buttons & BUTTON_SELECT){
  select = true;
  }  
  delay(100);
 }


void alphaNumSelect(){
  // This function allows users to input an opponents name for correspondance game
  uint8_t buttons = lcd.readButtons(); 
  if (buttons & BUTTON_LEFT) {
    coordIndex --;
    if (coordIndex == -1){
    coordIndex = 15;
    }
  }
  if (buttons & BUTTON_RIGHT) {
    coordIndex ++;
    if (coordIndex == 16){
    coordIndex = 0;
    }
  }
  if (buttons & BUTTON_UP) {
    alphaNumIndex[coordIndex] ++;
    if(alphaNumIndex[coordIndex] == 37){
    alphaNumIndex[coordIndex] = 0;  
    }
  } 
  if (buttons & BUTTON_DOWN) {
    alphaNumIndex[coordIndex] --;
    if(alphaNumIndex[coordIndex] == -1){
    alphaNumIndex[coordIndex] = 36;  
    }
  }
  lcd.setCursor(coordIndex,1);
  lcd.print('_');
  delay(35);
  corrUser[coordIndex] = alphaNumeric[alphaNumIndex[coordIndex]];
  lcd.setCursor(0,1);
  lcd.print(corrUser);
  if (corrUser[coordIndex] == ' '){
    corrUser[coordIndex] = '\0';
  }
  if (buttons & BUTTON_SELECT){
    select = true;
  }
  delay(100);
}

void clockSelect(){
// this functions determines the clock time, up and down change the game time, left and right change the increment 
  uint8_t buttons = lcd.readButtons(); 
  if (buttons & BUTTON_LEFT) {
    clockIncrement --;
    if (clockIncrement == -1){
    clockIncrement = 180;
    }
  }
  if (buttons & BUTTON_RIGHT) {
    clockIncrement ++;
    if (clockIncrement == 181){
    clockIncrement = 0;
    }
  }
  if (buttons & BUTTON_UP) {
    clockTime ++;
    if (clockTime == 181){
      clockTime = 60; 
    }
  } 
  if (buttons & BUTTON_DOWN) {
    clockTime --;
    if (clockTime == 59){
      clockTime = 180;
    }
  }

lcd.setCursor(0,0);
lcd.print("time: ");
lcd.print(clockTime);
if (clockTime < 100){
  lcd.print(" ");
}
lcd.setCursor(0,1);
lcd.print("increment: ");
lcd.print(clockIncrement);
if (clockIncrement < 100){
  lcd.print(" ");
}
if (buttons & BUTTON_SELECT){
  select = true;
  }  
  delay(100);
  
}


void colourSelect(){
// this functions determines the clock time, up and down change the game time, left and right change the increment 
  uint8_t buttons = lcd.readButtons(); 
  if (buttons & BUTTON_LEFT) {
    colourIndex --;
    if (colourIndex == -1){
    colourIndex = 1;
    }
  }
  if (buttons & BUTTON_RIGHT) {
    colourIndex ++;
    if (colourIndex == 2){
    colourIndex = 0;
    }
  }
if (colourIndex == 1){
  selectedColour = "black";
} else {
  selectedColour = "white";
}
lcd.setCursor(0,0);
lcd.print("Colour: ");
lcd.print(selectedColour);

if (buttons & BUTTON_SELECT){
  select = true;
  }  
delay(100);  
}
