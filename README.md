# Lichess-Link
Link an Arduino Uno with Wifi to the lichess.org API to play online chess the old fashioned way!

Full tutorial: https://create.arduino.cc/projecthub/kzra/lichess-link-cbdbab

## Online chess, the old fashioned way
Sick of being trolled online while losing to 13 year olds with bad manners? Want to play chess over the board without the distraction of a smartphone or laptop? Want to play correspondence games with friends or connect to random opponents?

Then lichess link is for you! The project connects an Arduino Uno WiFi Rev 2 with the lichess.org application programming interface (API) to allow you to receive and send chess moves using your Arduino and an LCD Shield with input buttons.

## How it works
First you connect your Arduino to the internet by inputting your network SSID and Password into the secrets.h module. You access your lichess.org account by inputting your personal API token into the secrets.h module.

### MODE 0: Connect to ongoing game.
Once connected, lichess link will first search for any ongoing games you have. If there is an ongoing game it will connect. If it is your turn your opponent's last move will be displayed and you will be asked to input a move. If its your opponent's turn your last move will be displayed and the screen will refresh every 5 seconds and update if your opponent has made a move.

**Move input:** You input your moves using UCI format. For example, at the start of the game "e2e4" will move white's pawn from position e2 to position e4. If you would like offer a draw, input "d d " and to resign input "g g ". A promoted pawn becomes a queen.

**Multiple ongoing games:** I advise against using lichess link for multiple ongoing games, as it will quickly become confusing to know which game you are playing in (game information is displayed just once, when you connect to an ongoing game). If you have multiple ongoing games, lichess link will connect to whichever lichess.org deems as most 'urgent'. In reality this means the game in which it is your turn and your clock time is lowest.

**If the game ends:** If there is only one ongoing game and it ends for any reason, the lichess link will display the winner (if any) and the reason. Press the select button to progress to the next display.

### MODE 1: Create a correspondence game
If there are no ongoing games you will be prompted to choose between two game modes. Mode 1 is to create a correspondence game (with unlimited time for each player). If you select this mode you will be prompted to input a username to which a correspondence game request will be sent. If the request is sent successfully the display will update and refresh every 5 seconds until the request is accepted or declined. In either case the program then returns to Mode 0.

**Username input:** You can input any username up to 16 characters long. The username cannot contain spaces or non alpha-numeric characters.

### MODE 2: Create a game with a random opponent.*
Mode 2 creates a timed game against any public lichess.org user. You will be prompted to input the time (a value between 60 and 180 minutes) and the increment (a value between 0 and 180 seconds) of the game, choose a colour, and input a pre-move.

If a game is found the program will return to Mode 0, if not you will be prompted to choose between Mode 1 and Mode 2 again.

**Pre-move input:** Due to the time delay in receiving game information you are required to input your first move before the game has begun. This move will be made for you at the start of the game whether you are playing as white or black. It is important that this is a valid move as any invalid move is likely to abort the game.

*\*Currently you must also connect to the game on your laptop or smartphone. If you play solely on the Arduino your opponent will be able to claim victory after 90 seconds as the lichess server believes you have disconnected from the game. This is a limitation that can hopefully be solved in a future version of the software!*

### ERROR MESSAGES
While using the lichess link you may receive certain information or error messages on the screen. Here is how to interpret them:

**Wifi failed:** Your arduino was unable to connect to your WiFi. Check your connection settings and try again.

**Server failed:** Your arduino was unable to contact the lichess.org server, you should press the "reset" button on the LCD shield.

**HTTP failed:** Your arduino was unable to read the response to an HTTP request. If this is accompanied by Bad Request you are receiving a 400 response. This is likely if you have a made an invalid move or attempted to stream a game that has already ended. After any HTTP fail your Arduino will automatically reset so there is no need to do anything. If you are repeatedly receiving HTTP fails it may be that the delay time between sending and processing a request is too small. You can increase it by adjusting the delayTime variable in the code (default value: 5000).

## Assembling the lichess link
This should be quite simple. Install the following libraries using the library manager on your Arduino IDE: ArduinoJson (v.6.), WifiNINA and Adafruit_RGBLCDShield.

Attach the LCD shield to your Arduino, input your secret information to the arduino_secrets.h module and upload the lichess-link.ino code from github.

## Acknowledgements
This project was only possible due to the fantastic documentation and examples of two libraries and the lichess.org API.

1. [Arduino Json](https://arduinojson.org). This library is used throughout the code to deserialize the json output from the lichess API following an HTTP request. The library allows you to stream the output directly into a json document. There is also [this great tool](https://arduinojson.org/v6/assistant/) which computes the memory requirements of a json output.

2. [Wifi NiNa](https://www.arduino.cc/en/Reference/WiFiNINA). This library is used to connect to the lichess.org server and make HTTP requests. A lot of the lichess link code was repurposed from examples in this library.

3. [Lichess.org API](https://lichess.org/api). This API is used to communicate with lichess.org. In this program I make three kinds of GET request and three kinds of POST request.

    * GET #1 : /api/account - to retrieve the current users username
    * GET #2: /api/account/playing - to stream ongoing games (mode 0)
    * GET #3: /api/board/game/stream/ - to determine the outcome of an ended game
    * POST #1: /api/board/game/ - to send move information
    * POST #2: /api/challenge/ - to create a correspondence challenge (mode 1)
    * POST #3: /api/board/seek - to create a random challenge (mode 2)

## Contribute/Feedback
This project is currently in an alpha version. This was my first time doing any internet-based programming so I expect there are many ways the project could be improved! Get in touch or comment if you have any feedback on how to make the program more efficient. I've uploaded the code on GitHub so you can fork the repository and make changes if you prefer to work that way.

If you decide create a lichess link, I hope it brings you peace and allows you to enjoy online chess in a new way.  
