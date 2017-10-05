#include <SoftwareSerial.h> //include the software serial library
byte statusLed    = 13;

byte sensorInterrupt = 0;  // 0 = digital pin 2
byte sensorPin       = 2;

// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.
float calibrationFactor = 4.5;

volatile byte pulseCount;  

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned int frac;
unsigned long oldTime;


SoftwareSerial esp8266(3, 4); //set the software serial pins RX pin = 3, TX pin = 4 
//definition of variables
#define SSID "Marketing" //replace x with your wifi network name
#define PASS "qweasdzxc" //replace x with your wifi network password


String sendAT(String command, const int timeout)
{
  String response = "";
  esp8266.print(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (esp8266.available())
    {
      char c = esp8266.read();
      response += c;
    }
  }
  
    Serial.print(response);
  
  return response;
}

void connectwifi(){
  sendAT("AT\r\n", 1000);
  sendAT("AT+CWMODE=1\r\n", 1000); //call sendAT function to set ESP8266 to station mode
  sendAT("AT+CWJAP=\""SSID"\",\""PASS"\"\r\n", 2000); //AT command to connect wit the wifi network

  while(!esp8266.find("OK")) { //wait for connection
  } 
  sendAT("AT+CIFSR\r\n", 1000); //AT command to print IP address on serial monitor
  sendAT("AT+CIPMUX=0\r\n", 1000); //AT command to set ESP8266 to multiple connections
}

void setup()
{

  Serial.begin(9600);// begin the serial communication with baud of 9600
  esp8266.begin(9600);// begin the software serial communication with baud rate 9600
  
  sendAT("AT+RST\r\n", 2000); // call sendAT function to send reset AT command
  connectwifi();

  // Set up the status LED line as an output
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH);  // We have an active-low LED attached
  
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}

void loop(){
  // put your main code here, to run repeatedly:
if((millis() - oldTime) > 1000)    // Only process counters once per second
  { 
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);
        
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
      
    
    
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print(".");             // Print the decimal point
    // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
    frac = (flowRate - int(flowRate)) * 10;
    Serial.print(frac, DEC) ;      // Print the fractional part of the variable
    Serial.print("L/min");
    // Print the number of litres flowed in this second
    Serial.print("  Current Liquid Flowing: ");             // Output separator
    Serial.print(flowMilliLitres);
    Serial.print("mL/Sec");

    // Print the cumulative total of litres flowed since starting
    Serial.print("  Output Liquid Quantity: ");             // Output separator
    Serial.print(totalMilliLitres);
    Serial.println("mL"); 

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }
 String F =String(frac);
  String FM =String(flowMilliLitres);
   String TM =String(totalMilliLitres);
  updateTS(F,FM,TM); // call the function to update Thingspeak channel
  delay(3000);
}

/*
Insterrupt Service Routine
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}

void updateTS(String F,String FM,String TM){
  Serial.println("");
  sendAT("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80\r\n", 1000);    
  delay(2000);
  String cmdlen;
  String cmd="GET /update?key=UTVHT9PY3WBCYH6Y&field1="+F+"&field2="+FM+"&field3="+TM+"\r\n"; // update the temprature and humidity values on thingspeak url,replace xxxxxxx with your write api key
   cmdlen = cmd.length();
  sendAT("AT+CIPSEND="+cmdlen+"\r\n", 2000);
   esp8266.print(cmd);
   sendAT("AT+CIPCLOSE\r\n", 2000);
   Serial.println("");
    
 
}




