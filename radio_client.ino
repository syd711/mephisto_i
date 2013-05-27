#include <SPI.h>
#include <Ethernet.h>
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 7, 6, 5, 3);
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,3);

// Enter the IP address of the server you're connecting to:
IPAddress server(192,168,1,5); 
EthernetClient client;

// The id of the relais to block output during booting.
int RELAIS_ID = 2;

// Common UI status info variables
// current station playing
int currentStationId = 0;
// number of stations available
int stations = 0;
String display = "";
unsigned long displayRefreshMillis = 0;
unsigned const long DISPLAY_REFRESH_MILLIS = 1500; //refresh interval
unsigned long scrollRefreshMillis = 0;
unsigned long SCROLLING_REFRESH_MILLIS = 400; //scrolling delay

int titleScrollIndex = -8;
String title = "";

/**
 * The buttons input 
 */
int NEXT_PIN = A0; 
int PREV_PIN = A1;  
int nextStation = 0;
int prevStation = 0;

void setup() {
 // Open serial communications and wait for port to open:
  Serial.begin(9600);

  // set up the LCD's number of columns and rows: 
  lcd.begin(20, 4);
  lcd.noAutoscroll();
  
  //apply relais port
  pinMode(RELAIS_ID, OUTPUT);     
  
  //boot message
  //digitalWrite(RELAIS_ID, HIGH); 
  bootRouter();
  
  // start the Ethernet connection:
  Serial.println("Ethernet setup");
  Ethernet.begin(mac, ip);


  // give the Ethernet shield a second to initialize:
  delay(1000);

  // if you get a connection, report back via serial:
  Serial.println("Connection setup ....");
  if (client.connect(server, 6600)) {
    loadPlaylist();
  } 
  else {
    // if you didn't get a connection to the server:
    Serial.println("Error: connection failed");
    lcd.clear();
    displayRow(0, "Error: MPD connect");
    displayRow(1, "       failed");
  }
}

/**
 * Initial booting message
 */
void bootRouter() {
  Serial.println("     MEPHISTO I     ");
  digitalWrite(RELAIS_ID, LOW); 
  lcd.setCursor(0,0);
  lcd.print("     MEPHISTO I     ");
  lcd.setCursor(0,1);
  lcd.print("        is          ");
  lcd.setCursor(0,2);
  lcd.print("      booting       ");
  for(int i=0; i<20; i++) {//37 sec.
    lcd.setCursor(i, 3);
    lcd.print("*");
    delay(1900);
  }
  digitalWrite(RELAIS_ID, HIGH); 
}

/**
 * Yup, lets loooooop...
 */
void loop() {
  checkDisplayData();
  checkDisplayScrolling();
  checkNextPrevButtons();
  checkConnection();
}

/**
 * Checks if the display requires refresh
 */
void checkDisplayData() {    
  unsigned long currentMillis = millis();
  if(currentMillis - displayRefreshMillis >= DISPLAY_REFRESH_MILLIS)
  {
    displayRefreshMillis = currentMillis;
    if(nextStation > 0) {
      client.println("next");
      client.println("play");
      nextStation = 0;
    }
    else if(prevStation > 0) {
      client.println("previous");
      client.println("play");
      prevStation = 0;
    }
    else {
      client.println("currentsong");
    }
  }
  String displayBuffer = "";    
  while (client.available()) {
    char c = client.read();
    displayBuffer.concat(c);
  }  

  if(displayBuffer.length() > 0 && displayBuffer != display) {
    display = displayBuffer;
    if(display.indexOf("OK") > 0 && display.indexOf("Id:") > 0) {
      refreshDisplay();    
    }
  }  
}

/**
 * Checks the next and prev station buttons.
 */
void checkNextPrevButtons() {
  int nextValue = analogRead(NEXT_PIN);    
  if(nextValue > 100) {
    delay(100);
    nextValue = analogRead(NEXT_PIN);    
    if(nextValue > 0) {
      title = "";
      lcd.clear();
      displayRow(0, "Next station...");
      nextStation++;
    }
  }
  int prevValue = analogRead(PREV_PIN);    
  if(prevValue > 100) {
    delay(100);
    prevValue = analogRead(PREV_PIN);    
    if(prevValue > 0) {
      title = "";
      lcd.clear();
      displayRow(0, "Previous station...");
      prevStation++;
    }
  }
}

/**
 * Central method to update the radio display.
 */
void refreshDisplay() {
  titleScrollIndex = 0; //reset the scrolling of the title
  title = "";
  displayStationId(display);
  displayRadioName(display);
  displayTitle(display);
}


/**
 * Scrolls the active values
 */
void checkDisplayScrolling() {
  unsigned long currentMillis = millis();
  if(currentMillis - scrollRefreshMillis >= SCROLLING_REFRESH_MILLIS)
  {
    scrollRefreshMillis = currentMillis;
    scrollTitle();
  }
} 

/**
 * Checks if the Ethernet connection to
 * the router is still valid.
 */
void checkConnection() {
  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    client.stop();
    init();    
  }
}


/**
 * Checks the amount of stations available
 */
void loadPlaylist() {
  Serial.println("Loading playlist");
  delay(200);
  client.println("playlist");
  delay(500);
  String list = "";

  while (client.available()) {
    char c = client.read();
    list.concat(c);
    if(c == 10) {
      stations++;
    }
  }
  stations--;
  stations--;
}


/**
 * Formats the radio station and display it in first row.
 */
void displayStationId(String display) {
  int index = display.indexOf("Pos:") + 4;
  int lastIndex = display.indexOf("Id:")-1;
  String stationId = display.substring(index, lastIndex);
  
  //calc current station id
  int id = stringToNumber(stationId);
  id++;
  
  //reset display if station has changed
  if(id != currentStationId) {
    currentStationId = id;
    lcd.clear();
  }
  
  lcd.setCursor(0,0);
  lcd.print("                    ");
  lcd.setCursor(0,0);
  lcd.print("Station ");
  lcd.print(id);
  lcd.print("/");
  lcd.print(stations);  
}

/**
 * Formats the radio name and display it in second row.
 */
void displayRadioName(String display) {
  int index = display.indexOf("Name:") + 6;
  int lastIndex = display.indexOf("Pos:")-1;
  if(index > 0 && lastIndex > 0) {
    String radioName = display.substring(index, lastIndex);
    //radioName = convertSpecialChars(radioName);
    displayRow(1, radioName);
  }
}

/**
 * Formats the title and display it in 3.+4. row.
 */
void displayTitle(String display) {
  int index = display.indexOf("Title:") + 7;
  int lastIndex = display.indexOf("Name:")-1;
  if(index > 0 && lastIndex > 0) {
    title = display.substring(index, lastIndex); //store global title string
    //title = convertSpecialChars(title);
    displayRow(3, title);
  }  
}

/**
 * Invoked when the scroll trigger is executed
 */
void scrollTitle() {
  if(titleScrollIndex <= 0) { //just wait for n intervals
    if(titleScrollIndex > -4) {
      displayRow(3, title);    
    }
    titleScrollIndex++;
    return;
  }
  String activeScrollTitle = "";
  if(titleScrollIndex+20 <= title.length()) {
    activeScrollTitle = title.substring(titleScrollIndex, titleScrollIndex+20);
    titleScrollIndex++;  
    displayRow(3, activeScrollTitle);    
  } 
  else {
    titleScrollIndex = -8; //set amount of wait intervals
  }
}



/***************************************************************************
 * Common helper
 **************************************************************************/
/**
 * Because I am too stupid to figure out how else...
 */
int stringToNumber(String thisString) {
  int i, value, length;
  length = thisString.length();
  char blah[(length+1)];
  for(i=0; i<length; i++) {
    blah[i] = thisString.charAt(i);
  }
  blah[i]=0;
  value = atoi(blah);
  return value;
}

/**
 * Shows the given line for the row of the display.
 */
void displayRow(int row, String line) {
  lcd.setCursor(0,row);
  if(line.length() <= 20) {
    lcd.print("                    ");
    lcd.setCursor(0,row);
    lcd.print(line);
  }
  else {
    String prefix = line.substring(0,20);
    lcd.print(prefix);
  }  
}

/**
 * Format Umlaute and special characters: ö,ä,ü,@
 */
String convertSpecialChars(String telnetInput) {
  int length = telnetInput.length();
  char convertedChars[(length)];
  int index = 0;
  for(int i=0; i<length; i++) {
    char aChar = telnetInput.charAt(i);
    int ascii = aChar;
    if(ascii == -61) {
      convertedChars[index] = 239; //ö
      index++;
      continue;
    }
    else if(ascii < 0) {
      Serial.print("Filtered:");
      Serial.println(ascii);
      continue;
    }
    convertedChars[index] = aChar;
    index++;
  }
  String converted = String(convertedChars);
  converted.trim();
  return converted;
}




