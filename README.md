# Locker Management System

## Description  
Locker management system for Arduino-controlled smart locker.

## Features  
- Changeable password stored in EEPROM  
- Alarm system triggers when the locker is forcefully opened  

## Usage  
1. Power on the Arduino system.  
2. Use the 4x4 keypad to enter the password.  
3. If the password is correct, the servo unlocks the locker.  
4. If the password is incorrect, access is denied.  
5. If the locker is forcefully opened, the buzzer alarm will sound.  
6. The LCD displays status messages and prompts during operation.

### Special keys  
- **A**: Clear input  
- **B**: Erase last inserted character  
- **C**: Change password (only works if you have access)  
- **D**: Lock safe (only works if you have access and safe door is closed)  
- ***:** Toggle mute for keypad sounds (alarm sound can't be muted)  
- **#**: Confirm input  

## Technologies Used  
- **Arduino Uno** — microcontroller platform  
- **4x4 Matrix Keypad** — for user input  
- **Servo Motor** — controls the locking mechanism  
- **Buzzer** — for alarm notifications  
- **16x2 LCD with I2C Module** — displays status and prompts  
- **EEPROM** — stores the password securely  
- **Arduino IDE** — development environment  

## Installation (for Arduino Uno)  
- Connect a buzzer to pin 5  
- Connect a servo (acting as the lock) signal pin to pin 3  
- Connect 4x4 keypad pins (Row1 to Col4) to Arduino pins 13 to 6  
- Connect an LCD with I2C module to analog pins A5 (SCL) and A4 (SDA)  
- Connect a potentiometer to pin A3; place it below the hinges of the door so it rotates with the door to detect if the door is closed or open

# Contact  
You can reach me at: [alishouman2006@gmail.com](mailto:alishouman2006@gmail.com)

#Note:
LCD Ui is not complete yet, stay tuned for future updates
