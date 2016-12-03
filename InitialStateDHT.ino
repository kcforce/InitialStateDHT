/*
send data to initialstate.com
*/


#include <ESP8266WiFi.h>
#include "DHT.h"

//function Prototypes
bool postData();
bool postBucket();

#define DHTPIN 5   

// Uncomment whatever DHT sensor type you're using!
#define DHTTYPE DHT11  // DHT 11
//#define DHTTYPE DHT21  // DHT 21
//#define DHTTYPE DHT22  // DHT 22

DHT dht(DHTPIN,DHTTYPE); //Instantiate DHT object
WiFiClient client;  //Instantiate WiFi object


const char* MY_SSID = "ND-guest";
const char* MY_PWD =  "";


////////////////////////////
// Initial State Streamer //
////////////////////////////

// Data destination
// https can't be handled by the ESP8266, thus "insecure"
#define ISDestURL "insecure-groker.initialstate.com"
// Bucket key (hidden reference to your bucket that allows appending):
#define bucketKey "---Your Bucket Key---"
// Bucket name (name your data will be associated with in Initial State):
#define bucketName "---Your Bucket Name---"   //"Arduino Stream"
// Access key (the one you find in your account settings):
#define accessKey "---Your Access Key---" 

// How many signals are in your stream? You can have as few or as many as you want
const int NUM_SIGNALS = 5;

// What are the names of your signals (i.e. "Temperature", "Humidity", etc.)
String signalName[NUM_SIGNALS] = {"Humidity", "Temperature(Cel)","Temperature(Fehr)",
                                  "Heat Index(Cel)","Heat Index(Fehr)"};

// This array is to store our signal data later
String signalData[NUM_SIGNALS];




void setup()
{
 
  Serial.begin(115200);
  dht.begin();
  Serial.print("Connecting to "+*MY_SSID);
  WiFi.begin(MY_SSID, MY_PWD);
  Serial.println("going into wl connect");

  while (WiFi.status() != WL_CONNECTED) //not connected,..waiting to connect
    {
      delay(1000);
      Serial.print(".");
    }
  Serial.println("wl connected");
  Serial.println("");
  Serial.println("Credentials accepted! Connected to wifi\n ");
  Serial.println("");
}

void loop() {



  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  if (client.available())
  {
    char c = client.read();
    Serial.write(c);
  }

    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f))
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }


  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(f, h, false);
    // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(t, h);


  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.print(" *C ");
  Serial.print(hif);
  Serial.println(" *F\n");

 

  //fill our array with each sensor element strings casted from floats
   signalData[0] = (String) h;
   signalData[1] = (String) t;
   signalData[2] = (String) f;
   signalData[3] = (String) hic;
   signalData[4] = (String) hif;


  // The postData() function streams our events
  while(!postData());   

  // Wait for 1 seconds before collecting and sending the next batch
  delay(1000);

}

// this method makes a HTTP connection to the server and creates a bucket is it does not exist:
bool postBucket() {
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection:
  if (client.connect(ISDestURL, 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    // Build HTTP request.
    String toSend = "POST /api/buckets HTTP/1.1\r\n";
    toSend += "Host:";
    toSend += ISDestURL;
    toSend += "\r\n" ;
    toSend += "User-Agent:Arduino\r\n";
    toSend += "Accept-Version: ~0\r\n";
    toSend += "X-IS-AccessKey: " accessKey "\r\n";
    toSend += "Content-Type: application/json\r\n";
    String payload = "{\"bucketKey\": \"" bucketKey "\",";
    payload += "\"bucketName\": \"" bucketName "\"}";
    payload += "\r\n";
    toSend += "Content-Length: "+String(payload.length())+"\r\n";
    toSend += "\r\n";
    toSend += payload;
   
    client.println(toSend);
    Serial.println(toSend);
 
    return true;
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    return false;
  }
}



// this method makes a HTTP connection to the server and sends the signals measured:
bool postData() {
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection:
  if (client.connect(ISDestURL, 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    // Build HTTP request.
    // Build HTTP request.

  for (int i=0; i<NUM_SIGNALS; i++){
    String toSend = "POST /api/events HTTP/1.1\r\n";
    toSend += "Host:";
    toSend += ISDestURL;
    toSend += "\r\n" ;
    toSend += "Content-Type: application/json\r\n";
    toSend += "User-Agent: Arduino\r\n";
    toSend += "Accept-Version: ~0\r\n";
    toSend += "X-IS-AccessKey:  " accessKey "\r\n";
    toSend += "X-IS-BucketKey:  " bucketKey "\r\n";
   
    String payload = "[{\"key\": \"" + signalName[i] + "\", ";
    payload +="\"value\": \"" + signalData[i] + "\"}]\r\n";
   
    toSend += "Content-Length: "+String(payload.length())+"\r\n";
    toSend += "\r\n";
    toSend += payload;

    Serial.println(toSend);
    client.println(toSend);
    delay(3000);//so not to overload requests on Intialstate, adjust as needed
   }
  return true;
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    return false;
  }

}
