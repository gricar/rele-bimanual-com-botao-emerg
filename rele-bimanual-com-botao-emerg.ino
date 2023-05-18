#include <ezButton.h>


// =======================================================================
// --- Mapeamento de Hardware ---
#define leftButtonPin 23
#define rightButtonPin 22
#define emergencyButtonPin 21 //External Interrupt
#define restartButtonPin 19
#define ledRelayPin 33
#define ledBlinkPin 32 //Builtin Led 2
#define ledEmergencyPin 25


// =======================================================================
// --- Variáveis Globais ---
unsigned long DEBOUNCE_TIME = 30;
static bool isRunning = true;
unsigned long leftBtnPressedTime = 0;
unsigned long rightBtnPressedTime = 0;
unsigned long timeStart = 0;
unsigned long timeEmergency = 0;
volatile byte state = LOW; //External Interrupt


// =======================================================================
// --- Protótipo das Funções ---
void blink(int time_interval);
void handleButtons();
void handleRestartButton();
void IRAM_ATTR emergencyStop();


ezButton leftButton(leftButtonPin);
ezButton rightButton(rightButtonPin);
ezButton restartButton(restartButtonPin);


// =======================================================================
// --- Configurações Iniciais ---
void setup() {
  Serial.begin(9600);
  delay(10);
  
  leftButton.setDebounceTime(DEBOUNCE_TIME);
  rightButton.setDebounceTime(DEBOUNCE_TIME);
  restartButton.setDebounceTime(DEBOUNCE_TIME + 4900); //pressionar por 5s para reiniciar

  pinMode(ledRelayPin, OUTPUT);
  pinMode(ledBlinkPin, OUTPUT);
  pinMode(ledEmergencyPin, OUTPUT);  
  pinMode(emergencyButtonPin, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(emergencyButtonPin), emergencyStop, RISING);

  digitalWrite(ledRelayPin, LOW);
  digitalWrite(ledBlinkPin, LOW);
  digitalWrite(ledEmergencyPin, LOW);
}

void loop() {
  Serial.printf("isRunning: %d\n", isRunning);
  if (millis() - timeStart > 500 && isRunning) {
    blink(200);
    handleButtons();
  } else {
    handleRestartButton();
  }
}

// =======================================================================
// --- Desenvolvimento das Funções ---
void blink(int time_interval)
{
  static int previousMillis = 0;

  unsigned long currentMillis = millis();
  
  bool led_state = digitalRead(ledBlinkPin);

  if((currentMillis - previousMillis) >= time_interval)
  {
    digitalWrite(ledBlinkPin, !led_state);
    previousMillis = millis();
  }
}


void handleButtons() {
  unsigned long minTimeToEnableBothBtns = 300;

  leftButton.loop();
  rightButton.loop();

  int leftBtnState = leftButton.getState();
  int rightBtnState = rightButton.getState();

  if (leftBtnState) { // Btn is not pressed
    leftBtnPressedTime = 0;  
  } else if (leftBtnPressedTime == 0) { // Btn is pressed and PressedTime is zero
    leftBtnPressedTime = millis();
  }

  if (rightBtnState) { // Btn is not pressed
    rightBtnPressedTime = 0;  
  } else if (rightBtnPressedTime == 0) { // Btn is pressed and PressedTime is zero
    rightBtnPressedTime = millis();
  }

  unsigned long btnsTimeDiff = abs((long)(leftBtnPressedTime - rightBtnPressedTime));

  if (!leftBtnState && !rightBtnState) { // Btns are pressed
    if (btnsTimeDiff <= minTimeToEnableBothBtns) { // Validate diff between btns
      digitalWrite(ledRelayPin, HIGH);
      Serial.println("Acionamento bimanual correto!");      
    } else {
      Serial.printf("Botoes acionados fora do intervalo limite de %dms\n", minTimeToEnableBothBtns);      
    }
  } else { // Btns are not pressed
    digitalWrite(ledRelayPin, LOW);
    Serial.println("Ambos botoes nao estao pressionados");
  }

  delay(10);
}


void IRAM_ATTR emergencyStop()
{
  isRunning = false;
  timeStart = millis();
  timeEmergency = millis();
  digitalWrite(ledEmergencyPin, HIGH);
  digitalWrite(ledBlinkPin, LOW);
  digitalWrite(ledRelayPin, LOW);
}


void handleRestartButton()
{
  restartButton.loop();

  int restartBtnState = restartButton.getState();
  int minTimeToRestart = 5000; // tempo minimo para restart

  bool restartBtnEnable = millis() - timeEmergency > minTimeToRestart;

  if(restartBtnEnable && !restartBtnState && !isRunning) //Button pullup -> Low level enable led
  {
    isRunning = true;
    digitalWrite(ledEmergencyPin, LOW);
    Serial.println("Reiniciando o sistema...");
  }
}