#include <Servo.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define buzz 5
#define pot A3

#define lockAngle 90
#define unlockAngle 0

Servo lockServo;

LiquidCrystal_I2C lcd (0x27 , 16 , 2);

char keymap[4][4] =
{
  '1' , '2' , '3' , 'A',
  '4' , '5' , '6' , 'B',
  '7' , '8' , '9' , 'C',
  '*' , '0' , '#' , 'D'
};

byte rows[] = {13 , 12 , 11 , 10};
byte cols[] = {9 , 8 , 7 , 6};

Keypad kp (makeKeymap(keymap) , rows , cols , 4 , 4);

byte mode = 0;  // 0 is when in password entery mode
// 1 is when you have access, 2 is when in password changing mode and mode 3 is when you start the locker for the first time

unsigned long lockdownTime;  // A variable for calculating lockdown time

unsigned long alarmTime;  // A variable for calculating alarm frequency intervals

unsigned long lcdUITime;  // A variable for calculating the time for which text will appear on the lcd

unsigned long lcdUIDuration;  // A variable for the duration that the lcd will have a certain text appear on it

bool isPasswordLoaded = false;  // A variable that is used once in the program for loading the password from EEPROM

bool isLockdown = false;  // A variable that decides to innitiate lockdown or stop it

bool isSilent = false;  // A variable that is used for toggleing sound

bool isAlarm = false;  // For Silencing other sounds when the alarm is active

bool lcdDelayActive = false;  // Set to true when the delay is needed, false otherwise

byte attempts = 0;  // After 3 attepts innitiate lockdown

byte currentText = 255;  // Used for choosing which text to display

byte passwordSize;  // Used to load the password from EEPROM and also used in other stuff
char* password;  // A pointer to the password which is stored at the heap
char* input;  // A pointer to the user input which is stored at the heap also

byte indx = 0;  // The size of the user input

byte lastSecond = 255;  // To update the lockdown time displayed on the LCD

void setup()
{
  pinMode(buzz , OUTPUT);
  pinMode(pot , INPUT);
  lockServo.attach(3);
  lcd.init();
  lcd.backlight();

  Serial.begin(9600);
  
  input = nullptr; // Set to null until the user inserts input
}

void loop()
{
  if(EEPROM.read(0) > 16)  // When starting the safe for the first time take a password from the user to save it
  {
    lcdUpdateRow0(1);
    mode = 3;
  }
  else if (!isPasswordLoaded)  // If the password wasn't loaded before, load it
  {
    loadPassword();
    isPasswordLoaded = true;
  }
 
  alarm();  // Starts the alarm if a condition is met

  if(mode == 0)
  {
    if(analogRead(pot) > 2)  // If the safe was opened somehow in mode 0 set isAlarm to true
    {
      isAlarm = true;
    }

    lockServo.write(lockAngle);  // In mode 0 the safe should always be locked
    if(!isLockdown && !isAlarm)  // Will ask you to insert password if no lockdown or alarm is active
      lcdUpdateRow0(1);
  }

  else if(mode == 1)
  {
    lcdUpdateRow0(2);  // In mode 1 always display "Access granted"
    lockServo.write(unlockAngle);  // In mode 1 the lock should always be open
  }

  else if(mode == 2)
  {
    lcdDelayActive = false;  // If there is an active lcd delay, cancel it
    lcdUpdateRow0(1);  // In mode 1 always display "Insert password"
    lockServo.write(unlockAngle);  // Just like in mode 1, the lock should always be open in mode 2
  }

  else if(mode == 3)
  {
    lcdUpdateRow0(1);  // In mode 1 always display "Insert password"
    lockServo.write(unlockAngle);  // Again, servo is always open if not in mode 0
  }
  
  if (!isLockdown)  // Take user input if lockdown isn't active
    takeInput();

  if(attempts > 2)  // Innitialize lockdown if failed attepts exceed 2
  {
    lockdownTime = millis();
    isLockdown = true;
    attempts = 0;
    lcdDelayActive = false;
  }

  lockdown();  // If a condition is met lockdown starts
  lcdTime();  // If a condition is met stop lcd row 0 updates
}

void takeInput()  // Takes input from user and stores it or acts upon it
{
  char key = kp.getKey();
  if(key)
  {
    if(key == 'A' && mode != 1)  // A clears user input
    {
      if(indx > 0)
      {
        if(!isSilent && !isAlarm)
          tone(buzz , 500 , 200);
        
        charClear(input , indx);
        lcdClearRow(1);
      }
    }
    
    else if(key == 'B'  && mode != 1)  // B erases last character from user input
    {
      if(indx > 0)
      {
        if(!isSilent && !isAlarm)
          tone(buzz , 500 , 200);
      
        charPopback(input , indx);
        lcdClearInputChar(indx);
      }
    }
    
    else if(key == 'C')  // C allows the user to change the password (only if user has access)
    {
      if(mode != 0 && mode!= 3)
      {
        if(!isSilent)
          tone(buzz , 500 , 200);
      
        if(mode == 1)
        {
          mode = 2;
        }
        else
        {
          mode = 1;
        }
        charClear(input , indx);
        lcdClearRow(1);
      }
    }
      
    else if (key == 'D')  // D locks the safe (only if user has access and the safe door is closed)
    {
      if (mode != 0 && mode != 3)
        if (analogRead(pot) < 3)
    	  {
          if (!isSilent)
            tone(buzz , 500 , 200);
        
          mode = 0;
          lockServo.write(lockAngle);
          lcdUpdateWithDelay(0 , 2000);
          charClear(input , indx);
          lcdClearRow(1);
    	  }
      
        else
        {
          mode = 1;
          tone(buzz , 1250 , 500);
          lcdUpdateWithDelay(5 , 2000);
          charClear(input , indx);
          lcdClearRow(1);
        }
    }
    
    else if(key == '#' && mode != 1)  // # is for confirming user input, it does so  in different ways depending on mode
    {
      if (indx > 0)
      {
        if(mode == 0)
          if(checkMatch(input , password , indx , passwordSize))
            grantAccess();
          else
            denyAccess();
        else if (mode == 2)
        {
          if (!isSilent)
            tone(buzz , 750 , 125);

          mode = 1;
          setPassword(input , password , indx , passwordSize);
          charClear(input , indx);
          lcdClearRow(1);

          lcdUpdateWithDelay(6 , 2000);
        }
        else if(mode = 3)
        {
          if (!isSilent)
            tone(buzz , 750 , 125);
          
          mode = 1;
          setPassword(input , password , indx , passwordSize);
          charClear(input , indx);
          lcdClearRow(1);

          lcdUpdateWithDelay(6 , 2000);
        }
      }
      else
      {
        if (!isSilent)
          tone(buzz , 1250 , 500);
      }
    }
    
    else if(key == '*'  && !isAlarm)  // * allows the user to toggle sounds
    {
      isSilent = !isSilent;
      if (!isSilent)
      {
        tone(buzz , 1000 , 50);
      }
    }
      
    else if(mode != 1)
    {
      if(indx < 16)
      {
        lcdDelayActive = false;

        if(!isSilent && !isAlarm)
            tone(buzz , 1500 , 50);

        lcd.setCursor(indx , 1);
        lcd.write('*');
        charPushback(input , indx , key);
      }
      else
        if (!isSilent)  // Different sound to warn the user that he can't insert more characters
          tone(buzz , 1250 , 500);
    }
  }
}
  
void charPushback(char*& arr , byte& size , char input)  // Adds a character to the input array
{
  ++size;
  
  char* ptr = new char[size];
  
  if (arr != nullptr)
    memcpy(ptr , arr , size - 1);
  
  ptr[size - 1] = input;
  delete[] arr;
  arr = ptr;
}

void charPopback(char*& arr , byte& size)  // Removes the last character in the input array
{
  if (size > 1)
  {
    --size;
    char* ptr = new char[size];
    
    if(arr != nullptr)
      memcpy(ptr , arr , size);
    
    delete[] arr;
    arr = ptr;
  }
  else
  {
    size = 0;
    delete[] arr;
    arr = nullptr;
  }
}

bool checkMatch(char* input1 , char* input2 , byte size1 , byte size2)  // Checks if two arrays are equal (here we check if password array is equal to input array)
{
  if (!(size1 == size2))
    return false;
  
  for (int i = 0 ; i < size1 ; i++)
  {
    if(input1[i] != input2[i])
      return false;
  }
  return true;
}

void grantAccess()  // When the user enters the correct password, this function is called
{
  if (!isSilent)
    tone(buzz , 750 , 125);

  else
    noTone(buzz);  // If the alarm was active and you insert the correct password the alarm stops
    isAlarm = false;

  attempts = 0;
  
  mode = 1;
  
  charClear(input , indx);
  lcdClearRow(1);
}

void denyAccess()  // If the user enters the wrong password, this function is called
{
  if (!isSilent && !isAlarm)
    tone(buzz , 500 , 200);
  
  attempts++;
  charClear(input , indx);
  lcdClearRow(1);
  
  lcdUpdateWithDelay(3 , 2000);
}

void charClear(char*& arr , byte& size)  // Clears a dynamic array of chars
{
  size = 0;
  delete[] arr;
  arr = nullptr;
}

void lockdown()  // Activates lockdown when isLockdown is true
{
  if(isLockdown)
  {
    lcdUpdateRow0(4);

    byte remainingTime = (millis() - lockdownTime) / 1000;

    if(remainingTime < 30)
    {
      if (remainingTime != lastSecond)
      {
        lcdClearRow(1);
        lcd.print(30 - remainingTime);
        lastSecond = remainingTime;
      }
    }
    else
    {
    isLockdown = false;
    lcdClearRow(1);
    }
  }
}

void charCopyArr(char* source , char*& dest , byte& sizeS , byte& sizeD)  // Copies an array to another one, used for setting a new password
{
  if (source != nullptr && sizeS != 0)
  {
    sizeD = sizeS;
    delete[] dest;
    dest = new char[sizeS];

    
    memcpy(dest , source , sizeS);
  }
}

void alarm()
{
  if (isAlarm)  // If you don't have access but the locker is open somehow
  {
    byte freqPeriod = (millis() - alarmTime) / 100;  // Time in centiseconds
    
      if(freqPeriod < 3)  // Alarm tone changes every 3 centiseconds
        tone(buzz , 1000);
      else if(freqPeriod < 6)
        tone(buzz , 800);
      else
        alarmTime = millis();
    if(!isLockdown)
      lcdUpdateRow0(7);
  }
  else
  {
    alarmTime = millis();
    isAlarm = false;
  }
}

void setPassword(char* source , char*& dest , byte& sizeS , byte& sizeD)
{
  charCopyArr(source , dest , sizeS , sizeD);
  EEPROM.update(0 , sizeD);  // Saves password to EEPROM
  for(int i = 0 ; i < sizeD ; i++)
    EEPROM.update(i + 1 , dest[i]);
}

void loadPassword()  // Loads password from EEPROM
{
  passwordSize = EEPROM.read(0);  // Load the size stored at address 0

  if (passwordSize > 0) 
  {  
    password = new char[passwordSize];

    for (int i = 0; i < passwordSize; i++)
    {
      password[i] = EEPROM.read(i + 1);
    }
  }
}

void lcdUpdateRow0(byte nextText)  // Checks if what we have on the lcd is different from what we want to display, usefull for avoiding constantly updating lcd
{
  if(lcdDelayActive)
    return;

  if (currentText == nextText)
    return;
  
  currentText = nextText;
  lcdClearRow(0);

  switch (currentText)  // Each sentence has its number
  {
    case 0:
      lcd.print("Safe locked");
      break;
    case 1:
      lcd.print("Insert password");
      break;
    case 2:
      lcd.print("Access granted");
      break;
    case 3:
      lcd.print("Access denied");
      break;
    case 4:
      lcd.print("Lockdown ends in");
      break;
    case 5:
      lcd.print("Close safe");
      break;
    case 6:
      lcd.print("Password saved");
      break;
    case 7:
      lcd.print("GO AWAY THIEF");
      break;
    default:
      Serial.println("Wrong argument at function lcdUpdateRow0");
  }
}

void lcdTime()  // A function that stops lcd updates for a while
{
  if(lcdDelayActive)
    {
      unsigned long timePassed = millis() - lcdUITime;
      if (timePassed >= lcdUIDuration)
        lcdDelayActive = false;
    }
}

void lcdUpdateWithDelay(byte text , unsigned long duration)  // For text that will be shown for a certain amount of time
{
  lcdDelayActive = false;
  lcdUpdateRow0(text);
  lcdUITime = millis();
  lcdDelayActive = true;
  lcdUIDuration = duration;
}

void lcdClearRow(int row)  // This function clears a specified row in the lcd
{
  lcd.setCursor(0 , row);

  lcd.print("                ");  // Prints 16 spaces
  lcd.setCursor(0 , row);
}

void lcdClearInputChar(int index)  // Erases a character from the lcd
{
  lcd.setCursor(index , 1);
  lcd.write(' ');
  lcd.setCursor(index , 1);
}