#include <reg51.h>

// Define ports and pins
#define lcd_port P2
#define adc_port P1

// LCD Port
sbit rs = P3^0;
sbit rw = P3^1;
sbit en = P3^2;
sbit flag = P2^7;
sbit mode = P3^3;  // Switch to toggle between Voltmeter and Ammeter

// ADC Port
sbit rd = P3^7;
sbit wr = P3^6;
sbit cs = P3^5;
sbit intr = P3^4;

// Function prototypes
void busy();
void lcd_cmd(unsigned char);
void lcd_data(unsigned char);
void lcd_string(unsigned char *);
void lcd_init();
void hex2lcd(unsigned char);
void conv();
void read();
void delay(unsigned int);

// Global variables
unsigned int adc_avg, adc;
bit current_mode = 0;  // 0 = Voltmeter, 1 = Ammeter

void busy() {
    flag = 1;
    rs = 0;
    rw = 1;
    while(flag != 0) {
        en = 0;
        en = 1;
    }
}

void lcd_cmd(unsigned char val_lcd) {
    busy();
    lcd_port = val_lcd;
    rs = 0;
    rw = 0;
    en = 1;
    en = 0;
}

void lcd_data(unsigned char dat) {
    busy();
    lcd_port = dat;
    rs = 1;
    rw = 0;
    en = 1;
    en = 0;
}

void lcd_string(unsigned char *string) {
    while(*string)
        lcd_data(*string++);
}

void lcd_init(void) {
    lcd_cmd(0x02);
    lcd_cmd(0x38);
    lcd_cmd(0x01);
    lcd_cmd(0x0c);
    lcd_cmd(0x06);
    lcd_cmd(0x80);
}

void hex2lcd(unsigned char hex) {
    char temp1, temp2;
    temp1 = hex;
    temp2 = 0;
    do {
        temp1 = temp1 - 100;
        if (temp1 >= 0) temp2++;
    } while (temp1 >= 0);
    if (temp2 > 0)
        lcd_data(temp2 + 0x30);
    temp2 = 0;
    temp1 = temp1 + 100;
    do {
        temp1 = temp1 - 10;
        if (temp1 >= 0) temp2++;
    } while (temp1 >= 0);
    lcd_data(temp2 + 0x30);
    temp2 = temp1 + 10;
    lcd_data(temp2 + 0x30);
}

void conv() {
    cs = 0;
    wr = 0;
    wr = 1;
    cs = 1;
    while(intr);
}

void read() {
    cs = 0;
    rd = 0;
    adc = adc_port;
    rd = 1;
    cs = 1;
}

void delay(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 123; j++);
}

void main() {
    char i;
    lcd_init();  // Initialize LCD

    // Display initial mode
    if (current_mode == 0) {
        lcd_string("Voltmeter");
    } else {
        lcd_string("Ammeter");
    }

    while (1) {
        // Check if the switch is pressed (active low)
        if (mode == 0) {
            delay(20);  // Debounce delay
            if (mode == 0) {  // Confirm switch is still pressed
                current_mode = !current_mode;  // Toggle mode
                lcd_cmd(0x01);  // Clear LCD
                if (current_mode == 0) {
                    lcd_string("Voltmeter");
                } else {
                    lcd_string("Ammeter");
                }
                while (mode == 0);  // Wait for switch release
            }
        }

        // Read ADC and display value based on mode
        adc_avg = 0;
        for (i = 0; i < 10; i++) {
            conv();  // Start ADC conversion
            read();  // Read ADC value
            adc_avg += adc;  // Accumulate ADC values
        }
        adc_avg = adc_avg / 10;  // Calculate average ADC value

        lcd_cmd(0xC0);  // Move cursor to second line of LCD
        adc = adc_avg * 59;  // Convert ADC value to voltage/current
        if (current_mode == 1) {  // If in Ammeter mode
            adc = (adc) / 15;  // Adjust for current measurement
        }

        // Display the value on LCD
        hex2lcd((unsigned char)(adc / 1000));  // Display thousands place
        lcd_data('.');  // Display decimal point
        adc = adc % 1000;  // Get the remaining value
        hex2lcd((unsigned char)(adc / 10));  // Display tenths place

        // Display unit based on mode
        if (current_mode == 0) {
            lcd_string("V");  // Display "V" for Voltmeter
        } else {
            lcd_string("mA");  // Display "A" for Ammeter
        }
    }
}
