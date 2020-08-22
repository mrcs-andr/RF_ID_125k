// KeyPad_Demo_3x4 (C)2013 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// A quick demo of most of the library functions.
// Open the serial monitor (115200 baud) to see the output.

// Include our library :)
#include <KeyPad.h>

// Setup for a 3x4 keypad
KeyPad  myKeys(3, 4);

void setup()
{
  myKeys.setColPins(4, 5, 6);
  myKeys.setRowPins(A5, A4, 2, 3);
  myKeys.setDebounceDelay(50);
  
  Serial.begin(115200);
}

void loop()
{
  char key;
  
  key = myKeys.readKeys();
  if (key != 0)
    Serial.print(key);
}
