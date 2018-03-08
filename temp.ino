float temp(){

/*---------------GETS AND CALCULATES CELSIUS TEMPERATURE
 Wait a few seconds between measurements.
  Reading temperature or humidity takes about 250 milliseconds!
   Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  */
  delay(2000);

  
  float h = dht.readHumidity();
  float t = dht.readTemperature()-1.2;             // Read temperature as Celsius (the default)
  float f = dht.readTemperature(true)-(1.2*9/5);    // Read temperature as Fahrenheit (isFahrenheit = true)
 
  if (isnan(h) || isnan(t) || isnan(f)) {           // Check if any reads failed and exit early (to try again).
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  
  //float hif = dht.computeHeatIndex(f, h);           // Compute heat index in Fahrenheit (the default)
  //float hic = dht.computeHeatIndex(t, h, false);    // Compute heat index in Celsius (isFahreheit = false)

/*
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temp: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.print(" *C ");
  Serial.print(hif);
  Serial.println(" *F");
  */
 //----------------END OF TEMPERATURE CALCULATION

 //----------------DISPLAYS TEMPERATURE 
  lcd.setCursor(3,2);
  lcd.print(t);
  lcd.print("C");
  lcd.setCursor(14,2);
  lcd.print(h);
  lcd.print("%");
//  lcd.print(analogRead(0));
//  lcd.print(analogRead(1));
//-----------------END OF TEMPURATURE DISPLAY
return t;
}
