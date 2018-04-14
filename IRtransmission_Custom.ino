#define SIGN   0
#define SPACE  1

#define MATCH_562(x) ( x == 10 || x == 11 || x == 12 || x==13)
#define MATCH_16875(x) ( x == 32 || x == 33 || x == 34)
#define MATCH_9000(x) (x== 178 || x ==179 || x == 180)
#define MATCH_4500(x) (x == 88 || x == 89 || x == 90)
#define MATCH_2500(x) (x ==43 || x == 44 || x ==45)

#define MATCH_LEAD(x1,x2) (MATCH_9000(x1) && MATCH_4500(x2)) 
#define MATCH_REPEAT(x1,x2,x3) (MATCH_9000(x1) && MATCH_2500(x2) && MATCH_562(x3))
#define MATCH_ZERO(x1,x2) (MATCH_562(x1) && MATCH_562(x2))
#define MATCH_ONE(x1,x2) (MATCH_562(x1) && MATCH_16875(x2))

#define FORWARD_LEFT 0
#define FORWARD 1
#define FORWARD_RIGHT 2
#define LEFT 3
#define RIGHT 4
#define BACK_LEFT 5
#define BACK 6
#define BACK_RIGHT 7

#define ON 1
#define OFF 0

int table[80];
int x = 0;
volatile unsigned long int ticks = 0;
volatile bool sign = false;
volatile bool space = false;
bool transmission_ended = false;
bool transmission_started = false;
unsigned volatile int spaces = 0;
volatile int it = 0;
bool pin_val;

int WHEELS_STATE;
int ENGINE_STATE;

void setup_timer2(){
  cli();//stop interrupts  
  TCCR2A = _BV(WGM21);  //set CTC mode
  TCCR2B = _BV(CS21);   //set /8 prescaler
  TCNT2  = 0;   //initialize counter value to 0
  long unsigned int compareMatchRegister = 100; //interrupts per every 50us  
  OCR2A = compareMatchRegister;  
  TIMSK2 |= (1 << OCIE2A);// enable timer compare interrupt
  sei();//allow interrupts
}

void setWheelsState(int state)
{
  switch (state)
  {
   case 0xFF30CF:
   {
    WHEELS_STATE = FORWARD_LEFT;
    ENGINE_STATE = ON;
    break; 
   }
   case 0xFF18E7:
   {
    WHEELS_STATE = FORWARD;
    ENGINE_STATE = ON;
    break; 
   }
   case 0xFF7A85:
   {
    WHEELS_STATE = FORWARD_RIGHT;
    ENGINE_STATE = ON;
    break; 
   }
   case 0xFF10EF:
   {
    WHEELS_STATE = LEFT;
    ENGINE_STATE = ON;
    break; 
   }
   case 0xFF5AA5:
   {
    WHEELS_STATE = RIGHT;
    ENGINE_STATE = ON;
    break; 
   }
   case 0xFF42BD:
   {
    WHEELS_STATE = BACK_LEFT;
    ENGINE_STATE = ON;
    break; 
   }
   case 0xFF4AB5:
   {
    WHEELS_STATE = BACK;
    ENGINE_STATE = ON;
    break; 
   }
   case 0xFF52AD:
   {
    WHEELS_STATE = BACK_RIGHT;
    ENGINE_STATE = ON;
    break; 
   }
   case 0xFFFFFFFF:
   {
    ENGINE_STATE = ON;
    break; 
   }      
  }
}

void setup() {  
  Serial.begin(9600);   
  setup_timer2();
  DDRB |= 0b00100000; // set 13 as output, rest as input
  pinMode(12,INPUT);
}

void loop() {
  if(transmission_ended)
  {
     //printTable();
     //Serial.println(decodeTable(), HEX);
     setWheelsState(decodeTable());
     Serial.println(WHEELS_STATE);
     clearTable();
     transmission_ended = false;
  }
}

ISR(TIMER2_COMPA_vect){ 

pin_val = (PINB & _BV(PB4));
 
if(pin_val == SIGN)
{
  PORTB |= (1 << 5);
}
else
{
  PORTB &= ~(1 << 5);
}

 if (!transmission_ended)
 {
    if(pin_val == SIGN)
    {
      if(space == true)
      {
        table[it++] = ticks;
        ticks = 0;  
      }
        transmission_started = true;
        ticks++;
        sign = true;
        space = false;
        spaces = 0;
    }
    else if (transmission_started)
    {
      if (spaces > 91)
      {
       transmission_ended = true;
       transmission_started = false;
       space = false; 
       sign = false;
       ticks = 0;
       it=0;
       spaces = 0;
       return;
      }

      if( sign == true )
      {
        table[it++] = ticks;
        ticks = 0;
      }

      sign = false;
      space = true;
      
      ticks++;
      spaces++;       
    }
 }
}

void printTable()
{
  for(int i=0;i<80;i++)
  {
    Serial.print(table[i]);
    Serial.print(" ");
  }
  Serial.println();
}

void clearTable()
{
  for(int i=0;i<80;i++)
  {
    table[i]=0;
  }
}

long decodeTable()
{
  int pos = 0; 

  if(MATCH_REPEAT(table[0], table[1], table[2]))
  {
    return -1;
  }
  
  if (!MATCH_LEAD(table[0], table[1]))
  {
    return -9;
  }

  long decoded_val = 0;
  
  for (int i = 2; i < 66 ; i+=2)
  {
    
    if(MATCH_ONE(table[i],table[i+1]))
    {
      decoded_val = (decoded_val << 1) | 1 ;     
    }
    else if (MATCH_ZERO(table[i],table[i+1]))
    {
      decoded_val = (decoded_val << 1) | 0 ;  
    }
    else
    {
      return -9;
    }
  }
  
  return decoded_val;
}
