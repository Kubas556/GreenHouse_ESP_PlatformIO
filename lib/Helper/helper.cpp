#include <Arduino.h>
#include "helper.h"
using std::vector;
//======================================================================
// HELPING FUNCTIONS
//======================================================================
//https://forum.arduino.cc/index.php?topic=41389.0 credit
char* subStr (char* str, char *delim, int index) {
  char *act, *sub, *ptr;
  static char copy[50];
  int i;

  // Since strtok consumes the first arg, make a copy
  strcpy(copy, str);

  for (i = 1, act = copy; i <= index; i++, act = NULL) {
     //Serial.print(".");
     sub = strtok_r(act, delim, &ptr);
     if (sub == NULL) break;
  }
  return sub;

}

 String getArguments(String data,  int index) {
  char separator = '|';
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

vector<String> getParsedCommand(String data) {
    int count = 1;
    char separator = '|';

    for (size_t i = 0; i < (data.length()-1); i++)
    {
      if(data.charAt(i) == separator)
      count++;
    }

    const size_t count2 = count;

    vector<String> ret;
    
    for (size_t i = 0; i < count; i++)
    {
      ret.push_back(getArguments(data,i));
    }
    
    return ret;
};
//=======================================================================