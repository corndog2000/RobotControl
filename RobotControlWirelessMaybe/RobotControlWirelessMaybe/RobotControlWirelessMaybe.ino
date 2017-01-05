//My attempt at controlling Spark motor controllers with an Arduino Mega, wish me luck

//A delay of 1000 Microseconds is Full Reverse
//A delay of 1000 to 1460 Microseconds is Proportional Reverse
//A delay of 1460 to 1540 Microseconds is neutral
//A delay of 1540 to 2000 Microseconds is Proportional Forward
//A delay of 2000 Microseconds is Full Forward

//#include <Wire.h>       // for I2C
#include <VirtualWire.h>  // for 433Mhz
#include <XBOXRECV.h>     //For Xbox controller with USB shield

#ifdef dobogusinclude     //Other Includes for USB shield
#include <spi4teensy3.h>
#include <SPI.h>
#endif

USB Usb;      //Initialization for USB shield
XBOXRECV Xbox(&Usb);

int backLeft = 6;        //Back Left Motor pin
int backLeftSpeed = 1460; //Back Left Motor starting speed

int frontLeft = 5;        //Front Left Motor pin
int frontLeftSpeed = 1460; //Front Left Motor starting speed

int backRight = 4;        //Back Right Motor pin
int backRightSpeed = 1460; //Back Right Motor starting speed

int frontRight = 3;        //Front Right Motor pin
int frontRightSpeed = 1460; //Front Right Motor starting speed

short joyXPre = 0;          //joystick values before processing
short joyYPre = 0;

short joyX = 0;             //joyX < 0 = Left, joyX > 0 = Right
short joyY = 0;             //joyY > 0 = Forward, joyY < 0 = Reverse

int Speed = 1460;           //Starting speed for Serial communication

const int lowNeutral = 1460;
const int highNeutral = 1540;

//bool received = false;    //Used for I2C communication

void setup()
{
  Usb.Init();   //Initialize USB shield
  delay(1000);  //Wait for initialization before continueing

  pinMode(backLeft, OUTPUT);    //Makes the pins outputs
  pinMode(frontLeft, OUTPUT);
  pinMode(backRight, OUTPUT);
  pinMode(frontRight, OUTPUT);

  pinMode(0, INPUT);  //Input for Left/Right joystick
  pinMode(1, INPUT);  //Input for Forward/Reverse joystick

  Serial.begin(250000);
  //Wire.begin(8);                //Initializes the I2C bus
  //Wire.onReceive(receiveEvent); //Call receiveEvent when you get data

  // Initialize VirtualWire
  //vw_set_ptt_inverted(true);  // Required for DR3100
  //vw_setup(4800);             // Bits per sec
  //vw_rx_start();              // Start the receiver PLL running

  delay(1000);
}

//Back Left
unsigned long endTimeBackLeftHigh = 0;
unsigned long endTimeBackLeftLow = 0;
bool backLeftL = false;
bool initBackLeft = false;

//Front Left
unsigned long endTimeFrontLeftHigh = 0;
unsigned long endTimeFrontLeftLow = 0;
bool frontLeftL = false;
bool initFrontLeft = false;

//Back Right
unsigned long endTimeBackRightHigh = 0;
unsigned long endTimeBackRightLow = 0;
bool backRightL = false;
bool initBackRight = false;

//Front Right
unsigned long endTimeFrontRightHigh = 0;
unsigned long endTimeFrontRightLow = 0;
bool frontRightL = false;
bool initFrontRight = false;

bool strafe = false;

int motorsRunning = 0;

void loop()
{
  bool serialControl = false;
  bool joystickControl = true;
  bool wiredJoystick = false;
  bool EbayReciever = false;    //433 Mhz reciever
  bool XboxWirelessControl = true;
  bool sendToMotor = true;

  const int controlNum = 0; //Use the Xbox controller number 0, I only have one controller

  if (serialControl)
  {
    // This code is used when controlling using serial
    if (Serial.available() > 0) {
      char fb = Serial.read();
      if (fb == 'a') {
        Speed = Serial.parseInt();
        backLeftSpeed = frontLeftSpeed = backRightSpeed = frontRightSpeed = Speed;
      }
      else {
        char lr = Serial.read();
        Speed = Serial.parseInt();
        if (fb == 'b') {
          if (lr == 'l') {
            backLeftSpeed = Speed;
          }
          else {
            backRightSpeed = Speed;
          }
        }
        else {
          if (lr == 'l') {
            frontLeftSpeed = Speed;
          }
          else {
            frontRightSpeed = Speed;
          }
        }
      }
      Serial.println(Speed);
    }
  }
  if (joystickControl)
  {
    if (wiredJoystick)
    {
      joyYPre = analogRead(1);
      joyY = map(joyYPre, 280, 800, 1000, 2000);  //Forward/Reverse
      joyXPre = analogRead(0);
      joyX = map(joyXPre, 280, 725, -150, 150);     //Left/Right
    }
    else if (motorsRunning <= 0 && EbayReciever) // Wireless Joystick
    {
      uint8_t buf[6];
      uint8_t buflen = 6;

      if (vw_get_message(buf, &buflen)) // Non-blocking
      {
        joyXPre = buf[1] << 8 | buf[0];
        joyYPre = buf[3] << 8 | buf[2];
        strafe = !(buf[5] << 8 | buf[4]);

        joyY = map(joyYPre, 0, 1023, 1000, 2000);
        joyX = map(joyXPre, 0, 1023, -150, 150);

        initBackLeft = initFrontLeft = initBackRight = initFrontRight = true;  //We need to initialize the PWM for each motor
        backLeftL = frontLeftL = backRightL = frontRightL = false;    //Have we set each motor to low?
        motorsRunning = 4;
      }
    }
    else if (motorsRunning <= 0 && XboxWirelessControl) //Xbox Controller
    {
      Usb.Task();
      if (Xbox.XboxReceiverConnected) {
        if (Xbox.Xbox360Connected[controlNum]) {

          //We have to use the "Val" to seperate it from LeftHatX which is a different varible in the library
          //The "LeftHat" is the left joystick
          int LeftHatXValPre = 0;   //Joystick value before maping the values
          int LeftHatYValPre = 0;

          if (Xbox.getAnalogHat(LeftHatX, controlNum) > 7500 || Xbox.getAnalogHat(LeftHatX, controlNum) < -7500) {
            LeftHatXValPre = (Xbox.getAnalogHat(LeftHatX, controlNum));
            //Serial.println(LeftHatXValPre);
          }
          if (Xbox.getAnalogHat(LeftHatY, controlNum) > 7500 || Xbox.getAnalogHat(LeftHatY, controlNum) < -7500) {
            LeftHatYValPre = (Xbox.getAnalogHat(LeftHatY, controlNum));
            //Serial.println(LeftHatYValPre);
          }
          strafe = (Xbox.getButtonPress(A, controlNum));
          //Serial.println(strafe ? "strafe" : "no strafe");

          joyY = map(LeftHatYValPre, -32768, 32768, 1000, 2000);
          joyX = map(LeftHatXValPre, -32768, 32768, -150, 150);

          initBackLeft = initFrontLeft = initBackRight = initFrontRight = true;  //We need to initialize the PWM for each motor
          backLeftL = frontLeftL = backRightL = frontRightL = false;    //Have we set each motor to low?
          motorsRunning = 4;
        }
      }
    }

    bool drivingForward = joyY > highNeutral;  //Are we driving?
    bool drivingReverse = joyY < lowNeutral;

    //Serial.print("joyX="); Serial.println(joyX);
    //Serial.print("joyY="); Serial.println(joyY);
    //Serial.println(drivingForward);
    //Serial.println(drivingReverse);
    //Serial.println(joyXPre);
    //Serial.println(joyYPre);


    backLeftSpeed = frontLeftSpeed = backRightSpeed = frontRightSpeed = joyY;   //Sets the speed for all motors based on the Forward/Reverse of the joystick

    if (abs(joyX) > 7) {    //Am I moving the joystick left or right?
      if (joyX < 0 && strafe /*&& (!drivingForward && !drivingReverse)*/) {     //Strafe Left
        backRightSpeed = lowNeutral + joyX;
        frontRightSpeed = highNeutral + abs(joyX);
        backLeftSpeed = highNeutral + abs(joyX);
        frontLeftSpeed = lowNeutral + joyX;
      }
      else if (joyX > 0 && strafe /*&& (!drivingForward && !drivingReverse)*/) {      //Strafe Right
        backRightSpeed = highNeutral + joyX;
        frontRightSpeed = lowNeutral - joyX;
        backLeftSpeed = lowNeutral - joyX;
        frontLeftSpeed = highNeutral + joyX;
      }
      else if (joyX < 0 && !strafe && (!drivingForward && !drivingReverse)) {     //Zero point turn Left
        backRightSpeed = highNeutral + abs(joyX);   //highNeutral for forwards movement
        frontRightSpeed = highNeutral + abs(joyX);  //lowNeutral for backwords movement
        backLeftSpeed = lowNeutral + joyX;
        frontLeftSpeed = lowNeutral + joyX;
      }
      else if (joyX > 0 && !strafe && (!drivingForward && !drivingReverse)) {      //Zero point turn Right
        backRightSpeed = lowNeutral - joyX;
        frontRightSpeed = lowNeutral - joyX;
        backLeftSpeed = highNeutral + joyX;
        frontLeftSpeed = highNeutral + joyX;
      }
      else if (joyX < 0) {       //Turning Left
        backRightSpeed += drivingForward ? abs(joyX) : joyX;
        frontRightSpeed += drivingForward ? abs(joyX) : joyX;
      }
      else if (joyX > 0) {                //Turning Right
        backLeftSpeed += drivingForward ? joyX : -joyX;
        frontLeftSpeed += drivingForward ? joyX : -joyX;
      }
    }
  }
  /*if (!sendToMotor && motorsRunning > 0) {
   Serial.print("bl ="); Serial.println(backLeftSpeed);
   Serial.print("fl ="); Serial.println(frontLeftSpeed);
   Serial.print("br ="); Serial.println(backRightSpeed);
   Serial.print("fr ="); Serial.println(frontRightSpeed);
   motorsRunning = 0; 
  }*/
  if (sendToMotor && motorsRunning > 0) {
    //My owm PWM code because analogWrite 0-255 is not precise enough

    if (initBackLeft) {   //Back Left
      endTimeBackLeftHigh = micros() + backLeftSpeed;   //How many microseconds since startup until we end sending HIGH
      digitalWrite(backLeft, HIGH);     //Set motor to HIGH
      endTimeBackLeftLow = 4294967295;        //Crazy big number since we don't yet know when to end sending LOW
      initBackLeft = false;             //We initialized the back left motor
    }
    else {
      unsigned long t = micros();       //Used for comparason
      if (t > endTimeBackLeftLow) {           //Done sending LOW
        motorsRunning--;                //This motor is done being signaled
      }
      else if (t > endTimeBackLeftHigh) {      //Done sending HIGH for PWM wave
        if (!backLeftL) {
          digitalWrite(backLeft, LOW);    //Set motor to LOW
          endTimeBackLeftLow = micros() + backLeftSpeed; //How many microseconds since startup until we end sending LOW
        }
        backLeftL = true;   //Make sure we only set to LOW one time
      }
    }

    if (initFrontLeft) {   //Front Left
      endTimeFrontLeftHigh = micros() + frontLeftSpeed;
      digitalWrite(frontLeft, HIGH);
      endTimeFrontLeftLow = 4294967295;
      initFrontLeft = false;
    }
    else {
      unsigned long t = micros();
      if (t > endTimeFrontLeftLow) {
        motorsRunning--;
      }
      else if (t > endTimeFrontLeftHigh) {
        if (!frontLeftL) {
          digitalWrite(frontLeft, LOW);
          endTimeFrontLeftLow = micros() + frontLeftSpeed;
        }
        frontLeftL = true;
      }
    }

    if (initBackRight) {   //Back Right
      endTimeBackRightHigh = micros() + backRightSpeed;
      digitalWrite(backRight, HIGH);
      endTimeBackRightLow = 4294967295;
      initBackRight = false;
    }
    else {
      unsigned long t = micros();
      if (t > endTimeBackRightLow) {
        motorsRunning--;
      }
      else if (t > endTimeBackRightHigh) {
        if (!backRightL) {
          digitalWrite(backRight, LOW);
          endTimeBackRightLow = micros() + backRightSpeed;
        }
        backRightL = true;
      }
    }

    if (initFrontRight) {   //Front Right
      endTimeFrontRightHigh = micros() + frontRightSpeed;
      digitalWrite(frontRight, HIGH);
      endTimeFrontRightLow = 4294967295;
      initFrontRight = false;
    }
    else {
      unsigned long t = micros();
      if (t > endTimeFrontRightLow) {
        motorsRunning--;
      }
      else if (t > endTimeFrontRightHigh) {
        if (!frontRightL) {
          digitalWrite(frontRight, LOW);
          endTimeFrontRightLow = micros() + frontRightSpeed;
        }
        frontRightL = true;
      }
    }
  }



  /*
    digitalWrite(backLeft, HIGH);           //Back left motor driver code
    delayMicroseconds(backLeftSpeed);
    digitalWrite(backLeft, LOW);
    delayMicroseconds(backLeftSpeed);

    digitalWrite(backRight, HIGH);          //Back right motor driver code
    delayMicroseconds(backRightSpeed);
    digitalWrite(backRight, LOW);
    delayMicroseconds(backRightSpeed);

    digitalWrite(frontLeft, HIGH);          //Front left motor driver code
    delayMicroseconds(frontLeftSpeed);
    digitalWrite(frontLeft, LOW);
    delayMicroseconds(frontLeftSpeed);

    digitalWrite(frontRight, HIGH);         //Front right motor driver code
    delayMicroseconds(frontRightSpeed);
    digitalWrite(frontRight, LOW);
    delayMicroseconds(frontRightSpeed);
  */
}


/*
  short readSpeed(){        //I2C signal combination because the transmitter sends 2 bytes
  byte x1 = Wire.read();
  byte x2 = Wire.read();
  short x = x2<<8 | x1;
  return x;
  }

  void receiveEvent(int howMany) {    //Sets the I2C values to the joystick values that can be used by the motors when I2C data is recieved
  joyX = readSpeed();
  joyY = readSpeed();
  }
*/
