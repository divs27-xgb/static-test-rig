#include <HX711.h>
#include <SPI.h>
#include <SD.h>

#define HX_DT  2
#define HX_SCK 3
#define SD_CS  21  
#define gram_cal 0.0046094095
#define ig_pin 4
#define BUFFER_SIZE 256

struct Sample {
  uint32_t t_us;
  int32_t  raw;
};

Sample buffer[BUFFER_SIZE];
volatile uint16_t head = 0;
volatile uint16_t tail = 0;

HX711 scale;
File logFile;

long tare = 0;
inline bool bufferFull() {
  return ((head + 1) % BUFFER_SIZE) == tail;
}

inline bool bufferEmpty() {
  return head == tail;
}

void pushSample(uint32_t t, int32_t raw) {
  if (!bufferFull()) {
    buffer[head] = {t, raw};
    head = (head + 1) % BUFFER_SIZE;
  }
  // else: overflow → should NOT happen if SD is healthy
}
void setup() {
  Serial.begin(230400);
  delay(1000);

  scale.begin(HX_DT, HX_SCK);
  scale.set_gain(128);

  while (!scale.is_ready()) delay(10);
  tare = scale.read_average(100);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed!");
    while (1);
  }

  logFile = SD.open("/thrust.csv", FILE_WRITE);
  logFile.println("time_us,raw");
  logFile.flush();
  // pinMode(ig_pin,OUTPUT);
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.println("Started");

}
void loop() {
  // put your main code here, to run repeatedly:
  // put your main code here, to run repeatedly:
  if(Serial.available()>0){
    int task=Serial.read();
    Serial.println(task);
    if(task==97){
      startlogging();
    }

    while (Serial.available() > 0) {
      Serial.read();
    }
  }
}


void startlogging(){
    digitalWrite(LED_BUILTIN,HIGH);
    Serial.println("Logging started");
    uint32_t start=micros();

while(1){
    if (scale.is_ready()) {
    uint32_t t = micros()-start;
    int32_t raw = (scale.read() - tare)*gram_cal;
    pushSample(t, raw);
  }

  if (!bufferEmpty()) {
    for (int i = 0; i < 16 && !bufferEmpty(); i++) {
      Sample s = buffer[tail];
      tail = (tail + 1) % BUFFER_SIZE;

      logFile.print(s.t_us);
      logFile.print(",");
      logFile.println(s.raw);
    }
    logFile.flush(); 
  }

}
}



