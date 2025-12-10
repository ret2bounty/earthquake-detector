#include <stdint.h>

// These match the addresses in our .repl file
#define UART_BASE   0x10000000
#define SENSOR_ADDR 0x20000000

// UART Register Offsets (NS16550 standard)
#define UART_THR    0x00 // Transmitter Holding Register

// Pointers to hardware
volatile uint32_t* uart = (volatile uint32_t*)UART_BASE;
volatile uint32_t* sensor = (volatile uint32_t*)SENSOR_ADDR;

void print_char(char c) {
    *uart = c;
}

void print_str(const char* s) {
    while (*s) print_char(*s++);
}

int main() {
    print_str("System Armed. Monitoring seismic activity...\n");

    int threshold = 50; // G-force threshold (arbitrary unit)

    while (1) {
        // Read the virtual sensor
        uint32_t vibration_level = *sensor;

        if (vibration_level > threshold) {
            print_str("!!! EARTHQUAKE DETECTED !!! Level: ");
            // (Simple hex printing logic could go here)
            print_char('0' + (vibration_level / 100)); 
            print_char('\n');
        }

        // Wait a bit to avoid spamming
        for (volatile int i = 0; i < 1000; i++); 
    }
    return 0;
}