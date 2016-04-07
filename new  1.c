//Writer:SunChao
//Modified:Lijiangtao
//Date:2014/08/14

//Date：2014/09/21
//new version exchange map
//TXN_L to P-H
//TXP_H to N-H
//TXP_L to N-L
//TXN_H to P-L

#define exchange_to_new 1
#define CLK_ADJ 110 //default 117

//default 5
//reader transmit rate unit KHz
//10KHz 5KHz 2KHz 1KHz available
#define RX_RATE 5

//default 48
//tag tranmit rate 
//speed: 6/TX_PERIOD uint Kbps  ( TX_PERIOD/12 (ms))
#define TX_PERIOD 48

#include "io430.h"

//receive state
#define REV_IDLE 0
#define REV_SOF 1
#define REV_FIRST_BIT 2
#define REV_DATA 3

unsigned char REV_STATE=REV_IDLE;
unsigned char REV_PTR; 
//unsigned char REVD_FRAME=0;

#define RES_UID 0x934726
#define CMD_SEARCH 0xa5


//3 data+ 1 CRC
#define TX_BYTES_LENGTH 4
#define TX_LENGTH (8*TX_BYTES_LENGTH)
unsigned char TxBuf[TX_BYTES_LENGTH+1];
unsigned char TX_BIT_PTR;//point to bits 0-7
unsigned char TX_BYTE_PTR;//point to bytes


#define RX_BYTES_LENGTH 2
#define RX_LENGTH (8*RX_BYTES_LENGTH)
#define RX_BUF_SIZE RX_LENGTH+5
unsigned char RxBuf[RX_BUF_SIZE];


//Control Tx and Rx port init
#define RX_Enable() P1OUT&=~BIT2
#define RX_Disable() P1OUT|=BIT2
#define Tx_Enable() P1OUT|=BIT2
#define Tx_Disable() P1OUT&=~BIT2

#ifdef exchange_to_new

  #define TXP_L() P1OUT&=~BIT7
  #define TXP_H() P1OUT|=BIT7

  #define TXN_L() P1OUT|=BIT6
  #define TXN_H() P1OUT&=~BIT6
#else
  #define TXP_L() P1OUT&=~BIT6
  #define TXP_H() P1OUT|=BIT6

  #define TXN_L() P1OUT&=~BIT7
  #define TXN_H() P1OUT|=BIT7
#endif




#define Rx_Int_Enable()  P1IE|=BIT5
#define Rx_Int_Disable()  P1IE&=~BIT5

void Port_init(void)
{
  //P1.2-control p1.5-in p1.6-txp p1.7-txn
  //p1.2.6.7-out
  P1DIR|=BIT2|BIT6|BIT7|BIT4; 
  TXP_L();
  TXN_H();
  P1OUT|=BIT4;
  RX_Enable();
  //p1.5 input interrupt
  P1IES |= BIT5;                            // P1.5 Hi to lo edge
  P1IFG &= ~BIT5;                           // P1.5 IFG cleared
  Rx_Int_Enable();
}

//MCLK-1MHz ACLK-12KHz
void SysClkCfg(void)
{
   BCSCTL3 |= LFXT1S_2;                      // Select VLO as low freq clock     
  /* End Initialization Code */ 
  //1Mhz
  if (CALBC1_1MHZ==0xFF)                     // If calibration constant erased
  {											
    while(1);                               // do not load, trap CPU!!	
  }
  DCOCTL = 0;                               // Select lowest DCOx and MODx settings
  BCSCTL1 = CALBC1_1MHZ;                    // Set range
  DCOCTL = CALDCO_1MHZ;                     // Set DCO step + modulation */
}
//TimerA init 12KHz

#define TimerA_Int_Enable() TA0CCTL0 |= CCIE
#define TimerA_Int_Disable() TA0CCTL0 &= ~CCIE
#define TimerA_Start() TA0CTL= TASSEL_1 | MC_2 // ACLK, contmode
#define TimerA_Stop() TA0CTL= TASSEL_1 //ACLK

#ifdef exchange_to_new
#define SetPeriod(x) TA0CCR0 = x*CLK_ADJ/100
#else

#define SetPeriod(x) TA0CCR0 = x
#endif


#define SetTimerAValue(x) TA0R = x
#define GetTimerAValue() TA0R

void TimerA_Init(void)
{
  TimerA_Stop(); 
  SetPeriod(200);
  SetTimerAValue(0);
  TimerA_Int_Disable();
}

unsigned char CRC8_CHECK(unsigned char *ucPtr, unsigned char ucLen);
unsigned char CRC8_GEN(unsigned char *data,unsigned char datalen); 

//not correct, just for debug
void delayms(volatile unsigned short ms)
{
   volatile unsigned short int i;
   for(;ms;ms--)
   {
     for(i=100;i;i--)
     {
     }
   }
 }

void PreFillTxBuf(void)
{
  TxBuf[0]=RES_UID&0xff;
  TxBuf[1]=(RES_UID>>8)&0xff;
  TxBuf[2]=(RES_UID>>16)&0xff;
  TxBuf[3]=CRC8_GEN(TxBuf,3);
 }

unsigned char t1,t2,t3,t4,t5,t6;
int main( void )
{
  //Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
  
  SysClkCfg();
  
    //time para
    //RX_RATE 5K 800us 400us 200us  0.1ms-0.0833ms
    //10KHz 5KHz 2KHz 1KHz available(2.5 1.25)
    //#define RX_RATE 5
    switch(RX_RATE)
    {
      case 10:
      t1=4;
      t2=8;
      
      t3=0;
      t4=2;
      
      t5=2;
      t6=4;
      break;
      
      case 5:
      
      t1=6;
      t2=11;
      t3=0;
      t4=3;
      t5=3;
      t6=6;
      break;
      
      case 2:
      
      t1=12;
      t2=21;
      
      t3=2;
      t4=6;
      
      t5=6;
      t6=12;    
      
      break;
      
     case 1:
      
      t1=25;
      t2=45;
      
      t3=5;
      t4=12;
      
      t5=12;
      t6=25;         
      
      break;
      default:
      t1=6;
      t2=11;
      t3=0;
      t4=3;
      t5=3;
      t6=6;
      
    }
  
  Port_init();
  
  TimerA_Init();
  
  PreFillTxBuf();
  
  __bis_SR_register(LPM3_bits + GIE);     //Enter LPM3 and Global Int
   
  
  while(1)
  {
    
    
    
  
  }
  

}


 unsigned short int TimerAValue;
//Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
 
  unsigned char i,j;
  
  TimerAValue=GetTimerAValue();
  SetTimerAValue(0);
  TimerA_Start();
  
  switch(REV_STATE)
  {
    case REV_IDLE:
         
         REV_STATE=REV_SOF;
         
    break;   
    
    case REV_SOF: 
         
         if((TimerAValue>t1)&&(TimerAValue<t2))
         {
           REV_STATE=REV_FIRST_BIT;
         }
         else
         {
           REV_STATE=REV_SOF;
         }
    break;
    case REV_FIRST_BIT:
         
         if((TimerAValue>t3)&&(TimerAValue<=t4))
         {
           REV_STATE= REV_DATA;
           REV_PTR=0;
         }
         else
         {
           REV_STATE=REV_SOF;
         }
    break;
    case REV_DATA:
         
         if((TimerAValue>t3)&&(TimerAValue<=t4))
         {
            REV_STATE= REV_DATA;
            RxBuf[REV_PTR++]=0/*TimerAValue*/;
         }
         else if((TimerAValue>t5)&&(TimerAValue<=t6))
         {
            REV_STATE= REV_DATA;
            RxBuf[REV_PTR++]=1/*TimerAValue*/;
         }else
         {
            REV_STATE=REV_SOF;
         }
         if(REV_PTR>=RX_LENGTH)
         {
            //REVD_FRAME=1;//revd
            //process
            for(i=0,j=0;i<RX_BYTES_LENGTH;i++,j+=8)
            {
              RxBuf[i]=RxBuf[j]|(RxBuf[j+1]<<1)+(RxBuf[j+2]<<2)
              +(RxBuf[j+3]<<3)+(RxBuf[j+4]<<4)+(RxBuf[j+5]<<5) 
              +(RxBuf[j+6]<<6)+(RxBuf[j+7]<<7);
            }
            //Check CRC8
            if(CRC8_CHECK(RxBuf,2))
            {
                //error
                //reset timer
                SetTimerAValue(0);
                TimerA_Stop();
                
            }else
            { 
              //Right
              
              //Tx_Enable();
              //delayms(50);
              //TXN_L();
              //TXP_H();
              //delayms(200);
              //TXP_L();
              //TXN_H();
              //RX_Enable();
                if(RxBuf[0]==CMD_SEARCH)
                {
                    //disable rx 
                    Rx_Int_Disable();
                    //enable tx
                    TXP_L();
                    TXN_L();
                    Tx_Enable();//enable tx power
                    TX_BYTE_PTR=0;
                    TX_BIT_PTR=0;
                    //timer
                    TimerA_Stop();
                    SetTimerAValue(0);
                    SetPeriod(TX_PERIOD);//intrrupt every 4ms 12KHz-48period
                    TimerA_Int_Enable();
                    TimerA_Start();
                }else
                {
                    //error
                    //reset timer
                    SetTimerAValue(0);
                    TimerA_Stop();
                  
                }
             
            }
            
            //reset statemachine
            REV_STATE=REV_IDLE;
            
            break;
         }
      
    break;
    default: break;
  }
  
  P1IFG &= ~BIT5;                       // P1.5 IFG cleared
}

//Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
  static unsigned char halfbit;
  SetTimerAValue(0);//reset to 0
   //TX_BYTE_PTR=0 indicate to transmit sof
   if(!TX_BYTE_PTR)
   {
      //transmit sof
      switch(TX_BIT_PTR)
      {
        case 0:
          TXN_L();
          TXP_H();
        break;
        case 1:
          TXN_L();
          TXP_H();
        break;
        case 2:
          TXP_L();
          TXN_H();
        break;
        case 3:
          TXN_L();
          TXP_H();         
        break;
        default:break;
      }
      if(TX_BIT_PTR>=3)
      {
        TX_BIT_PTR=0;
        TX_BYTE_PTR++;
        halfbit=0;
      }else
      {
        TX_BIT_PTR++;
      }
     
   }else
   {
      //transmit data
      if(!halfbit)
      {
        if(TX_BYTE_PTR>TX_BYTES_LENGTH)
        {
          //end for transmit
          //disable tx 
          TXP_L();
          TXN_H();
          Tx_Disable();//disable tx power
          //timer
          TimerA_Stop();
          SetTimerAValue(0);
          TimerA_Int_Disable();
          //enable rx
          Rx_Int_Enable();
          return;
        }
        if((TxBuf[TX_BYTE_PTR-1])&(1<<TX_BIT_PTR))
        {
          //0
          TXN_L();
          TXP_H();
        }else
        {
          //1
          TXP_L();
          TXN_H();
        }


     }else
     {
        if((TxBuf[TX_BYTE_PTR-1])&(1<<TX_BIT_PTR))
        {
          //0
          TXP_L();
          TXN_H();
        }else
        {
          //1
          TXN_L();
          TXP_H();
        }
               
        if(TX_BIT_PTR>=7)
        {
          TX_BIT_PTR=0;
          TX_BYTE_PTR++;
        }else
        {
          TX_BIT_PTR++;
        }
        
       
     }
     halfbit=~halfbit;
     
   }
   
   TA0CCTL0&=~CCIFG;//Clear flags
}

// CRC8
// X^8   +   X^2   +   X^1   +   1   
unsigned char const CRC8_TAB[256]   =   
{     
 0x00,0x07,0x0E,0x09,0x1C,0x1B,0x12,0x15,0x38,0x3F,0x36,0x31,0x24,0x23,0x2A,0x2D,   
 0x70,0x77,0x7E,0x79,0x6C,0x6B,0x62,0x65,0x48,0x4F,0x46,0x41,0x54,0x53,0x5A,0x5D,   
 0xE0,0xE7,0xEE,0xE9,0xFC,0xFB,0xF2,0xF5,0xD8,0xDF,0xD6,0xD1,0xC4,0xC3,0xCA,0xCD,   
 0x90,0x97,0x9E,0x99,0x8C,0x8B,0x82,0x85,0xA8,0xAF,0xA6,0xA1,0xB4,0xB3,0xBA,0xBD,   
 0xC7,0xC0,0xC9,0xCE,0xDB,0xDC,0xD5,0xD2,0xFF,0xF8,0xF1,0xF6,0xE3,0xE4,0xED,0xEA,   
 0xB7,0xB0,0xB9,0xBE,0xAB,0xAC,0xA5,0xA2,0x8F,0x88,0x81,0x86,0x93,0x94,0x9D,0x9A,   
 0x27,0x20,0x29,0x2E,0x3B,0x3C,0x35,0x32,0x1F,0x18,0x11,0x16,0x03,0x04,0x0D,0x0A,   
 0x57,0x50,0x59,0x5E,0x4B,0x4C,0x45,0x42,0x6F,0x68,0x61,0x66,0x73,0x74,0x7D,0x7A,   
 0x89,0x8E,0x87,0x80,0x95,0x92,0x9B,0x9C,0xB1,0xB6,0xBF,0xB8,0xAD,0xAA,0xA3,0xA4,   
 0xF9,0xFE,0xF7,0xF0,0xE5,0xE2,0xEB,0xEC,0xC1,0xC6,0xCF,0xC8,0xDD,0xDA,0xD3,0xD4,   
 0x69,0x6E,0x67,0x60,0x75,0x72,0x7B,0x7C,0x51,0x56,0x5F,0x58,0x4D,0x4A,0x43,0x44,   
 0x19,0x1E,0x17,0x10,0x05,0x02,0x0B,0x0C,0x21,0x26,0x2F,0x28,0x3D,0x3A,0x33,0x34,   
 0x4E,0x49,0x40,0x47,0x52,0x55,0x5C,0x5B,0x76,0x71,0x78,0x7F,0x6A,0x6D,0x64,0x63,   
 0x3E,0x39,0x30,0x37,0x22,0x25,0x2C,0x2B,0x06,0x01,0x08,0x0F,0x1A,0x1D,0x14,0x13,   
 0xAE,0xA9,0xA0,0xA7,0xB2,0xB5,0xBC,0xBB,0x96,0x91,0x98,0x9F,0x8A,0x8D,0x84,0x83,   
 0xDE,0xD9,0xD0,0xD7,0xC2,0xC5,0xCC,0xCB,0xE6,0xE1,0xE8,0xEF,0xFA,0xFD,0xF4,0xF3   
};   
//CRC8
unsigned char CRC8_CHECK(unsigned char *ucPtr, unsigned char ucLen)   
{   
			 unsigned char ucIndex;                  
			 unsigned char ucCRC8 = 0;                
       
			 while(ucLen --)
			 {   
							 ucIndex = ucCRC8 ^ (*ucPtr++);   
							 ucCRC8 = CRC8_TAB[ucIndex];   
			 }   
 
			 
			 return ucCRC8;   
} 
//CRC8
unsigned char CRC8_GEN(unsigned char *data,unsigned char datalen) 
{ 
  unsigned char CRC=0; 
  unsigned char genPoly = 0x07;  
	unsigned char i,j;
  for (i=0; i<datalen; i++) 
  { 
    CRC ^= data[i];
    for(j = 0; j<8; j++) 
    {  
        if(CRC & 0x80 )  
            CRC = (CRC << 1) ^ genPoly; 
        else  
            CRC <<= 1; 
    } 
  }  
  return CRC; 
}  
