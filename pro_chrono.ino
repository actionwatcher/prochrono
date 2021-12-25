#include <Arduino.h>

/*
Arduino Compatible Wired Remote Control for ProChrono Digital version 0.01
Designed and Programmed by Douglas Good, November 2013.
WORKING SO FAR:
- String Change Command
- Delete String Command
- Delete Shot Command
- Review Shot Command
- Redisplay Command
- Checksum validation for data sent and received to/from ProChrono
The circuit:
* TX on pin 12 (middle "ring" section of 3.5mm plug) for data going out to ProChrono from Arduino
* RX on pin 11 (tip of 3.5mm plug) for data coming INTO Arduino from ProChrono
* ProChrono Ground to Arduino Ground (base of 3.5mm plug)
* 10K ohm Resistor from ground to RX pin 11 *VERY IMPORTANT* data won't be received without this!
* Review shot button connecting Pin 6 to ground
* Delete Shot button connecting Pin 7 to ground
* Delete String button connecting Pin 8 to ground
* Next String (aka String Change) button connecting Pin 9 to ground
* Redisplay button connecting Pin 10 to ground
Not yet implemented:
* Clear All Strings button (must press and hold for 4 seconds) on pin X
* LCD readout to duplicate the chronograph display on the remote
Note:
Pin 11 was chosen for RX as it should be compatible with the Mega and Leonardo as
well as the UNO. Not all pins on the Mega and Leonardo can be used to receive serial data.
*/
// define SerialMonitor in order to send debug messages to the computer.
// If using as a standalone remote, undefine this to save memory.
#define SerialMonitor

#include <SoftwareSerial.h>

int reviewPin = 4; // Review Shot button, switched to ground
int delshotPin = 5; // Delete shot button pin, switched to ground
int delstringPin = 6; // Delete string button pin, switched to ground
int nextstringPin = 10; // Next string button pin, switched to ground
int redisplayPin = 9; // Redisplay button pin, switched to ground
int rxpin = 11; // Incoming Data from ProChrono is recieved on this pin (tip)
// NOTE: RX pin must be tied to ground with a 10K ohm resistor!
int txpin = 12; // Data going out to ProChrono is transmitted on this pin (ring)
int ledpin = 13; // Serial Indicator LED, completely optional
int BadDataTimeout = 900; // Milliseconds after which data is assumed to be bad and Rx string is cleared
long lastRxtime = 0; // Records the start of the last data recieve time as a timeout for bad data
char aChar = 0x00; // used to read incoming serial data
String Incoming = ""; // used to collect incoming data from the serial port
// ProChrono Command Packets
// These command strings tell the ProChrono to perform specific functions. All 5 buttons on the front of the
// unit are duplicated here. In order to be understood by the ProChrono, a 2 character (1 hex byte) checksum
// must be appended to these commands using the AppendChecksum function.
// Note that the ProChrono protocol documentation is a bit confusing in the way functions are named, so
// I've named some of them differently here to better describe what they do.
String ReviewString = ":00000004"; //Same as hitting the "Review" button on ProChrono
String DeleteShot = ":00000006"; //Same as hitting "Delete Shot" button on ProChrono
String DeleteString = ":00000007"; //Same as hitting "Delete String" button on ProChrono
String NextString = ":00000005"; //Same as hitting "String Change" button on ProChrono
String GoToFirstVelocity = ":00000008"; //Jumps to the most recent shot in the string
String GoToFirstStatistic = ":00000009"; //Jumps to the "HI" statistic
String RedisplayString = ":0000000E"; //Same as hitting the "Redisplay" button on ProChrono
// The following are not yet implemented and/or used in this version of this code
String GetCurrentShotInfo = ":00000003"; // Not yet implemented returns first byte 04, second byte string, third byte shot
String RequestVelocityData = ":0200000101"; // last byte is the string number to send data for
SoftwareSerial ProChrono(rxpin, txpin); // RX, TX for ProChrono
void setup() {
    pinMode(ledpin, OUTPUT); // set up the built in LED indicator (on the Uno)
    digitalWrite(ledpin, LOW); // turn off the LED
    pinMode(rxpin, INPUT); // the pin that receives data coming into the Arduino (must use 10k pulldown resistor!)
    pinMode(txpin, OUTPUT); // the pin that sends data from the Arduino
    digitalWrite(txpin, LOW);
    pinMode(delshotPin, INPUT); // create a pin for the Delete Shot button
    digitalWrite(delshotPin, HIGH); // and set the pullup resistor on
    pinMode(delstringPin, INPUT); // create a pin for the Delete String button
    digitalWrite(delstringPin, HIGH); // and set the pullup resistor on
    pinMode(nextstringPin, INPUT); // create a pin for the Next String button
    digitalWrite(nextstringPin, HIGH); // and set the pullup resistor on
    pinMode(redisplayPin, INPUT); // create a pin for the Redisplay button
    digitalWrite(redisplayPin, HIGH); // and set the pullup resistor on
    pinMode(reviewPin, INPUT); // create a pin for the Review button
    digitalWrite(reviewPin, HIGH); // and set the pullup resistor on
#ifdef SerialMonitor
// Open serial communications with PC and wait for port to open:
    Serial.begin(9600);
    while (!Serial) { ; // wait for serial port to connect. Needed for Leonardo only
    }
    Serial.println("Arduino ProChrono Remote connected");
#endif
    ProChrono.begin(1200); // set the data rate for the ProChrono Serial port at 1200 baud
}

void loop() {
    auto cnt = ProChrono.available();
    for(int i = 0; i < cnt; ++i) {
        digitalWrite(ledpin, HIGH);
        lastRxtime = millis(); // reset our incoming data timeout counter
        aChar = ProChrono.read(); // grab the character
        Serial.write(aChar); // display it in the monitor
        Incoming += aChar; // add it to our receive string
        if ((Incoming.length() >= 8) && VerifyChecksum(Incoming)) {
#ifdef SerialMonitor
            Serial.println();
            Serial.println(Incoming + " is a valid packet");
#endif
            digitalWrite(ledpin, LOW);
// This is where we would process the incoming data... to be coded in a future version.
            Incoming = ""; // clear our receive buffer string
            lastRxtime = 0; // zero out our timeout counter
        }
    }
// This is a simplistic check for bad serial data. If a valid packet isn't received within
// the timeout period, we assume it's bad data, throw it away and reset our variables.
// This should be coded to be a smarter, more in-depth check.
    if (((millis() - lastRxtime) >= BadDataTimeout) && (lastRxtime > 0)) {
#ifdef SerialMonitor
        Serial.println(Incoming + " appears to be invalid data.");
#endif
        Incoming = "";
        lastRxtime = 0;
        digitalWrite(ledpin, LOW);
    }
// The functions below are called when buttons on the remote are pushed
    if (digitalRead(nextstringPin) == LOW) { // We have a button press
        delay(75); // wait for a fraction of a second so we don't repeat too fast
        if (digitalRead(nextstringPin) == LOW) { // if button is still down then
            SendPacket(NextString); // send the Next string command
        }
    }
    if (digitalRead(redisplayPin) == LOW) { // We have a button press
        delay(75); // wait for a fraction of a second so we don't repeat too fast
        if (digitalRead(redisplayPin) == LOW) { // if button is still down then
            SendPacket(RedisplayString); // send the Next string command
        }
    }
    if (digitalRead(delshotPin) == LOW) { // We have a button press
        delay(150); // wait for a bit so we don't repeat too fast
        if (digitalRead(delshotPin) == LOW) {
            SendPacket(DeleteShot);
        }
    }
    if (digitalRead(delstringPin) == LOW) { // We have a button press
        delay(1000); // wait for one second; we don't want to accidentally delete a string!
        if (digitalRead(delstringPin) == LOW) { // if button is still down then
            SendPacket(DeleteString); // send Delete Shot command
        }
    }
    if (digitalRead(reviewPin) == LOW) { // We have a button press
        delay(75); // wait for a fraction of a second
        if (digitalRead(reviewPin) == LOW) { // if button is still down then
            SendPacket(ReviewString); // send Delete Shot command
        }
    }
} // end loop
void SendPacket(String PacketData) {
    digitalWrite(ledpin, HIGH);
    ProChrono.print(AppendChecksum(PacketData));
#ifdef SerialMonitor
    Serial.println("Sending " + AppendChecksum(PacketData));
#endif
    delay(400); // pause a bit in case button is held down
    digitalWrite(ledpin, LOW);
}

String AppendChecksum(String strCommand) {
// Appends the correct ProChrono checksum value to a given string
    return strCommand + GetChecksumStr(strCommand);
}

String GetChecksumStr(String strCommand) {
// calculates the checksum of a string and returns it as a 2 digit hex string.
    String hexNumber = "";
    int nChecksum = 0;
    for (int x = 1; x < strCommand.length(); x++)
        nChecksum += (int) strCommand.charAt(x); // add the characters in the string
    nChecksum = (256 - (nChecksum % 256)); // Calculate the checksum
    hexNumber = String(nChecksum, HEX); // Convert to a hex string
    hexNumber.toUpperCase(); // ProChrono talks in all caps, so we must too
    return hexNumber;
}

bool VerifyChecksum(String strCommand) {
// Returns TRUE if the given string contains a valid checksum, and FALSE if not
    String tempstr = strCommand.substring(0, strCommand.length() - 2); // get all but the last 2 characters
    tempstr = GetChecksumStr(tempstr); // get the checksum of the string
    if (strCommand.endsWith(tempstr)) // test whether last 2 characters match the calculated checksum
        return true;
    else
        return false;
}