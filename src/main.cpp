#include <Arduino.h>

// 함수 프로토타입 선언
void handleCH3();                // 밝기 제어 인터럽트 핸들러
void handleCH9();                // ON/OFF 제어 인터럽트 핸들러
void updateSpectrumColor();      // RGB 색상 변환 함수

// ===================== 핀 설정 =====================
const int ch3Pin = 2;    // CH3: LED 밝기 제어용 PWM 입력 (외부 인터럽트 가능 핀)
const int ch9Pin = 3;    // CH9: LED ON/OFF 제어용 PWM 입력 (외부 인터럽트 가능 핀)
const int ch1Pin = 7;    // CH1: RGB 색상 제어용 PWM 입력 (핀체인지 인터럽트 사용)

const int ledPin = 6;    // 밝기 제어 LED (PWM 출력 가능 핀)
const int ledPin2 = 9;   // ON/OFF 제어 LED
const int redPin = 5;    // RGB Red
const int greenPin = 10; // RGB Green
const int bluePin = 11;  // RGB Blue

// ===================== PWM 측정용 변수 =====================
volatile unsigned long ch3Start = 0, ch3Width = 0;
volatile bool ch3Updated = false;

volatile unsigned long ch9Start = 0, ch9Width = 0;
volatile bool ch9Updated = false;

volatile unsigned long ch1Start = 0, ch1Width = 0;
volatile bool ch1Updated = false;
volatile bool ch1LastState = LOW;

// ===================== 동작 변수 =====================
int brightness = 0;   // 밝기 값 (0~255)
float hue = 0;        // 색상 값 (0~360)

// ===================== 초기 설정 =====================
void setup() {
  Serial.begin(9600);

  // 입력핀 설정
  pinMode(ch3Pin, INPUT);
  pinMode(ch9Pin, INPUT);
  pinMode(ch1Pin, INPUT);

  // 출력핀 설정
  pinMode(ledPin, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // 외부 인터럽트 연결
  attachInterrupt(digitalPinToInterrupt(ch3Pin), handleCH3, CHANGE); // 밝기용
  attachInterrupt(digitalPinToInterrupt(ch9Pin), handleCH9, CHANGE); // ON/OFF용

  // 핀체인지 인터럽트 활성화 (CH1 = D7 = PCINT23)
  PCICR |= (1 << PCIE2);        // PORTD 핀 인터럽트 허용
  PCMSK2 |= (1 << PCINT23);     // D7(=PCINT23)만 허용
}

// ===================== 메인 루프 =====================
void loop() {
  // ========== [김예진 담당] LED 밝기 제어 ==========
  if (ch3Updated) {
    ch3Updated = false;

    // PWM 펄스 폭(μs)을 0~255로 매핑하여 밝기 조절
    if (ch3Width >= 1000 && ch3Width <= 2000) {
      brightness = map(ch3Width, 1000, 2000, 0, 255);
      analogWrite(ledPin, brightness);  // 밝기 적용
      Serial.print("CH3 PWM: ");
      Serial.print(ch3Width);
      Serial.print(" us → Brightness: ");
      Serial.println(brightness);
    } else {
      analogWrite(ledPin, 0); // 유효하지 않은 신호면 끄기
    }
  }

  // ========== [김예진 담당] LED ON/OFF 제어 ==========
  if (ch9Updated) {
    ch9Updated = false;

    Serial.print("CH9 PWM: ");
    Serial.print(ch9Width);
    Serial.print(" us → ");

    if (ch9Width >= 1500) {
      digitalWrite(ledPin2, HIGH);   // 1500us 이상이면 LED ON
      Serial.println("LED ON");
    } else if (ch9Width > 500) {
      digitalWrite(ledPin2, LOW);    // 500~1500us 사이면 LED OFF
      Serial.println("LED OFF");
    } else {
      Serial.println("NO SIGNAL");   // 너무 짧으면 신호 없음으로 간주
    }
  }

  // ========== [팀원 정이지 담당] RGB 색상 전환 ==========
  if (ch1Updated) {
    ch1Updated = false;

    if (ch1Width >= 1000 && ch1Width <= 2000) {
      hue = map(ch1Width, 1000, 2000, 0, 360);  // PWM 신호 → 0~360도 색상
      Serial.print("CH1 PWM: ");
      Serial.print(ch1Width);
      Serial.print(" us → Hue: ");
      Serial.println(hue);
    }
  }

  // 현재 hue 값에 따라 RGB 색상 조절
  updateSpectrumColor();
}

// ===================== 외부 인터럽트 핸들러 =====================
void handleCH3() {
  if (digitalRead(ch3Pin) == HIGH) {
    ch3Start = micros();
  } else {
    ch3Width = micros() - ch3Start;
    ch3Updated = true;
  }
}

void handleCH9() {
  if (digitalRead(ch9Pin) == HIGH) {
    ch9Start = micros();
  } else {
    ch9Width = micros() - ch9Start;
    ch9Updated = true;
  }
}

// ===================== 핀체인지 인터럽트 핸들러 (CH1 색상) =====================
ISR(PCINT2_vect) {
  unsigned long now = micros();
  bool ch1State = PIND & (1 << PIND7);  // D7 상태 확인

  if (ch1State != ch1LastState) {
    ch1LastState = ch1State;

    if (ch1State == HIGH) {
      ch1Start = now;
    } else {
      ch1Width = now - ch1Start;
      ch1Updated = true;
    }
  }
}

// ===================== HSV → RGB 변환 및 출력 =====================
void updateSpectrumColor() {
  float r, g, b;
  int i = int(hue / 60.0) % 6;
  float f = (hue / 60.0) - i;
  float q = 1 - f;

  switch (i) {
    case 0: r = 1; g = f; b = 0; break;
    case 1: r = q; g = 1; b = 0; break;
    case 2: r = 0; g = 1; b = f; break;
    case 3: r = 0; g = q; b = 1; break;
    case 4: r = f; g = 0; b = 1; break;
    case 5: r = 1; g = 0; b = q; break;
  }

  analogWrite(redPin,   int(r * 255));
  analogWrite(greenPin, int(g * 255));
  analogWrite(bluePin,  int(b * 255));
}
