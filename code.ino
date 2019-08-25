#define F_CPU 12000000 
#include<avr\io.h>
#include<math.h>
int rightmotspeed,leftmotspeed; 


void I2C_Init()
{
    TWSR=0x00; //no pre scaling 
    TWBR=0x70; //SCL frequency is 50K for 12Mhz
    TWCR=0x04; //enabling i2c 
}

void I2C_Start()
{
    TWCR = ((1<<TWINT) | (1<<TWSTA) | (1<<TWEN));
    while (!(TWCR & (1<<TWINT)));                       //------start condition for i2c
}

void I2C_Stop(void)
{
    TWCR = ((1<< TWINT) | (1<<TWEN) | (1<<TWSTO));      //------stop condition for i2c 

}

void I2C_Write(uint8_t i2cData)
{
    TWDR = i2cData ;
    TWCR = ((1<< TWINT) | (1<<TWEN));                   //------function to write to MPU6050 of any other i2c device 
    while (!(TWCR & (1 <<TWINT)));
}

uint8_t I2C_Read(uint8_t ackOption)
{
    TWCR = ((1<< TWINT) | (1<<TWEN) | (ackOption<<TWEA));
    while (!(TWCR & (1<<TWINT)));                       //------to read from MPU6050 or any other i2c device 
    return TWDR;

}
int main()
{ 
  int a,b;
int x,y,z; //raw values 
int az,ay,ax;  //angle values 
DDRA=0xFF;  //in1,in2,in3,in4 of motor driver 
while(1)
{
 /*
  * *******************
  * please refer to the MPU6050's datasheet for more info, page 33 onwards for more information, 
  * also refer to the register map given on invensense.com
  * *******************
  */
I2C_Init();
I2C_Start();
I2C_Write(0b11010000); //mpu6050's address is 1101000 and 0 is "write bit" hence 0b11010000 is passed
I2C_Write(0b1101011); //register address  of POWER REGISTER where we have written SLEEP BIT to 0 to TURN ON  the mpu6050 
I2C_Write(0b0000000); //entire POWER REGISTER is written to zero  
I2C_Stop();
                      //by default the accelerometer hast the scalar of 2g and hence it dosent need to be set again
                      //--------------------------------------

I2C_Start();              
I2C_Write(0b11010000);  //MPU's address is passed   
I2C_Write(0b111100);    //MPU's register which is to be is passed , in this case it is X direction accelerometer(8-15 bit) out of 16 bits of acc) 

I2C_Start();                    //XACC(8-15)
I2C_Write(0b11010001);


I2C_Read(0);
I2C_Stop();

a=TWDR;

                    //---------------------------------------

I2C_Start();
I2C_Write(0b11010000); ////MPU's address is passed   
I2C_Write(0b111011);  //MPU's register which is to be is passed , in this case it is X direction accelerometer(8-15 bit) out of 16 bits of acc)

I2C_Start();
I2C_Write(0b11010001);  // Read bit(1) is passed after the address hence 0b11010001               
                                                              //XACC(0-7)

I2C_Read(0);
I2C_Stop();

b=TWDR;

                      //------------------------------------

b=(b<<8)|(a); //net 16 Bit data is stored in x variable which is 'signed' (you will see why in a minute)
x=b;


                       //-----------------------------------
                        

                      //--------------------------------------

I2C_Start();              
I2C_Write(0b11010000);    
I2C_Write(0b111110);     

I2C_Start();                    //YACC(8-15)
I2C_Write(0b11010001);


I2C_Read(0);
I2C_Stop();

a=TWDR;                         

                    //---------------------------------------

I2C_Start();
I2C_Write(0b11010000);
I2C_Write(0b111101);

I2C_Start();
I2C_Write(0b11010001);                  //YACC(0-7)


I2C_Read(0);
I2C_Stop();

b=TWDR;

                      //------------------------------------

b=(b<<8)|(a);                             
y=b;                  //raw 16 bit Y acc. data 


                       //-----------------------------------


                      //--------------------------------------

I2C_Start();              
I2C_Write(0b11010000);    
I2C_Write(0b1000000);     

I2C_Start();                    //ZACC(8-15)
I2C_Write(0b11010001);


I2C_Read(0);
I2C_Stop();

a=TWDR;

                    //---------------------------------------

I2C_Start();
I2C_Write(0b11010000);
I2C_Write(0b111111);

I2C_Start();
I2C_Write(0b11010001);                  //ZACC(0-7)


I2C_Read(0);
I2C_Stop();

b=TWDR;

                      //------------------------------------

b=(b<<8)|(a);
z=b;                  //raw 16 bit z acc. data 


                       //-----------------------------------


           
                       ax=57.3*atan2(-y,-z)+180; //raw degree values
                       ay=57.3*atan2(-x,-z)+180; //raw degree values



            

            if(ax>180)
            {
              ax=(ax-360);
            }
            if(ay>180)
            {
              ay=(ay-360);
            }
                      /*
                       * now the data we recive is as follows 
                       * when the MPU6050 is tilted forward the value of ax recived is in +ve and increases as we incline it more 
                       * it is -ve if we tilt it backward 
                       * tilt it right ways it is +ve 
                       * tilt it left and it is -ve 
                       * all this is due to the signed int that we introduced for the storage of the 16 acc bits 
                       * what it does is that when the accelerometer is moved in one direction say  +ve x its value reaches +64(int) starting from 32  
                       * moved in other direction say  -ve x its value reaches 32(int) starting from 0 hence it gets into +/- 32 when done with signed int 
                       * please appreciate my effort XD
                       * explore by printing values of x,y,z,ax,ay,az you will know 
                       */
            TCCR2A |=((1<<WGM20)|(1<<COM2A1)|(1<<COM2B1)); // starting pwm for motor speed control 
            TCCR2B |=(1<<CS22);
            DDRH |=(1<<DDH6); //pwm right pin 
            DDRB |=(1<<DDB4); //pwm left pin
                   /*    
                    *     below is the basic motor speed contol according to the values in ax and ay 
                    */
            if(ax>30)
            {             

              
              if(ay>30)       //turning right 
                  {                 rightmotspeed=3*ax-2*ay;
                                    leftmotspeed=3*ax;
                                    if(leftmotspeed>250)
                                        leftmotspeed=250;       

                                   OCR2A=rightmotspeed;
                                   OCR2B=leftmotspeed;
                          PORTA=0x35;
                          
                  }
              else if(ay<-30)  //turning left 
                  {                 rightmotspeed=3*ax;
                                    leftmotspeed=3*ax+2*ay;
                                    if(rightmotspeed>250)
                                        rightmotspeed=250;

                                    OCR2A=rightmotspeed;
                                    OCR2B=leftmotspeed;
                          PORTA=0x35;            
                  }
              else if((ay<30) |(ay>-30))      //going straight
                          {
                                    rightmotspeed=3*ax;
                                    leftmotspeed=3*ax;
                                    OCR2A=rightmotspeed;
                                    OCR2B=leftmotspeed;
                              PORTA=0x35;
                                    
                          }
            }

            
            else if(ax<-30)
            {             

              
              if(ay>30)             //back right turn 
                  {                 rightmotspeed=-3*ax-2*ay;
                                    leftmotspeed=-3*ax;
                                    if(leftmotspeed>250)
                                        leftmotspeed=250;

                                   OCR2A=rightmotspeed;
                                   OCR2B=leftmotspeed;
                          PORTA=0x2B;
                          
                  }
              else if(ay<-30)       //back left turn  
                  {                 rightmotspeed=-3*ax;
                                    leftmotspeed=-3*ax+2*ay;
                                    if(rightmotspeed>250)
                                        rightmotspeed=250;
                                    OCR2A=rightmotspeed;
                                    OCR2B=leftmotspeed;
                          PORTA=0x2B;           
                  }
              else if(ay<30 |ay>-30) //back 
                          {
                                    rightmotspeed=-3*ax;
                                    leftmotspeed=-3*ax;
                                    OCR2A=rightmotspeed;
                                    OCR2B=leftmotspeed;
                          PORTA=0x2B;
                                   
                                    
                          }
            }

            else if((ay<-30) && (ax<30) && (ax>-30) )
            {

            PORTA=0x04;
                        PORTH =(1<<PH6); 
            }
            else if((ay>30) && (ax<30) && (ax>-30))
            {

            PORTA=0x10;
                        PORTB =(1<<PB4);  
            }


            else 
            PORTA=0;
         

}

}
