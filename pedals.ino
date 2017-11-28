#define DEBOUNCE 1500

struct key{
  int pin;
  int midiKey;
  int debounce;
  int keySent;
  int value;
};

struct key keys[] = {
  { 2, 24, 0, 0, 0 },  // C
  { 3, 25, 0, 0, 0 },  // Db
  { 4, 26, 0, 0, 0 },  // D
  { 5, 27, 0, 0, 0 },  // Eb
  { 6, 28, 0, 0, 0 },  // E
  { 7, 29, 0, 0, 0 },  // F
  { 8, 30, 0, 0, 0 },  // Gb
  { 9, 31, 0, 0, 0 },  // G
  { 10, 32, 0, 0, 0 },  // Ab
  { 11, 33, 0, 0, 0 },  // A
  { 12, 34, 0, 0, 0 },  // Bb
  { 13, 35, 0, 0, 0 },  // B
  { 19, 36, 0, 0, 0 },  // high C
  {  0,  0, 0, 0, 0 }     // end of list marker
};

struct octaveSwitch{
  int pin;
  int debounce;
  int value;
  char direction[4];
};

struct octaveSwitch octaveSwitches[] = {
  { 18, 0, 0, 'up' }, // up
  { 17, 0, 0, 'down' }, // down
  {  0, 0, 0, 'end' } // end of list marker
};

int octave = 0;
int keyOffset = 0;
int keyVelocity = 100;

int noteON = 144;
int noteOFF = 128;

void setup(){
  //set octave switch pins
  for(int i = 0; octaveSwitches[i].pin != 0; ++i){
    pinMode(octaveSwitches[i].pin, INPUT_PULLUP);
  }

  //set pedal pins
  for(int i = 0; keys[i].pin != 0; ++i){
    pinMode(keys[i].pin, INPUT_PULLUP);
  }

  //the midi channel can be changed by holding down a note on startup
  //to get channel 14-16 you have to hold down the high c and one of the first 3 notes
  setMidiChannel();

  //start serial with midi baudrate 31250
  Serial.begin(31250);    
}

void setMidiChannel(){
	for(int i = 0; keys[i].pin != 0; ++i){
		if(digitalRead(keys[i].pin) == LOW){ //pedal is pressed
			int add = i;
			if(i != 12 && digitalRead(keys[12].pin) == LOW && i < 3){
			//the high c is pressed, and the current pedal in the loop is not the high c and the pin is not higher then 2(we only have 16 channels)
				add + 13; //add 12 so the midi channels 14-16 can be adressed
			}
			noteON + add; //add the number to the noteON value
			noteOFF + add; //add the number to the noteOFF value
			return true; //return true so it doesn't fire again
		}
	}
}

void sendMidi(byte cmd, byte data1, byte data2){
  Serial.write(cmd);
  Serial.write(data1);
  Serial.write(data2);
}

void noteOn(int midiKey){
  sendMidi(noteON, midiKey, keyVelocity);
}

void noteOff(int midiKey){
  sendMidi(noteOFF, midiKey, 0);
}

//void getMidiKey(int key){
//  return key + keyOffset + (octave * 12);
//}

void handleOctaveSwitchEvents(){ 
  for(int i = 0; octaveSwitches[i].pin != 0; ++i){ //for all octave switches
    octaveSwitches[i].value = digitalRead(octaveSwitches[i].pin);
    if(octaveSwitches[i].debounce == 0){ //switch has been off
      if(octaveSwitches[i].value == LOW){ //switch is on now
        octaveSwitches[i].debounce = DEBOUNCE; //set debounce value

        if(octaveSwitches[i].direction == 'up'){
          ++octave;
        }

        if(octaveSwitches[i].direction == 'down'){
          --octave;
        }
        
      }
      else{ //switch has been on
        if(octaveSwitches[i].value == HIGH){ //key is off now
          --octaveSwitches[i].debounce; //decrease the debounce
        }
        else{ //switch has not gone off
          octaveSwitches[i].debounce = DEBOUNCE; //reset debounce
        }
      }
    }
  }
}

void handleKeyEvents(){
  for(int i = 0; keys[i].pin != 0; ++i){ //for all keys
    keys[i].value = digitalRead(keys[i].pin);
    if(keys[i].debounce == 0){ //key has been off
      if(keys[i].value == LOW){ //key is on now
        int key = keys[i].midiKey + keyOffset + (octave * 12);
        noteOn(key); //send note on
        keys[i].keySent = key; //save what key is sent
        keys[i].debounce = DEBOUNCE;  //set debounce value
      }
    }
    else{ //key has been on
      if(keys[i].value == HIGH){ //key is off now
        if(--keys[i].debounce == 0){ //if debounce counter is 0
          noteOff(keys[i].keySent); //send note off, keySent is the correct one even if ovtave or offset have changed
        } 
      }
      else{ //key has not gone off
        keys[i].debounce = DEBOUNCE;//reset debounce
      }
    }
  } 
}

void loop() {
  handleOctaveSwitchEvents();
  handleKeyEvents();
}
