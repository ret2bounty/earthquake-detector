#include <stdint.h>

//
// Hardware Addresses (from earthquake.repl)
//
#define UART_BASE   0x10000000
#define SENSOR_BASE 0x20000000
#define GPIO_BASE   0x30000000

//
// Sensor Register Offsets
//
#define SENSOR_X_AXIS   0x00
#define SENSOR_Y_AXIS   0x04
#define SENSOR_Z_AXIS   0x08
#define SENSOR_STATUS   0x0C
#define SENSOR_PEAK     0x10
#define SENSOR_COUNTER  0x14

//
// Status Register Bits
//
#define STATUS_DATA_READY    0x01
#define STATUS_QUAKE_ACTIVE  0x02
#define STATUS_THRESHOLD     0x04

//
// Alert Levels
//
#define LEVEL_NONE     0
#define LEVEL_MINOR    1
#define LEVEL_MODERATE 2
#define LEVEL_SEVERE   3
#define LEVEL_EXTREME  4

//
// Thresholds for Magnitude Classification
//
#define THRESHOLD_MINOR    30
#define THRESHOLD_MODERATE 60
#define THRESHOLD_SEVERE   100
#define THRESHOLD_EXTREME  150

//
// Sampling Configuration
//
#define BUFFER_SIZE      32
#define IDLE_DELAY       50000
#define EVENT_DELAY      10000
#define STATUS_INTERVAL  20

//
// Hardware Pointers
//
volatile uint32_t *Uart = (volatile uint32_t *)UART_BASE;
volatile uint32_t *SensorX = (volatile uint32_t *)(SENSOR_BASE + SENSOR_X_AXIS);
volatile uint32_t *SensorY = (volatile uint32_t *)(SENSOR_BASE + SENSOR_Y_AXIS);
volatile uint32_t *SensorZ = (volatile uint32_t *)(SENSOR_BASE + SENSOR_Z_AXIS);
volatile uint32_t *SensorStatus = (volatile uint32_t *)(SENSOR_BASE + SENSOR_STATUS);
volatile uint32_t *SensorPeak = (volatile uint32_t *)(SENSOR_BASE + SENSOR_PEAK);
volatile uint32_t *SensorCounter = (volatile uint32_t *)(SENSOR_BASE + SENSOR_COUNTER);

//
// Global State
//
uint32_t MagnitudeBuffer[BUFFER_SIZE];
uint32_t BufferIndex = 0;
uint32_t TotalEvents = 0;
uint32_t MaxRecordedMagnitude = 0;

//
// Types
//
typedef struct
{
    int32_t X;
    int32_t Y;
    int32_t Z;
    uint32_t Magnitude;
    uint32_t Status;
} accel_reading;

// ============ UART Functions ============

void PrintChar(char C)
{
    *Uart = C;
}

void PrintStr(const char *S)
{
    while (*S)
    {
        PrintChar(*S++);
    }
}

void PrintDec(int32_t Num)
{
    if (Num < 0)
    {
        PrintChar('-');
        Num = -Num;
    }
    
    if (Num == 0)
    {
        PrintChar('0');
        return;
    }
    
    char Buf[12];
    int I = 0;
    
    while (Num > 0)
    {
        Buf[I++] = '0' + (Num % 10);
        Num /= 10;
    }
    
    while (I > 0)
    {
        PrintChar(Buf[--I]);
    }
}

void PrintDecPadded(int32_t Num, int Width)
{
    // Calculate number of digits
    int32_t Temp = Num;
    int Digits = 0;
    int IsNegative = 0;
    
    if (Num < 0)
    {
        IsNegative = 1;
        Temp = -Num;
    }
    
    if (Temp == 0)
    {
        Digits = 1;
    }
    else
    {
        while (Temp > 0)
        {
            Digits++;
            Temp /= 10;
        }
    }
    
    // Add padding spaces
    int TotalWidth = Digits + IsNegative;
    for (int I = TotalWidth; I < Width; I++)
    {
        PrintChar(' ');
    }
    
    PrintDec(Num);
}

void PrintHex(uint32_t Num)
{
    const char Hex[] = "0123456789ABCDEF";
    PrintStr("0x");
    for (int I = 28; I >= 0; I -= 4)
    {
        PrintChar(Hex[(Num >> I) & 0xF]);
    }
}

void PrintNewline(void)
{
    PrintChar('\r');
    PrintChar('\n');
}

// ============ Math Functions ============

uint32_t IntegerSquareRoot(uint32_t N)
{
    if (N == 0)
    {
        return 0;
    }
    
    uint32_t X = N;
    uint32_t Y = (X + 1) / 2;
    
    while (Y < X)
    {
        X = Y;
        Y = (X + N / X) / 2;
    }
    
    return X;
}

int32_t AbsoluteValue(int32_t X)
{
    return (X < 0) ? -X : X;
}

// ============ Sensor Functions ============

void ReadAccelerometer(accel_reading *Reading)
{
    Reading->X = (int32_t)*SensorX;
    Reading->Y = (int32_t)*SensorY;
    Reading->Z = (int32_t)*SensorZ;
    Reading->Status = *SensorStatus;
    
    int32_t DeltaX = Reading->X;
    int32_t DeltaY = Reading->Y;
    int32_t DeltaZ = Reading->Z - 1000;
    
    uint32_t Sum = (uint32_t)(AbsoluteValue(DeltaX) * AbsoluteValue(DeltaX) + 
                              AbsoluteValue(DeltaY) * AbsoluteValue(DeltaY) + 
                              AbsoluteValue(DeltaZ) * AbsoluteValue(DeltaZ));
    Reading->Magnitude = IntegerSquareRoot(Sum);
}

void ResetPeak(void)
{
    *SensorPeak = 0;
}

uint32_t GetPeak(void)
{
    return *SensorPeak;
}

// ============ STA/LTA Detection Algorithm ============
// Short-Term Average / Long-Term Average ratio for earthquake detection
// This is a real seismological algorithm

uint32_t CalculateSTA(void)
{
    uint32_t Sum = 0;
    for (int I = 0; I < 4; I++)
    {
        int Index = (BufferIndex - 1 - I + BUFFER_SIZE) % BUFFER_SIZE;
        Sum += MagnitudeBuffer[Index];
    }
    return Sum / 4;
}

uint32_t CalculateLTA(void)
{
    uint32_t Sum = 0;
    for (int I = 0; I < BUFFER_SIZE; I++)
    {
        Sum += MagnitudeBuffer[I];
    }
    return Sum / BUFFER_SIZE;
}

void AddToBuffer(uint32_t Magnitude)
{
    MagnitudeBuffer[BufferIndex] = Magnitude;
    BufferIndex = (BufferIndex + 1) % BUFFER_SIZE;
}

// ============ Alert System ============

const char *GetLevelString(int Level)
{
    switch (Level)
    {
        case LEVEL_MINOR:    return "MINOR";
        case LEVEL_MODERATE: return "MODERATE";
        case LEVEL_SEVERE:   return "SEVERE";
        case LEVEL_EXTREME:  return "EXTREME";
        default:             return "NONE";
    }
}

int ClassifyMagnitude(uint32_t Magnitude)
{
    if (Magnitude >= THRESHOLD_EXTREME)
    {
        return LEVEL_EXTREME;
    }
    if (Magnitude >= THRESHOLD_SEVERE)
    {
        return LEVEL_SEVERE;
    }
    if (Magnitude >= THRESHOLD_MODERATE)
    {
        return LEVEL_MODERATE;
    }
    if (Magnitude >= THRESHOLD_MINOR)
    {
        return LEVEL_MINOR;
    }
    return LEVEL_NONE;
}

void PrintAlertBanner(int Level)
{
    PrintNewline();
    PrintStr("+========================================+");
    PrintNewline();
    
    switch (Level)
    {
        case LEVEL_MINOR:
            PrintStr("|     ! MINOR SEISMIC ACTIVITY !        |");
            break;
        case LEVEL_MODERATE:
            PrintStr("|    !! MODERATE EARTHQUAKE !!          |");
            break;
        case LEVEL_SEVERE:
            PrintStr("|   !!! SEVERE EARTHQUAKE !!!           |");
            break;
        case LEVEL_EXTREME:
            PrintStr("|  !!!! EXTREME EARTHQUAKE !!!!         |");
            break;
    }
    
    PrintNewline();
    PrintStr("+========================================+");
    PrintNewline();
}

void PrintReading(accel_reading *Reading, uint32_t STA, uint32_t LTA, int Level)
{
    PrintStr("  X:");
    PrintDecPadded(Reading->X, 5);
    PrintStr("  Y:");
    PrintDecPadded(Reading->Y, 5);
    PrintStr("  Z:");
    PrintDecPadded(Reading->Z, 5);
    PrintStr("  | Mag:");
    PrintDecPadded(Reading->Magnitude, 4);
    PrintStr("  | STA/LTA:");
    PrintDecPadded(STA, 4);
    PrintStr("/");
    PrintDecPadded(LTA, 3);
    
    PrintStr("  ");
    switch (Level)
    {
        case LEVEL_MINOR:    PrintStr("[MINOR]   "); break;
        case LEVEL_MODERATE: PrintStr("[MODERATE]"); break;
        case LEVEL_SEVERE:   PrintStr("[SEVERE]  "); break;
        case LEVEL_EXTREME:  PrintStr("[EXTREME] "); break;
        default:             PrintStr("          "); break;
    }
    
    PrintNewline();
}

void PrintEventSummary(uint32_t PeakMagnitude, uint32_t Duration)
{
    PrintNewline();
    PrintStr("+----------- EVENT SUMMARY -----------+");
    PrintNewline();
    PrintStr("| Peak Magnitude: ");
    PrintDec(PeakMagnitude);
    PrintNewline();
    PrintStr("| Duration: ");
    PrintDec(Duration);
    PrintStr(" samples");
    PrintNewline();
    PrintStr("| Classification: ");
    PrintStr(GetLevelString(ClassifyMagnitude(PeakMagnitude)));
    PrintNewline();
    PrintStr("| Total Events: ");
    PrintDec(TotalEvents);
    PrintNewline();
    PrintStr("+-------------------------------------+");
    PrintNewline();
    PrintNewline();
}

// ============ Delay Function ============

void Delay(int Cycles)
{
    for (volatile int I = 0; I < Cycles; I++)
    {
        // Busy wait
    }
}

// ============ Main Program ============

int main(void)
{
    accel_reading Reading;
    int CurrentLevel = LEVEL_NONE;
    int PreviousLevel = LEVEL_NONE;
    int InEvent = 0;
    uint32_t EventDuration = 0;
    uint32_t EventPeak = 0;
    uint32_t SampleCount = 0;
    uint32_t STALTARatioThreshold = 3;
    
    for (int I = 0; I < BUFFER_SIZE; I++)
    {
        MagnitudeBuffer[I] = 5;
    }
    
    PrintNewline();
    PrintStr("+===================================================+");
    PrintNewline();
    PrintStr("|        EARTHQUAKE EARLY WARNING SYSTEM            |");
    PrintNewline();
    PrintStr("|                 RISC-V Edition                    |");
    PrintNewline();
    PrintStr("+===================================================+");
    PrintNewline();
    PrintStr("|  Detection: STA/LTA Algorithm                     |");
    PrintNewline();
    PrintStr("|  Sensor: 3-Axis Accelerometer @ 0x20000000        |");
    PrintNewline();
    PrintStr("|  Thresholds:                                      |");
    PrintNewline();
    PrintStr("|    Minor: 30  Moderate: 60  Severe: 100  Ext: 150 |");
    PrintNewline();
    PrintStr("+===================================================+");
    PrintNewline();
    PrintNewline();
    PrintStr("System armed. Monitoring seismic activity...");
    PrintNewline();
    PrintNewline();
    
    while (1)
    {
        ReadAccelerometer(&Reading);
        SampleCount++;
        
        AddToBuffer(Reading.Magnitude);
        
        uint32_t STA = CalculateSTA();
        uint32_t LTA = CalculateLTA();
        
        if (LTA == 0)
        {
            LTA = 1;
        }
        
        CurrentLevel = ClassifyMagnitude(Reading.Magnitude);
        
        int STATrigger = (STA > LTA * STALTARatioThreshold);
        
        if (!InEvent && (CurrentLevel > LEVEL_NONE || STATrigger))
        {
            InEvent = 1;
            EventDuration = 0;
            EventPeak = Reading.Magnitude;
            ResetPeak();
            TotalEvents++;
            
            PrintAlertBanner(CurrentLevel > LEVEL_NONE ? CurrentLevel : LEVEL_MINOR);
        }
        
        if (InEvent)
        {
            EventDuration++;
            
            if (Reading.Magnitude > EventPeak)
            {
                EventPeak = Reading.Magnitude;
            }
            
            PrintReading(&Reading, STA, LTA, CurrentLevel);
            
            if (CurrentLevel > PreviousLevel && CurrentLevel > LEVEL_MINOR)
            {
                PrintStr("  *** INTENSITY INCREASING ***");
                PrintNewline();
            }
            
            if (CurrentLevel == LEVEL_NONE && STA <= LTA * 2)
            {
                InEvent = 0;
                PrintEventSummary(EventPeak, EventDuration);
                
                if (EventPeak > MaxRecordedMagnitude)
                {
                    MaxRecordedMagnitude = EventPeak;
                    PrintStr("  *** NEW MAXIMUM RECORDED ***");
                    PrintNewline();
                }
            }
        }
        else
        {
            if (SampleCount % STATUS_INTERVAL == 0)
            {
                PrintStr(".");
            }
            if (SampleCount % (STATUS_INTERVAL * 10) == 0)
            {
                PrintStr(" [");
                PrintDec(SampleCount);
                PrintStr(" samples, ");
                PrintDec(TotalEvents);
                PrintStr(" events]");
                PrintNewline();
            }
        }
        
        PreviousLevel = CurrentLevel;
        
        Delay(InEvent ? EVENT_DELAY : IDLE_DELAY);
    }
    
    return 0;
}