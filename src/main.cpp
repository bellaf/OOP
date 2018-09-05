#include <Arduino.h>

class ClickQueue {
    const byte pin;
    // make these slow for testing
    const unsigned CLICK_DOWN_MS = 250;
    const unsigned CLICK_TOTAL_MS = 1000;

    enum State {
      NOT_CLICKING = 0,
      CLICK_DOWN = 1,
      CLICK_PAUSE = 2
    } state;

    unsigned long clickStartMs;
    int pendingClicks;

  public:
    ClickQueue(byte attachTo) :
      pin(attachTo) {
    }

    void setup() {
      pinMode(pin, OUTPUT);
      state = NOT_CLICKING;
      pendingClicks = 0;
    }

    void loop() {
      switch (state) {
        case NOT_CLICKING:
          if (pendingClicks > 0) {
            pendingClicks --;
            digitalWrite(pin, HIGH);
            clickStartMs = millis();
            state = CLICK_DOWN;
          }
          break;
        case CLICK_DOWN:
          if (millis() - clickStartMs > CLICK_DOWN_MS) {
            digitalWrite(pin, LOW);
            state = CLICK_PAUSE;
          }
          break;
        case CLICK_PAUSE:
          if (millis() - clickStartMs > CLICK_TOTAL_MS) {
            state = NOT_CLICKING;
          }
          break;
      }
    }

    void addAClick() {
      pendingClicks++;
    }

    void addClicks(int clicks) {
      pendingClicks += clicks;
    }

    void cancel() {
      pendingClicks = 0;
      digitalWrite(pin, LOW);
      state = NOT_CLICKING;
    }
};

class Headlamp {
    const byte powerOutPin;
    ClickQueue brightnessClicker;
    boolean isOn;
    int brightness;

  public:
    Headlamp(byte powerOutAttach, byte brightnessOutAttach) :
      powerOutPin(powerOutAttach),
      brightnessClicker(brightnessOutAttach)
    {
    }

    void setup() {
      brightnessClicker.setup();

      pinMode(powerOutPin, OUTPUT);
      isOn = false;
      digitalWrite(powerOutPin, LOW);

      brightness = 0;
    }

    void loop() {
      brightnessClicker.loop();
    }

    void powerToggle() {
      if (isOn) {
        brightnessClicker.cancel();
        digitalWrite(powerOutPin, LOW);
        isOn = false;
      }
      else {
        digitalWrite(powerOutPin, HIGH);
        delay(50);
        brightnessClicker.addClicks(brightness);
        isOn = true;
      }
    }

    void brightnessClick() {
      if (isOn) {
        brightnessClicker.addAClick();
        brightness = (brightness + 1 ) % 5;
      }
    }

} ;

class Button {
    const byte pin;
    int state;
    unsigned long buttonDownMs;

  protected:
    virtual void shortClick() = 0;
    virtual void longClick() = 0;

  public:
    Button(byte attachTo) :
      pin(attachTo)
    {
    }


    void setup() {
      pinMode(pin, INPUT_PULLUP);
      state = HIGH;
    }

    void loop() {
      int prevState = state;
      state = digitalRead(pin);
      if (prevState == HIGH && state == LOW) {
        buttonDownMs = millis();
      }
      else if (prevState == LOW && state == HIGH) {
        if (millis() - buttonDownMs < 50) {
          // ignore this for debounce
        }
        else if (millis() - buttonDownMs < 250) {
          shortClick();
        }
        else  {
          longClick();
        }
      }
    }

};

class HeadlampControlButton: public Button {
    Headlamp &lamp;

  public:
    HeadlampControlButton(byte attachToPin, Headlamp &attachToHeadlamp) :
      Button(attachToPin),
      lamp(attachToHeadlamp) {
    }
  protected:
    void shortClick() {
      // short click
      lamp.brightnessClick();
    }

    void longClick() {
      // long click
      lamp.powerToggle();
    }
};

Headlamp headlamp(10, 11);
HeadlampControlButton button(7, headlamp);

void setup() {
  button.setup();
  headlamp.setup();
}

void loop() {
  button.loop();
  headlamp.loop();
}
