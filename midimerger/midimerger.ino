#include <MIDI.h>        // access to serial (5 pin DIN) MIDI
#include <USBHost_t36.h> // access to USB MIDI devices (plugged into 2nd USB port)

// Create the Serial MIDI ports
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI1);

// Create the ports for USB devices plugged into Teensy's 2nd USB port (via hubs)
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
USBHub hub3(myusb);
USBHub hub4(myusb);
MIDIDevice midi01(myusb);
MIDIDevice midi02(myusb);
MIDIDevice midi03(myusb);
MIDIDevice midi04(myusb);

MIDIDevice * midilist[10] = {
  &midi01, &midi02, &midi03, &midi04
  };

// A variable to know how long the LED has been turned on
elapsedMillis ledOnMillis;
bool activity = false;
midi::MidiType mtype; 


void sendToComputer(byte type, byte data1, byte data2, byte channel, const uint8_t *sysexarray, byte cable);

void setup() {
  //Serial.begin(115200);
  pinMode(13, OUTPUT); // LED pin
  digitalWrite(13, LOW);
  MIDI1.begin(MIDI_CHANNEL_OMNI);
  // Wait 1.5 seconds before turning on USB Host.  If connected USB devices
  // use too much power, Teensy at least completes USB enumeration, which
  // makes isolating the power issue easier.
  delay(1500);
  Serial.println("MidiMerger");
  delay(10);
  myusb.begin();
}

void loop() {
  activity = false;
  if (usbMIDI.read()) {
    // get the USB MIDI message, defined by these 5 numbers (except SysEX)
    byte type = usbMIDI.getType();
    byte channel = usbMIDI.getChannel();
    byte data1 = usbMIDI.getData1();
    byte data2 = usbMIDI.getData2();
    //byte cable = usbMIDI.getCable();

    // forward this message to 1 of the 3 Serial MIDI OUT ports
    if (type != usbMIDI.SystemExclusive) {
      // Normal messages, first we must convert usbMIDI's type (an ordinary
      // byte) to the MIDI library's special MidiType.
      mtype = (midi::MidiType)type;
      MIDI1.send(mtype, data1, data2, channel);
    
    } else {
      // SysEx messages are special.  The message length is given in data1 & data2
      unsigned int SysExLength = data1 + data2 * 256;
      MIDI1.sendSysEx(SysExLength, usbMIDI.getSysExArray(), true);
    }
    activity = true;
  }

  // Next read messages arriving from the (up to) 10 USB devices plugged into the USB Host port
  for (int port=0; port < 4; port++) {
    if (midilist[port]->read()) {
      uint8_t type =       midilist[port]->getType();
      uint8_t data1 =      midilist[port]->getData1();
      uint8_t data2 =      midilist[port]->getData2();
      uint8_t channel =    midilist[port]->getChannel();
      const uint8_t *sys = midilist[port]->getSysExArray();
      activity = true;
      if (type != 250 && type != 251 & type != 252 ){
        sendToComputer(type, data1, data2, channel, sys, 0);
        if(channel == 13 || channel == 16){
          uint8_t type = 176;
          mtype = (midi::MidiType)type;
          MIDI1.send(mtype, 41, data2, channel);
        }
        mtype = (midi::MidiType)type;
        MIDI1.send(mtype, data1, data2, channel);
//              Serial.print("type ");
//              Serial.print(type);
//              Serial.print(", data1 ");
//              Serial.print(data1);
//              Serial.print(", data2 ");
//              Serial.print(data2);
//              Serial.print(", channel ");
//              Serial.print(channel);
//              Serial.println();
      }
    }
  }

  // blink the LED when any activity has happened
  if (activity) {
    digitalWriteFast(13, HIGH); // LED on
    ledOnMillis = 0;
  }
  if (ledOnMillis > 15) {
    digitalWriteFast(13, LOW);  // LED off
  }
}


void sendToComputer(byte type, byte data1, byte data2, byte channel, const uint8_t *sysexarray, byte cable)
{
  if (type != midi::SystemExclusive) {
    usbMIDI.send(type, data1, data2, channel, cable);
  } else {
    unsigned int SysExLength = data1 + data2 * 256;
    usbMIDI.sendSysEx(SysExLength, sysexarray, true, cable);
  }
}
