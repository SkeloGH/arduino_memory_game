const int PLAYER_WAIT_TIME = 2000; // The time in milliseconds allowed between button presses

bool _button_down = false;    // Used to check if a button is pressed
bool _wait = false;           // Is the program _waiting for the user to press a button
bool _should_reset = false;   // Used to indicate to the program that once the player lost
bool _times_up = false;       // Used to indicate the elapsed time's up.
byte _sequence[100];          // Initialize storage for the light _sequence
byte _seq_length = 0;         // Current length of the _sequence
byte _input_count = 0;        // The number of times that the player has pressed a (correct) button in a given turn
byte _last_input = 0;         // Last input from the player
byte _expected_input = 0;     // The pin number expected for the player to input a signal to
byte _sound_pin = 5;          // Speaker output
byte _total_pins = 2;         // Number of buttons/LEDs (While working on this, I was using only 2 LEDs)
                              // You could make the game harder by adding an additional LED/button/resistors combination.
byte _pins[] = {13, 8};       // Button input pins and LED ouput pins - change these values if you want to connect your buttons to other pins
                              // The number of elements must match _total_pins below
long _last_input_time = 0;     // Timer variable for the delay between user inputs

///
/// DEVICE FUNCTIONS
///

void setAllPinsDirection(byte dir){
  ///
  /// Sets all the pins as either INPUT or OUTPUT based on the value of 'dir'
  ///
  for(byte i = 0; i < _total_pins; i++){
    pinMode(_pins[i], dir);
  }
}

void setAllPinsValue(byte val){
  ///
  /// Send the same value to all the LED pins
  ///
  for(byte i = 0; i < _total_pins; i++){
    digitalWrite(_pins[i], val);
  }
}

void beep(byte freq){
  ///
  /// Signals the _sound_pin which can be used with a beeper or indicator
  ///
  analogWrite(_sound_pin, 2);
  delay(freq);
  analogWrite(_sound_pin, 0);
  delay(freq);
}

void flash(short freq){
  ///
  /// Flashes all the LEDs together
  ///
  setAllPinsDirection(OUTPUT); /// We're activating the LEDS now
  for(int i = 0; i < 5; i++){
    setAllPinsValue(HIGH);
    beep(50);
    delay(freq);
    setAllPinsValue(LOW);
    delay(freq);
  }
}

void readPins(){
  _expected_input = _sequence[_input_count];       // - Find the value we expect from the player
  Serial.print("Expected: ");
  Serial.println(_expected_input);

  for(int i = 0; i < _total_pins; i++){            // - Loop through the all the pins and check wether
    byte current_pin = _pins[i];
    byte is_button_pressed = digitalRead(current_pin) != HIGH;

    if(current_pin == _expected_input){
      continue;                               // - Nothing to do, continue iterating
    }
    if(!is_button_pressed){
      _last_input = current_pin;
      _should_reset = true;                       // - Set the _should_reset - this means you lost
      _button_down = true;                          // - This will prevent the program from doing the same thing over and over again
      Serial.print("Read: ");
      Serial.println(_last_input);
    }
  }
  if(digitalRead(_expected_input) == 1){        // The player pressed the right button
    _last_input_time = millis();                     //
    _last_input = _expected_input;
    _input_count++;                             // The user pressed a (correct) button again
    _button_down = true;                            // This will prevent the program from doing the same thing over and over again
    Serial.print("Read: ");
    Serial.println(_last_input);
  }
}

void onButtonDown(){
  _button_down = false;
  delay(20);
  if(_should_reset){                              // If this was set to true up above, you lost
    doLoseProcess();                          // So we do the losing _sequence of events
  }else{
    if(_input_count == _seq_length){                 // Has the player finished repeating the _sequence
      _wait = false;                           // If so, this will make the next turn the program's turn
      _input_count = 0;                         // Reset the number of times that the player has pressed a button
      delay(1500);
    }
  }
}

///
/// GAME FUNCTIONS
///

void resetGame(){
  ///
  /// This function resets all the game variables to their default values
  ///
  _seq_length = 0;
  _input_count = 0;
  _last_input = 0;
  _expected_input = 0;
  _button_down = false;
  _wait = false;
  _should_reset = false;
  flash(150);
}

void setSequence(){
  /// Puts a new random value in the last position of the current _sequence,
  ///  then increases the new _sequence length.
  ///  https://www.arduino.cc/en/Reference/RandomSeed
  ///  https://www.arduino.cc/en/Reference/random
  randomSeed(analogRead(A0));
  _sequence[_seq_length] = _pins[random(0, _total_pins)];
  _seq_length++;
}

void playSequence(){
  ///
  /// Shows the user what must be memorized, looping through the stored _sequence,
  ///  and light the appropriate LED in turn
  ///
  for(int i = 0; i < _seq_length; i++){
      Serial.print("Seq: ");
      Serial.print(i);
      Serial.print("Pin: ");
      Serial.println(_sequence[i]);
      digitalWrite(_sequence[i], HIGH);
      delay(500);
      digitalWrite(_sequence[i], LOW);
      delay(250);
    }
}

void doLoseProcess(){
  ///
  /// The events that occur upon a loss:
  /// - Shows the user the last _sequence.
  /// - Resets everything for a new game.
  ///
  flash(50);
  delay(1000);
  playSequence();
  delay(1000);
  resetGame();
}

void arduinoTurn(){
  ///
  /// Arduino's turn actions:
  /// - Set all the pins mode as output
  /// - Define the new _sequence
  /// - Show the _sequence to the player
  /// - Set Wait to true as it's now going to be the turn of the player
  /// - Initialize measuring of the player's response time
  /// - Make a beep for the player to be aware
  ///
  setAllPinsDirection(OUTPUT);
  setSequence();
  playSequence();
  _wait = true;
  _last_input_time = millis();
  beep(50);
}

void playerTurn(){
  ///
  /// Player's turn actions:
  /// - check if time's up
  ///
  _times_up = millis() - _last_input_time > PLAYER_WAIT_TIME;
  setAllPinsDirection(INPUT);

  if(_times_up){
    doLoseProcess();
    return;
  }

  if(!_button_down){
    readPins();
  }

  if(_button_down && digitalRead(_last_input) == LOW){  // Check if the player released the button
    onButtonDown();
  }
}

///
/// Where the magic happens
///
void setup() {
  ///
  /// Delayed startup after connecting the arduino, in case of getting caught in a loop.
  /// Serial.begin(9600) can be removed as long as all references to `Serial` are removed.
  ///
  delay(2000);
  Serial.begin(9600);
  resetGame();
}

void loop() {
  if(!_wait){
    arduinoTurn();
  }else{
    playerTurn();
  }
}
