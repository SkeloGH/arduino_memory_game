#define PLAYER_WAIT_TIME 2000  // The time in milliseconds allowed between button presses

bool _input_active = false;         // Used to check if a button is pressed
bool _await_input = false;          // Is the program _waiting for the user to press a button
bool _is_game_over = false;         // Used to indicate to the program that once the player lost
bool _is_wait_expired = false;      // Used to indicate the elapsed time's up.
byte _sequence[100];                // Initialize storage for the light _sequence
byte _seq_length = 0;               // Current length of the _sequence
byte _seq_match_count = 0;          // The number of times that the player has pressed a (correct) button in a given turn
byte _last_input = 0;               // Last input from the player
byte _expected_input = 0;           // The pin number expected for the player to input a signal to
byte _sound_pin = 5;                // Speaker output
byte _total_pins = 2;               // Number of buttons/LEDs (While working on this, I was using only 2 LEDs)
                                    // You could make the game harder by adding an additional LED/button/resistors combination.
byte _pins[] = {13, 8};             // Button input pins and LED ouput pins - change these values if you want to connect your buttons to other pins
                                    // The number of elements must match _total_pins below
long _last_input_time = 0;          // Timer variable for the delay between user inputs

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

void readInputs(){
  ///
  /// Detect if the buttons are pressed or not
  ///

  for(int i = 0; i < _total_pins; i++){
    byte current_input = _pins[i];
    bool is_active = digitalRead(current_input) == HIGH;

    if(is_active){
      _last_input = current_input;
      _input_active = true;
      _last_input_time = millis();

      if(current_input != _expected_input){
        _is_game_over = true;
      }

      Serial.print("Read: ");
      Serial.println(current_input);
      break;
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
  _seq_match_count = 0;
  _last_input = 0;
  _expected_input = 0;
  _input_active = false;
  _await_input = false;
  _is_game_over = false;
  flash(150);
}

void incrementSequence(){
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

void terminateGame(){
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

void verifyTurnFinished(){
  ///
  /// Turn validation actions
  /// - Reset the input flag to allow new events to be captured
  /// - Verify the turn input is valid
  /// - Increase the matching sequence counter
  /// - Acknowledge user input with beep feedback
  /// - Check if user input sequence is complete
  /// - Stop capturing inputs and let program continue
  ///
  _input_active = false;

  if(_last_input == _expected_input){
    _seq_match_count++;
    beep(10);
  }

  if(_seq_match_count == _seq_length){
    _await_input = false;
  }
}

void arduinoTurn(){
  ///
  /// Arduino's turn actions:
  /// - Set a time span for delivering new instructions
  /// - Set all the pins mode as output
  /// - Define the new _sequence
  /// - Show the _sequence to the player
  /// - Set Wait to true as it's now going to be the turn of the player
  /// - Initialize measuring of the player's response time
  /// - Reset the number of times that the player has pressed a button
  /// - Make a beep for the player to be aware
  ///
  delay(1500);
  setAllPinsDirection(OUTPUT);
  incrementSequence();
  playSequence();
  _await_input = true;
  _last_input_time = millis();
  _seq_match_count = 0;
  beep(50);
}

void playerTurn(){
  ///
  /// Player's turn actions:
  /// - Check if time's up
  /// - Define the expected from the player
  /// - Setup pins into input mode
  /// - Read pins for input
  /// - Handle game termination early
  /// - Otherwise, check if the player is done passing input
  /// - Verify player turn is over
  ///
  _is_wait_expired = millis() - _last_input_time > PLAYER_WAIT_TIME;
  _expected_input = _sequence[_seq_match_count];

  setAllPinsDirection(INPUT);

  if(!_input_active){
    Serial.print("Expected: ");
    Serial.println(_expected_input);
    readInputs();
  }

  if(_is_wait_expired || _is_game_over){
    terminateGame();
    return;
  }

  if(_input_active && digitalRead(_last_input) == LOW){
    verifyTurnFinished();
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
  if(!_await_input){
    arduinoTurn();
  }else{
    playerTurn();
  }
}
