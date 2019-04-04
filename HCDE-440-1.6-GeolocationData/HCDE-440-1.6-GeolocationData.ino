/*A sketch to get the ESP8266 on the network and connect to some open services via HTTP to
 * get our external IP address and (approximate) geolocative information in the getGeo()
 * function. To do this we will connect to http://freegeoip.net/json/, an endpoint which
 * requires our external IP address after the last slash in the endpoint to return location
 * data, thus http://freegeoip.net/json/XXX.XXX.XXX.XXX
 * 
 * This sketch also introduces the flexible type definition struct, which allows us to define
 * more complex data structures to make receiving larger data sets a bit cleaner/clearer.
 * 
 * jeg 2017
 * 
 * updated to new API format for Geolocation data from ipistack.com
 * brc 2019
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h> //provides the ability to parse and construct JSON objects

const char* ssid = "NM";
const char* pass = "nikita123";
const char* key = "b4038b91c68deb4bdec045124c6df669";
const char* weatherkey = "73b43e4a3b95d2a36b9d9551fa564d82";

typedef struct { //creating a new data type with the name of MetData
  //You should report temperature, humidity, windspeed, wind direction, and cloud conditions
  String temperature;
  String humidity;
  String windspeed;
  String windDir;
  String cloudCon;
} MetData;

MetData weather;

typedef struct { //here we create a new data type definition, a box to hold other data types
  String ip;    //
  String cc;    //for each name:value pair coming in from the service, we will create a slot
  String cn;    //in our structure to hold our data
  String rc;
  String rn;
  String cy;
  String ln;
  String lt;
} GeoData;     //then we give our new data structure a name so we can use it in our code

GeoData location; //we have created a GeoData type, but not an instance of that type,
                  //so we create the variable 'location' of type GeoData

void setup() {
  Serial.begin(115200); //starts serial port
  delay(10); //delays 10ms
  Serial.print("This board is running: ");
  Serial.println(F(__FILE__)); //compiled file
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__)); //time of compiling
  
  Serial.print("Connecting to "); Serial.println(ssid); //prints what wifi is being connected to

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass); //handles connecting to wifi

  while (WiFi.status() != WL_CONNECTED) { //showing the user something is happening until connected
    delay(500);
    Serial.print(".");
  }
  // printing confirmation and assigned internal ip address 
  Serial.println(); Serial.println("WiFi connected"); Serial.println();
  Serial.print("Your ESP has been assigned the internal IP address ");
  Serial.println(WiFi.localIP());

  getGeo(); //calls the getGeo method

  //prints out everything about IP and location
  Serial.println("Your external IP address is " + location.ip);
  Serial.print("Your ESP is currently in " + location.cn + " (" + location.cc + "),");
  Serial.println(" in or near " + location.cy + ", " + location.rc + ".");
  Serial.print("and located at (roughly) ");
  Serial.println(location.lt + " latitude by " + location.ln + " longitude.");

  getMet(); //calls the getMet method

  //printing out weather data
  Serial.println("The temperature in your area is " + weather.temperature + " degrees F.");
  Serial.println("The humidity is " + weather.humidity + "%.");
  Serial.println("The windspeed is " + weather.windspeed + " MPH at a direction of " + weather.windDir + " degrees.");
  Serial.println("The cloud coverage is " + weather.cloudCon + "%");
}

void loop() {
  //if we put getIP() here, it would ping the endpoint over and over . . . DOS attack?
}

void getMet() {
  HTTPClient theClient; //creates an HTTPClient object named theClient
  String apistring = "http://api.openweathermap.org/data/2.5/weather?q=" + location.cy + "&units=imperial&appid=" + weatherkey; //concatonating the api request url
  theClient.begin(apistring); //make the request
  int httpCode = theClient.GET(); //get the HTTP code (-1 is fail)

  if (httpCode > 0) { //test if the request failed
    if (httpCode == 200) { //if successful...
      DynamicJsonBuffer jsonBuffer; //create a DynamicJsonBuffer object named jsonBuffer
      String payload = theClient.getString(); //get the string of json data from the request and assign it to payload
      Serial.println("Parsing...");
      JsonObject& root = jsonBuffer.parse(payload); //set the json data to the variable root
      
      if (!root.success()) { //check if the parsing worked correctly
        Serial.println("parseObject() failed");
        Serial.println(payload); //print what the json data is in a string form
        return;
      } //assign MetData object "weather" variables with the following data from the JsonObject
      weather.temperature = root["main"]["temp"].as<String>();
      weather.humidity = root["main"]["humidity"].as<String>();
      weather.windspeed = root["wind"]["speed"].as<String>();
      weather.windDir = root["wind"]["deg"].as<String>();
      weather.cloudCon = root["clouds"]["all"].as<String>();
    } else { //print error if the request wasnt successful
      Serial.println("Had an error connecting to the network.");
    }
  }
}

String getIP() {
  HTTPClient theClient;
  String ipAddress;

  theClient.begin("http://api.ipify.org/?format=json"); //Make the request
  int httpCode = theClient.GET(); //get the http code for the request

  if (httpCode > 0) {
    if (httpCode == 200) { //making sure the request was successful

      DynamicJsonBuffer jsonBuffer;

      String payload = theClient.getString();
      JsonObject& root = jsonBuffer.parse(payload);
      ipAddress = root["ip"].as<String>();

    } else { //error message for unsuccessful request
      Serial.println("Something went wrong with connecting to the endpoint.");
      return "error";
    }
  }
  return ipAddress; //returning the ipAddress 
}

void getGeo() {
  HTTPClient theClient;
  Serial.println("Making HTTP request");
  theClient.begin("http://api.ipstack.com/" + getIP() + "?access_key=" + key); //return IP as .json object
  int httpCode = theClient.GET();

  if (httpCode > 0) {
    if (httpCode == 200) {
      Serial.println("Received HTTP payload.");
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      Serial.println("Parsing...");
      JsonObject& root = jsonBuffer.parse(payload);

      // Test if parsing succeeds.
      if (!root.success()) {
        Serial.println("parseObject() failed");
        Serial.println(payload);
        return;
      }

      //Some debugging lines below:
      //      Serial.println(payload);
      //      root.printTo(Serial);

      //Using .dot syntax, we refer to the variable "location" which is of
      //type GeoData, and place our data into the data structure.

      location.ip = root["ip"].as<String>();            //we cast the values as Strings b/c
      location.cc = root["country_code"].as<String>();  //the 'slots' in GeoData are Strings
      location.cn = root["country_name"].as<String>();
      location.rc = root["region_code"].as<String>();
      location.rn = root["region_name"].as<String>();
      location.cy = root["city"].as<String>();
      location.lt = root["latitude"].as<String>();
      location.ln = root["longitude"].as<String>();

    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
    }
  }
}

