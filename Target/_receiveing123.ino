#include <Wire.h>
#include <SPI.h>

#define EMBEDX_TARGET "ESP32"
#define EMBEDX_VERSION "2.3"

#define TARGET_RX 16
#define TARGET_TX 17
#define TARGET_BAUD 115200

#define GPIO_TEST_OUT 26
#define GPIO_TEST_IN 27

#define ADC_TEST_PIN 34

#define I2C_SDA 21
#define I2C_SCL 22
#define I2C_ADDRESS 0x42

#define SPI_SCLK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define SPI_CS 5

#define SPI_TEST_LENGTH 8
#define SPI_CLOCK 1000000

HardwareSerial EmbedXSerial(2);

SPIClass EmbedXSPI(HSPI);

void gpioTest()
{
    pinMode(GPIO_TEST_OUT, OUTPUT);
    pinMode(GPIO_TEST_IN, INPUT);

    digitalWrite(GPIO_TEST_OUT, HIGH);
    delay(5);

    int highState =
        digitalRead(GPIO_TEST_IN);

    digitalWrite(GPIO_TEST_OUT, LOW);
    delay(5);

    int lowState =
        digitalRead(GPIO_TEST_IN);

    if (highState == HIGH &&
        lowState == LOW)
    {
        EmbedXSerial.println("GPIO:PASS");
    }
    else
    {
        EmbedXSerial.println("GPIO:FAIL");
    }
}

void uartTest(String data)
{
    if (data == "EMBEDX_UART_TEST")
    {
        EmbedXSerial.println(
            "UART_RESPONSE:EMBEDX_UART_TEST"
        );
    }
    else
    {
        EmbedXSerial.println(
            "UART:DATA_ERROR"
        );
    }
}

bool i2cTest()
{
    const char testData[] =
        "EMBEDX_I2C_TEST";

    delay(50);

    Wire.beginTransmission(
        I2C_ADDRESS
    );

    Wire.write(
        (const uint8_t *)testData,
        sizeof(testData) - 1
    );

    uint8_t error =
        Wire.endTransmission(true);

    if (error != 0)
        return false;

    delay(30);

    uint8_t received =
        Wire.requestFrom(
            I2C_ADDRESS,
            (uint8_t)6,
            true
        );

    if (received != 6)
        return false;

    char response[7];

    for (int i = 0; i < 6; i++)
    {
        if (Wire.available())
        {
            response[i] =
                Wire.read();
        }
        else
        {
            return false;
        }
    }

    response[6] = '\0';

    if (strcmp(
            response,
            "I2C_OK"
        ) == 0)
    {
        return true;
    }

    return false;
}

void spiReady()
{
    EmbedXSerial.println("SPI_READY");
}

bool spiTransaction()
{
    const uint8_t testData[SPI_TEST_LENGTH] =
    {
        'S',
        'P',
        'I',
        '_',
        'T',
        'E',
        'S',
        'T'
    };

    uint8_t receivedData[SPI_TEST_LENGTH];

    memcpy(
        receivedData,
        testData,
        SPI_TEST_LENGTH
    );

    EmbedXSPI.beginTransaction(
        SPISettings(
            SPI_CLOCK,
            MSBFIRST,
            SPI_MODE0
        )
    );

    digitalWrite(
        SPI_CS,
        LOW
    );

    EmbedXSPI.transfer(
        receivedData,
        SPI_TEST_LENGTH
    );

    digitalWrite(
        SPI_CS,
        HIGH
    );

    EmbedXSPI.endTransaction();

    Serial.print(
        "SPI RECEIVED: "
    );

    for (int i = 0;
         i < SPI_TEST_LENGTH;
         i++)
    {
        Serial.print(
            receivedData[i],
            HEX
        );

        Serial.print(" ");
    }

    Serial.println();

    if (receivedData[0] == 'S' &&
        receivedData[1] == 'P' &&
        receivedData[2] == 'I' &&
        receivedData[3] == '_' &&
        receivedData[4] == 'O' &&
        receivedData[5] == 'K')
    {
        return true;
    }

    return false;
}

void adcTest(int dacCode)
{
    const int samples = 10;

    long totalVoltage = 0;

    delay(100);

    for (int i = 0;
         i < samples;
         i++)
    {
        totalVoltage +=
            analogReadMilliVolts(
                ADC_TEST_PIN
            );

        delay(10);
    }

    int averageVoltage =
        totalVoltage / samples;

    EmbedXSerial.print(
        "ADC_RESULT:"
    );

    EmbedXSerial.println(
        averageVoltage
    );
}

void handleCommand(String command)
{
    command.trim();

    if (command == "PING")
    {
        EmbedXSerial.println(
            "PONG"
        );
    }
    else if (command == "IDENTIFY")
    {
        EmbedXSerial.println(
            "TARGET:ESP32"
        );
    }
    else if (command == "VERSION")
    {
        EmbedXSerial.println(
            "VERSION:2.3"
        );
    }
    else if (command == "GPIO_TEST")
    {
        gpioTest();
    }
    else if (command.startsWith(
                 "UART_TEST:"))
    {
        uartTest(
            command.substring(10)
        );
    }
    else if (command == "I2C_TEST")
    {
        if (i2cTest())
        {
            EmbedXSerial.println(
                "I2C:PASS"
            );
        }
        else
        {
            EmbedXSerial.println(
                "I2C:FAIL"
            );
        }
    }
    else if (command == "SPI_TEST")
    {
        spiReady();
    }
    else if (command == "SPI_START")
    {
        bool result =
            spiTransaction();

        if (result)
        {
            EmbedXSerial.println(
                "SPI:PASS"
            );
        }
        else
        {
            EmbedXSerial.println(
                "SPI:FAIL"
            );
        }
    }
    else if (command.startsWith(
                 "ADC_TEST:"))
    {
        int dacCode =
            command.substring(9).toInt();

        adcTest(dacCode);
    }
    else
    {
        EmbedXSerial.println(
            "ERROR:UNKNOWN_COMMAND"
        );
    }
}

void setup()
{
    Serial.begin(115200);

    EmbedXSerial.begin(
        TARGET_BAUD,
        SERIAL_8N1,
        TARGET_RX,
        TARGET_TX
    );

    Wire.begin(
        I2C_SDA,
        I2C_SCL,
        100000
    );

    EmbedXSPI.begin(
        SPI_SCLK,
        SPI_MISO,
        SPI_MOSI,
        SPI_CS
    );

    pinMode(
        SPI_CS,
        OUTPUT
    );

    digitalWrite(
        SPI_CS,
        HIGH
    );

    analogReadResolution(12);

    analogSetPinAttenuation(
        ADC_TEST_PIN,
        ADC_11db
    );

    delay(500);

    Serial.println(
        "EMBEDX:TARGET_READY"
    );
}

void loop()
{
    if (EmbedXSerial.available())
    {
        String command =
            EmbedXSerial.readStringUntil(
                '\n'
            );

        command.trim();

        Serial.print(
            "RECEIVED:["
        );

        Serial.print(command);

        Serial.println("]");

        handleCommand(command);
    }
}