// --------------BUTTON FOR TURNING OFF DISPLAY BACKLIGHT
   void lcd_on_off(){
   buttonState = digitalRead(buttonPin);
   if (prevbuttonState != buttonState) {
      // check if the pushbutton is pressed (different)
      // if the same, then change routine can be skipped
      // if it is, the buttonState is HIGH, display is off:
 
    if (buttonState == HIGH) 
    {     
      // turn display off    
      //lcd.noDisplay();
      lcd.noBacklight();  
     } 
    
    else 
    {
      // turn LED on:
      //lcd.display();
      lcd.backlight();  
    }
    prevbuttonState = buttonState;    //records the new status
   }
    return;
   }
   
   
 //----------------END OF DISPLAY BUTTON   
  
