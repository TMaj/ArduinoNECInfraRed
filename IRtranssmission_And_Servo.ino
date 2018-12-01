
#define SERVO_PORT PB1 //9
#define ENGINE_PORT_1 PD7 //7
#define ENGINE_PORT_2 PB0 //8

#define IR_PORT PB4  //12
#define DIODE_PORT PB5 //13

//IR TRANSMISSION DEFINITIONS
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

//WHEELS 
#define FORWARD 1
#define BACKWARD 0
#define STOP -1

#define STRAIGHT 1
#define RIGHT 0
#define LEFT 2

//GLOBAL VARIABLES
// SERVO & ENGINE VARIABLES
int servo_counter  = 0;
int servo_counter_limit = 0;
int servo_position = STRAIGHT;
int engine_state = STOP;
int period = 1000;
//IR TRANSMISSION VARIABLES
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
int last_engine_state = STOP;

int setup_timer1() {  
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;
    
    long unsigned int f = 50;
    long unsigned int ftimes = 1000;
    long unsigned int prescaler = 8;
    long unsigned int cmr = F_CPU / f / ftimes / prescaler - 1;

    Serial.print("Compare match register = ");
    Serial.println(cmr);
    
    OCR1A  |= cmr;
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS11);   // set prescaler to 8
    TIFR1  |= (1 << OCF1A);  // clear any pending interrupt
    TIMSK1 |= (1 << OCIE1A); // enable the output compare interrupt
}

void setup_timer2(){
  TCCR2A = _BV(WGM21);  //set CTC mode
  TCCR2B = _BV(CS21);   //set /8 prescaler
  TCNT2  = 0;   //initialize counter value to 0
  long unsigned int compareMatchRegister = 100; //interrupts per every 50us  
  OCR2A = compareMatchRegister;  
  TIMSK2 |= (1 << OCIE2A);// enable timer compare interrupt
}

void ports_setup(){
   DDRB |= 0b00100011; // set 13,8,9 as output, rest as input
   DDRD |= 0b10000000; // set 13,8,9 as output, rest as input
}

void timer1_servo_handler() {
    servo_counter++;

    
    if(servo_counter < servo_counter_limit) {
        PORTB |= (1 << 1);   //Set PB1 high  
    } else {
        PORTB &= ~(1 << 1);   //Set PB1 low
    }
    
    if(servo_counter >= period - 1) {
       servo_counter = 0;
       //Serial.println(servo_position);
    }
}

void timer2_ir_handler(){

pin_val = (PINB & _BV(IR_PORT));
 
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

long decodeTable(){
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

void clearTable(){
  for(int i=0;i<80;i++)
  {
    table[i]=0;
  }
}

void printTable(){
  for(int i=0;i<80;i++)
  {
    Serial.print(table[i]);
    Serial.print(" ");
  }
  Serial.println();
}


void setWheelsState(int state)
{  
  switch (state)
  {  
   case 0xFF18E7:
   {
    servo_position = STRAIGHT;
    engine_state = FORWARD;
    Serial.println(engine_state);
    break; 
   }
   case 0xFF10EF:
   {
    servo_position = LEFT;
    engine_state = FORWARD;
    break; 
   }
   case 0xFF5AA5:
   {
    servo_position = RIGHT;
    engine_state = FORWARD;
    break; 
   }
   case 0xFF42BD:
   {
    servo_position = LEFT;
    engine_state = BACKWARD;
    break; 
   }
   case 0xFF4AB5:
   {
    servo_position = STRAIGHT;
    engine_state = BACKWARD;
    break; 
   }
   case 0xFF52AD:
   {
    servo_position = RIGHT;
    engine_state = BACKWARD;
    break; 
   }
   case 0xFFFFFFFF:
   {
    engine_state = last_engine_state;    
    break; 
   } 
  }
  last_engine_state = engine_state;
}

void setup() {  
  Serial.begin(9600);
  ports_setup();

  cli(); //stop interrupts  
  setup_timer1();
  setup_timer2();
  sei(); //allow interrupts
}

ISR(TIMER1_COMPA_vect) {
   timer1_servo_handler();
}

ISR(TIMER2_COMPA_vect){
   timer2_ir_handler();
}

int engine_iteration =0;

void loop() {
    switch(servo_position) {
        case RIGHT:
            servo_counter_limit = 0.079 * period; //0.050
            break; // pos -90 (to left) double(1   / 20)
        case STRAIGHT:
            servo_counter_limit = 0.090 * period;
            break; // pos 0 double(1.5 / 20)
        case LEFT:
            servo_counter_limit = 0.100 * period; // 0.100
            break; // pos 90 (to right) double(2   / 20)
    }
    
   
    Serial.println(engine_state);
    switch(engine_state) {
        case FORWARD:
            PORTD &= ~(1 << 7);   //Set PD7 low
            PORTB |= (1 << 0);   //Set PB0 high
        break;
        case BACKWARD:
            PORTD |= (1 << 7);   //Set PD7 high
            PORTB &= ~(1 << 0);   //Set PB0 low
        break;
        default:
            PORTD &= ~(1 << 7);   //Set PD7 low
            PORTB &= ~(1 << 0);   //Set PB0 low
          
    }

  if(engine_iteration < 52){
    engine_iteration++;
  }  
  
 if(transmission_ended)
  {
     //printTable();
     //Serial.println(decodeTable(), HEX);
     setWheelsState(decodeTable());
     clearTable();
     transmission_ended = false;
     engine_iteration = 0;
  }
  else
  {
    if (engine_iteration > 50)
     engine_state = STOP;
  }
  
}
