// KeyPad_Demo_4x4 (C)2013 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// A quick demo of most of the library functions.
// Open the serial monitor (115200 baud) to see the output.
//
// IBridge:  4 x 4
//    Cols:  4,  5, 6, 7
//    Rows: A5, A4, 2, 3

// Include our library :)
#include <KeyPad.h>

// Setup for a 4x4 keypad
KeyPad  myKeys(4, 4);

void setup()
{
  myKeys.setColPins(4, 5, 6, 7);
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
