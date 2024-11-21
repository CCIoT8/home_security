  #include <WiFi.h>
  #include <PubSubClient.h>
  #include <Wire.h>
  #include <Adafruit_NeoPixel.h> 
  #define PIN_NEO_PIXEL 25  // The ESP32 pin GPIO16 connected to NeoPixel 
  #define NUM_PIXELS 12     // The number of LEDs (pixels) on NeoPixel LED strip 

  const int trigPin = 12; 
  const int echoPin = 13; 
  const int magnet_switch = 2;  
  const int vib_pin=34; 
  const int buzzer = 33; 
  
  //define sound speed in cm/uS 
  #define SOUND_SPEED 0.034 
  #define CM_TO_INCH 0.393701 
  
  long duration; 
  float distanceCm; 
  float distanceInch; 
  int dooropen;
  Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800); 

  // Replace the next variables with your SSID/Password combination
  const char* ssid = "hau";
  const char* password = "Chonghau1";
  const char* mqtt_server = "192.168.20.235";

  WiFiClient espClient;
  PubSubClient client(espClient);
  long lastMsg = 0;
  char msg[50];
  int value = 0;
  char* doormsg;
  
  void alarm(){ 
    NeoPixel.clear();  // set all pixel colors to 'off'. It only takes effect if pixels.show() is called 
    client.publish("TakeAPicture","saycheese!");

    // turn pixels to green one-by-one with delay between each pixel 
    for (int i = 0; i < 4; i++){ 
      for (int pixel = 0; pixel < NUM_PIXELS; pixel++) {           // for each pixel 
        NeoPixel.setPixelColor(pixel, NeoPixel.Color(255, 0, 0));  // it only takes effect if pixels.show() is called 
        NeoPixel.show();                                           // update to the NeoPixel Led Strip 
        delay(20);  // 500ms pause between each pixel 
      } 
      NeoPixel.clear(); 
      NeoPixel.show(); 
      digitalWrite(buzzer, LOW); 
      delay(50); 
  
      for (int pixel = 0; pixel < NUM_PIXELS; pixel++) {           // for each pixel 
        NeoPixel.setPixelColor(pixel, NeoPixel.Color(0, 0, 255));  // it only takes effect if pixels.show() is called 
        NeoPixel.show();                                           // update to the NeoPixel Led Strip 
        delay(20);  // 500ms pause between each pixel 
      } 
      NeoPixel.clear(); 
      NeoPixel.show(); 
      digitalWrite(buzzer, LOW); 
      delay(50); 
    } 
    digitalWrite(buzzer, HIGH); 
    for (int pixel = 0; pixel < NUM_PIXELS; pixel++) {           // for each pixel 
      NeoPixel.setPixelColor(pixel, NeoPixel.Color(255, 255, 255));  // it only takes effect if pixels.show() is called 
    } 
    NeoPixel.show(); 
    delay(50); 
    
  } 

  void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  void callback(char* topic, byte* message, unsigned int length) {
    Serial.println(topic);
    String messageTemp;
    
    for (int i = 0; i < length; i++) {
      Serial.print((char)message[i]);
      messageTemp += (char)message[i];
    }
    Serial.println();

  }

  void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect("ESP8266Client")) {
        Serial.println("connected");
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
  }

  void setup() { 
    Serial.begin(115200); // Starts the serial communication 
    pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output 
    pinMode(echoPin, INPUT); // Sets the echoPin as an Input 
    pinMode(magnet_switch, INPUT_PULLUP); 
    pinMode(vib_pin,INPUT); 
    pinMode(buzzer, OUTPUT); 
    NeoPixel.begin();  // initialize NeoPixel strip object (REQUIRED) 
    NeoPixel.clear(); 
    for (int pixel = 0; pixel < NUM_PIXELS; pixel++) {           // for each pixel 
      NeoPixel.setPixelColor(pixel, NeoPixel.Color(255, 255, 255));  // it only takes effect if pixels.show() is called 
    } 
    digitalWrite(buzzer, HIGH); 
    NeoPixel.show(); 
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
  }

  void loop() {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    // Clears the trigPin 
    digitalWrite(trigPin, LOW); 
    delayMicroseconds(2); 
    // Sets the trigPin on HIGH state for 10 micro seconds 
    digitalWrite(trigPin, HIGH); 
    delayMicroseconds(10); 
    digitalWrite(trigPin, LOW); 
    
    // Reads the echoPin, returns the sound wave travel time in microseconds 
    duration = pulseIn(echoPin, HIGH); 
    
    // Calculate the distance 
    distanceCm = duration * SOUND_SPEED/2; 
    
    // Prints the distance in the Serial Monitor 
    Serial.print("Distance (cm): "); 
    Serial.println(distanceCm); 
    
    if (digitalRead(magnet_switch) == LOW) { 
      //Serial.println("Door intact."); 
      doormsg = "intact";
    } 
    else { 
      //Serial.println("Door breached!"); 
      doormsg = "breached";
    } 
  
    int val; 
    char* windowmsg;
    val=digitalRead(vib_pin); 
    //Serial.println(val); 

    if (val == 1) { 
      windowmsg = "breached";
    }
    else {
      windowmsg = "intact";
    } 

    // sound the alarm
    if (doormsg == "breached"){
      alarm();
    }
    if (windowmsg == "breached"){
      alarm();
    }
    if ((distanceCm > 0) && (distanceCm < 10)){
      alarm();
    }
  

    delayMicroseconds(15); 
    long now = millis();

    if (now - lastMsg > 100) {
      lastMsg = now;

      client.publish("esp32/door", doormsg);
      
      char disString[8];
      dtostrf(distanceCm, 1, 2, disString);
      client.publish("esp32/distance", disString);

      client.publish("esp32/window", windowmsg);
    }
  }
