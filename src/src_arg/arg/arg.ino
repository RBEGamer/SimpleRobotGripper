#include <Wire.h>
#include <INA3221.h>

#define SERIAL_SPEED     9600  // serial baud rate
#define PRINT_DEC_POINTS 3       // decimal points to print

#define PIN_I2C_SCL 17
#define PIN_I2C_SDA 16


#define DRV_PWM_A 8
#define DRV_PWM_B 10

#define OPEN_GRIPPER_GPIO 2
#define CLOSE_GRIPPER_GPIO 3

#define GRIPPER_MOVE_DURATION_MAX 2000 //ms from close to open 3 (-1second) seconds max @ 5v
#define GRIPPER_STALL_CURRENT 400 // 400ma consumed if motor stalls
#define 
enum DRIVER_STATE{
  NONE = 0,
  OPEN = 1,
  CLOSE = 2
};
// Set I2C address to 0x41 (A0 pin -> VCC)
INA3221 ina_0(INA3221_ADDR40_GND);
//INA3221 ina_1(INA3221_ADDR41_VCC);

void current_measure_init() {
    ina_0.begin(&Wire);
    ina_0.reset();
    ina_0.setShuntRes(10, 10, 10);
}



DRIVER_STATE target_gripper_state = DRIVER_STATE::OPEN;
DRIVER_STATE read_gripper_state = DRIVER_STATE::OPEN;

void setup() {
    Wire.setSCL(PIN_I2C_SCL);
    Wire.setSDA(PIN_I2C_SDA);
    Serial.begin(SERIAL_SPEED);
    current_measure_init();

  pinMode(OPEN_GRIPPER_GPIO, INPUT_PULLUP);
  pinMode(CLOSE_GRIPPER_GPIO, INPUT_PULLUP);

  analogWrite(DRV_PWM_A, 0);
  analogWrite(DRV_PWM_B, 0);
}


long griper_state_timer = 0;
bool mode_block = false;
int violoation_counter = 0;
void loop() {
   // Serial.printf("I %3.0fma %1.1fV V %1.1fV\r\n", ina_0.getCurrent(INA3221_CH1) * 1000, ina_0.getVoltage(INA3221_CH1));
    if(digitalRead(OPEN_GRIPPER_GPIO) && digitalRead(CLOSE_GRIPPER_GPIO)){
      read_gripper_state = DRIVER_STATE::NONE;
      mode_block = true;
    }
    else if(digitalRead(OPEN_GRIPPER_GPIO) && !digitalRead(CLOSE_GRIPPER_GPIO)){
      read_gripper_state = DRIVER_STATE::CLOSE;
    }else if(!digitalRead(OPEN_GRIPPER_GPIO) && digitalRead(CLOSE_GRIPPER_GPIO)){
      read_gripper_state = DRIVER_STATE::OPEN;
    }else{
      read_gripper_state = DRIVER_STATE::NONE;
      mode_block = false;
    }

  if(read_gripper_state != target_gripper_state && !mode_block){
    target_gripper_state = read_gripper_state;
    griper_state_timer = millis();
    violoation_counter = 0;
  }
    const float consumed_power = ina_0.getCurrent(INA3221_CH1) * 1000;
  Serial.println(consumed_power);

    if(target_gripper_state == DRIVER_STATE::OPEN){
      analogWrite(DRV_PWM_A, 255);
      analogWrite(DRV_PWM_B, 0);
    }else if(target_gripper_state == DRIVER_STATE::CLOSE){
      analogWrite(DRV_PWM_A, 0);
      analogWrite(DRV_PWM_B, 255);
    }else{
      analogWrite(DRV_PWM_A, 0);
      analogWrite(DRV_PWM_B, 0);
    }

    if(target_gripper_state != DRIVER_STATE::NONE &&  ((millis()-griper_state_timer) > GRIPPER_MOVE_DURATION_MAX ||consumed_power > (GRIPPER_STALL_CURRENT*10))){
      violoation_counter++;

      if(violoation_counter > 10){
        target_gripper_state = DRIVER_STATE::NONE;
        Serial.println("TIMEOUT OR CURRENT OVERSHOOT=> RESET");
        mode_block = true;
      }
      
    }
    
  
    delay(100);
}
