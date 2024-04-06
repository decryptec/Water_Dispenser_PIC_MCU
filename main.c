#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <math.h>
#include <p18f4620.h>
#include <usart.h>
#include <string.h>

#pragma config OSC = INTIO67
#pragma config BOREN =OFF
#pragma config WDT=OFF
#pragma config LVP=OFF
#pragma config CCP2MX = PORTBE
//Prototyping
void Do_Init(); //void interrupt high_priority ISR(void);
void Select_ADC_Channel(char channel);
unsigned int get_full_ADC(void); //Get voltage steps
void Wait_Half_Second();
void Wait_One_Second();
void Wait_N_Seconds(char);
void INT0_ISR(void);
void INT1_ISR(void);
void INT2_ISR(void);
void interrupt high_priority chkisr(void);
//Var
int ISR0 = 0;
int ISR1 = 0;
int ISR2 = 0;
//Define bits
#define Full PORTCbits.RC0 
#define PUMP PORTDbits.RD0
#define Empty PORTEbits.RE0
#define Mid PORTCbits.RC1
//Start Functions Here
void init_UART()
{
 OpenUSART (USART_TX_INT_OFF & USART_RX_INT_OFF &
USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX &
USART_BRGH_HIGH, 25);
 OSCCON = 0x60;
}
void putch (char c)
{
 while (!TRMT);
 TXREG = c;
} 
void Select_ADC_Channel(char channel)     //Select Channel
{
    ADCON0 = channel * 4 + 1;             //Shifts bit by 2 to the left, keeps bit 0 on
}                                           
unsigned int get_full_ADC(void)           //Get step data
{    
    int result; 
    ADCON0bits.GO=1;                      //Start Conversion   
    while(ADCON0bits.DONE==1);            //Wait for conversion to be completed    
    result = (ADRESH * 0x100) + ADRESL;   //Combine result of U/L byte   
    return result;
}
void Do_Init()
{                                             
    TRISA = 0x0f;                         //Only use pin1 (AN0) as input, the rest aren't used
    TRISB = 0x07;                         //Interrupts INT0,INT1,INT2, 
    TRISC = 0x00;                         //All outputs, RC1 = 1 powers pump
    TRISD = 0x00;                         //All outputs, Goes to RGB LED, Not enough water RGB
    TRISE = 0x00;                         //Not used
    //ADCON0 is defined by Select_ADC_Channel Function
    ADCON1=0x0B;                          //Select pins AN0 through AN2 as analog signal, VDD-VSS as Ref.
    ADCON2=0xA9;                          //Right justify, 12 TAD, FOSC/8

    RBPU = 0x00; 
    //Flags
    INTCONbits.INT0IF = 0;                //Clear INT0IF
    INTCON3bits.INT1IF = 0;               //Clear INT1IF
    INTCON3bits.INT2IF =0;                //Clear INT2IF
    //Triggers with the use of buttons
    INTCON2bits.INTEDG0=1;                //INT0 EDGE falling
    INTCON2bits.INTEDG1=1;                //INT1 EDGE falling
    INTCON2bits.INTEDG2=1;                //INT2 EDGE rising
    //Enable
    INTCONbits.INT0IE =1;                 //Set INT0 IE
    INTCON3bits.INT1IE=1;                 //Set INT1 IE
    INTCON3bits.INT2IE=1;                 //Set INT2 IE
    //Globally enable Interrupts
    INTCONbits.GIE=1;
    
    
}
void Wait_One_Second()
{    
    Wait_Half_Second();
}
void Wait_Half_Second()                         // WAIT HALF A SECOND 
{
    T0CON = 0x03;                               // Timer 0, 16-bit mode, prescaler 1:16
    TMR0L = 0xDB;                               // set the lower byte of TMR
    TMR0H = 0x0B;                               // set the upper byte of TMR
    INTCONbits.TMR0IF = 0;                      // clear the Timer 0 flag
    T0CONbits.TMR0ON = 1;                       // Turn on the Timer 0
    while (INTCONbits.TMR0IF == 0);             // wait for the Timer Flag to be 1 for done
    T0CONbits.TMR0ON = 0;                       // turn off the Timer 0
}
void Wait_N_Seconds (char seconds)              // WAIT N AMOUNT OF SECONDS BY CALLING THE WAIT ONE SECOND FUNCTION N TIMES
{
    char I;                                     //intialize I varible
    for (I = seconds; I>0; I--)                //set I = seconds and decrease value till 0
    {
        Wait_One_Second();                      // calls Wait_One_Second for x number of times
    }
}
void interrupt high_priority chkisr()
{
    if (INTCONbits.INT0IF == 1) INT0_ISR(); //check if INT0 has occurred                                            
    //if (INTCON3bits.INT1IF == 1) INT1_ISR();//check if INT1 has occurred
    //if (INTCON3bits.INT2IF == 1) INT2_ISR();//check if INT2 has occurred
}
void INT0_ISR(void) 
{ 
 INTCONbits.INT0IF=0; //Clear the interrupt flag
 
 if(Empty==0)
 {
 ISR0 = 1;
 }
} 
void INT1_ISR(void) 
{ 
 INTCON3bits.INT1IF=0; //Clear the interrupt flag
 ISR1 = 1;
} 
void INT2_ISR(void) 
{ 
 INTCON3bits.INT2IF=0; //Clear the interrupt flag
 ISR2 = 1;
} 
void main()
{
    init_UART();
    Do_Init();
    while(1) //Loop Forever
    {        
        
        Wait_N_Seconds(1);
        Select_ADC_Channel(0);
        unsigned int num_step = get_full_ADC(); // GET NUMSTEP FROM ADC CHANNEL 
        float SensorVoltage = (num_step*4.0); // multiply by Ref value (about 4.096)
        SensorVoltage = SensorVoltage/1000;
        printf("\nSteps1 = %d \r\n", num_step);
        printf("Voltage1 = %f \r\n\n", SensorVoltage);
        
        if(SensorVoltage>3.5)
        {
            Full=1;
            Mid=0;
            Empty=0;
        }
        else
        {
            Full=0;
        }
           
        Wait_N_Seconds(1);
        Select_ADC_Channel(2);
        num_step = get_full_ADC(); // GET NUMSTEP FROM ADC CHANNEL 
        SensorVoltage = (num_step*4.0); // multiply by Ref value (about 4.096)
        SensorVoltage = SensorVoltage/1000;
        printf("\nSteps2 = %d \r\n", num_step);
        printf("Voltage2 = %f \r\n\n", SensorVoltage);
        
        if(SensorVoltage>3.5 && Full ==0)
        {
            Full=0;
            Mid=1;
            Empty=0;
        }
        else
        {
            Mid=0;
        }
        
        if(Full==0 && Mid == 0)
        {
            Full=0;
            Empty=1;
            Mid=0;
        }
        
        if(Empty==1)
        {
            Empty=0;
            Wait_N_Seconds(1);
            Empty=1;
        }
        if (ISR0 == 1) 
        { 
         INTCONbits.INT0IF=0; //Clear the interrupt flag
         printf("\r\nOne Liter Request\n");         
         PORTD = 0x01;
         Wait_N_Seconds(5);
         PORTD = 0x00;
         ISR0 = 0;
         printf("\nDone");
        } 
        /*
        if (ISR1 == 1)
        { 
         INTCON3bits.INT1IF=0; //Clear the interrupt flag
         printf("\r\nTwo Liter Request\n");
         PORTD = 0x01;
         Wait_N_Seconds(2);
         PORTD = 0x00;
         ISR1 = 0;
        } 
        if (ISR2 == 1) 
        { 
         INTCON3bits.INT2IF=0; //Clear the interrupt flag
         printf("\r\nThree Liter Request\n");
         PORTD = 0x01;
         Wait_N_Seconds(3);
         PORTD = 0x00;
         ISR2 = 0;
        } 
        */

    }
}
