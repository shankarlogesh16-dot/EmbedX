#include <Wire.h>
#include "driver/spi_slave.h"

#define TARGET_RX 16
#define TARGET_TX 17
#define TARGET_BAUD 115200

#define DAC_PIN 25

#define I2C_SDA 21
#define I2C_SCL 22
#define I2C_ADDRESS 0x42

#define SPI_SCLK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define SPI_CS 5

#define SPI_TEST_LENGTH 8

HardwareSerial EmbedXSerial(2);

volatile bool i2cWriteReceived = false;

uint8_t spiTxBuffer[SPI_TEST_LENGTH];

uint8_t spiRxBuffer[SPI_TEST_LENGTH];

spi_slave_transaction_t embedxSpiTransaction;


/* ========================================
   UNIFIED RESULT VARIABLES
   ======================================== */

bool resultCommunication = false;
bool resultIdentification = false;
bool resultGPIO = false;
bool resultADC = false;
bool resultUART = false;
bool resultI2C = false;
bool resultSPI = false;

String targetName = "UNKNOWN";
String firmwareVersion = "UNKNOWN";


/* ========================================
   ADC REFERENCE TABLE
   ======================================== */

struct ADCReference
{
    int dacCode;
    int expectedVoltage;
};

ADCReference adcReferences[] =
{
    {40, 587},
    {80, 1069},
    {120, 1566},
    {160, 2055},
    {200, 2558}
};


/* ========================================
   I2C
   ======================================== */

void i2cReceive(int count)
{
    while (Wire.available())
    {
        Wire.read();
    }

    i2cWriteReceived = true;
}

void i2cRequest()
{
    const uint8_t response[] =
    {
        'I',
        '2',
        'C',
        '_',
        'O',
        'K'
    };

    Wire.slaveWrite(
        response,
        sizeof(response)
    );
}

void prepareI2CResponse()
{
    const uint8_t response[] =
    {
        'I',
        '2',
        'C',
        '_',
        'O',
        'K'
    };

    Wire.slaveWrite(
        response,
        sizeof(response)
    );
}

void initializeI2C()
{
    while (Wire.available())
    {
        Wire.read();
    }

    delay(200);
}


/* ========================================
   UART COMMUNICATION
   ======================================== */

void clearTargetBuffer()
{
    while (EmbedXSerial.available())
    {
        EmbedXSerial.read();
    }
}

void sendCommand(String command)
{
    clearTargetBuffer();

    EmbedXSerial.println(
        command
    );

    Serial.print(
        "COMMAND: "
    );

    Serial.println(
        command
    );
}

bool waitForTargetResponse(
    String expected,
    unsigned long timeout
)
{
    unsigned long startTime =
        millis();

    while (
        millis() - startTime <
        timeout
    )
    {
        if (EmbedXSerial.available())
        {
            String response =
                EmbedXSerial.readStringUntil(
                    '\n'
                );

            response.trim();

            Serial.print(
                "TARGET RESPONSE: "
            );

            Serial.println(
                response
            );

            if (response == expected)
            {
                return true;
            }
        }
    }

    return false;
}


/* ========================================
   PING TEST
   ======================================== */

void pingTest()
{
    sendCommand(
        "PING"
    );

    if (
        waitForTargetResponse(
            "PONG",
            2000
        )
    )
    {
        resultCommunication = true;

        Serial.println(
            "TARGET COMMUNICATION: PASS"
        );
    }
    else
    {
        resultCommunication = false;

        Serial.println(
            "TARGET COMMUNICATION: FAIL"
        );
    }
}


/* ========================================
   IDENTIFICATION TEST
   ======================================== */

void identifyTest()
{
    sendCommand(
        "IDENTIFY"
    );

    unsigned long startTime =
        millis();

    bool passed = false;

    while (
        millis() - startTime <
        2000
    )
    {
        if (EmbedXSerial.available())
        {
            String response =
                EmbedXSerial.readStringUntil(
                    '\n'
                );

            response.trim();

            Serial.print(
                "TARGET RESPONSE: "
            );

            Serial.println(
                response
            );

            if (response == "TARGET:ESP32")
            {
                targetName = "ESP32";
                passed = true;
                break;
            }
        }
    }

    resultIdentification = passed;

    if (passed)
    {
        Serial.println(
            "TARGET IDENTIFICATION: PASS"
        );
    }
    else
    {
        Serial.println(
            "TARGET IDENTIFICATION: FAIL"
        );
    }
}


/* ========================================
   VERSION TEST
   ======================================== */

void versionTest()
{
    sendCommand(
        "VERSION"
    );

    unsigned long startTime =
        millis();

    while (
        millis() - startTime <
        2000
    )
    {
        if (EmbedXSerial.available())
        {
            String response =
                EmbedXSerial.readStringUntil(
                    '\n'
                );

            response.trim();

            Serial.print(
                "TARGET VERSION: "
            );

            Serial.println(
                response
            );

            if (response.startsWith("VERSION:"))
            {
                firmwareVersion =
                    response.substring(8);

                firmwareVersion.trim();
            }

            return;
        }
    }

    firmwareVersion = "UNKNOWN";

    Serial.println(
        "VERSION: FAIL"
    );
}


/* ========================================
   GPIO TEST
   ======================================== */

void gpioTest()
{
    sendCommand(
        "GPIO_TEST"
    );

    if (
        waitForTargetResponse(
            "GPIO:PASS",
            3000
        )
    )
    {
        resultGPIO = true;

        Serial.println(
            "GPIO TEST: PASS"
        );
    }
    else
    {
        resultGPIO = false;

        Serial.println(
            "GPIO TEST: FAIL"
        );
    }
}


/* ========================================
   UART TEST
   ======================================== */

void uartTest()
{
    sendCommand(
        "UART_TEST:EMBEDX_UART_TEST"
    );

    if (
        waitForTargetResponse(
            "UART_RESPONSE:EMBEDX_UART_TEST",
            3000
        )
    )
    {
        resultUART = true;

        Serial.println(
            "UART TEST: PASS"
        );
    }
    else
    {
        resultUART = false;

        Serial.println(
            "UART TEST: FAIL"
        );
    }
}


/* ========================================
   I2C TEST
   ======================================== */

bool performI2CTest()
{
    sendCommand(
        "I2C_TEST"
    );

    return waitForTargetResponse(
        "I2C:PASS",
        1500
    );
}

void i2cTest()
{
    resultI2C = false;

    for (
        int attempt = 1;
        attempt <= 3;
        attempt++
    )
    {
        Serial.print(
            "I2C ATTEMPT: "
        );

        Serial.println(
            attempt
        );

        if (
            performI2CTest()
        )
        {
            resultI2C = true;

            Serial.println(
                "I2C TEST: PASS"
            );

            return;
        }

        delay(100);
    }

    Serial.println(
        "I2C TEST: FAIL"
    );
}


/* ========================================
   SPI INITIALIZATION
   ======================================== */

bool initializeSPI()
{
    spi_bus_config_t buscfg = {};

    buscfg.mosi_io_num =
        SPI_MOSI;

    buscfg.miso_io_num =
        SPI_MISO;

    buscfg.sclk_io_num =
        SPI_SCLK;

    buscfg.quadwp_io_num =
        -1;

    buscfg.quadhd_io_num =
        -1;

    buscfg.max_transfer_sz =
        SPI_TEST_LENGTH;

    spi_slave_interface_config_t slvcfg = {};

    slvcfg.spics_io_num =
        SPI_CS;

    slvcfg.flags = 0;

    slvcfg.queue_size = 1;

    slvcfg.mode = 0;

    slvcfg.post_setup_cb =
        NULL;

    slvcfg.post_trans_cb =
        NULL;

    esp_err_t result =
        spi_slave_initialize(
            HSPI_HOST,
            &buscfg,
            &slvcfg,
            SPI_DMA_DISABLED
        );

    return result == ESP_OK;
}


/* ========================================
   SPI PREPARE
   ======================================== */

bool prepareSPITransaction()
{
    memset(
        spiTxBuffer,
        0,
        SPI_TEST_LENGTH
    );

    memset(
        spiRxBuffer,
        0,
        SPI_TEST_LENGTH
    );

    memset(
        &embedxSpiTransaction,
        0,
        sizeof(
            embedxSpiTransaction
        )
    );

    const uint8_t response[
        SPI_TEST_LENGTH
    ] =
    {
        'S',
        'P',
        'I',
        '_',
        'O',
        'K',
        0,
        0
    };

    memcpy(
        spiTxBuffer,
        response,
        SPI_TEST_LENGTH
    );

    embedxSpiTransaction.length =
        SPI_TEST_LENGTH * 8;

    embedxSpiTransaction.tx_buffer =
        spiTxBuffer;

    embedxSpiTransaction.rx_buffer =
        spiRxBuffer;

    esp_err_t result =
        spi_slave_queue_trans(
            HSPI_HOST,
            &embedxSpiTransaction,
            pdMS_TO_TICKS(1000)
        );

    return result == ESP_OK;
}


/* ========================================
   SPI COMPLETE
   ======================================== */

bool completeSPITransaction()
{
    spi_slave_transaction_t
        *completedTransaction;

    esp_err_t result =
        spi_slave_get_trans_result(
            HSPI_HOST,
            &completedTransaction,
            pdMS_TO_TICKS(3000)
        );

    if (result != ESP_OK)
    {
        Serial.println(
            "SPI SLAVE TRANSACTION TIMEOUT"
        );

        return false;
    }

    Serial.print(
        "SPI RX: "
    );

    for (
        int i = 0;
        i < SPI_TEST_LENGTH;
        i++
    )
    {
        Serial.print(
            spiRxBuffer[i],
            HEX
        );

        Serial.print(
            " "
        );
    }

    Serial.println();

    const uint8_t expected[
        SPI_TEST_LENGTH
    ] =
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

    if (
        memcmp(
            spiRxBuffer,
            expected,
            SPI_TEST_LENGTH
        ) == 0
    )
    {
        return true;
    }

    return false;
}


/* ========================================
   SPI TEST
   ======================================== */

void spiTest()
{
    resultSPI = false;

    clearTargetBuffer();

    sendCommand(
        "SPI_TEST"
    );

    if (
        !waitForTargetResponse(
            "SPI_READY",
            2000
        )
    )
    {
        Serial.println(
            "SPI TEST: FAIL - TARGET NOT READY"
        );

        return;
    }

    Serial.println(
        "SPI SLAVE: PREPARING TRANSACTION"
    );

    if (
        !prepareSPITransaction()
    )
    {
        Serial.println(
            "SPI TEST: FAIL - SLAVE QUEUE"
        );

        return;
    }

    delay(20);

    sendCommand(
        "SPI_START"
    );

    if (
        !completeSPITransaction()
    )
    {
        Serial.println(
            "SPI TEST: FAIL - SPI DATA"
        );

        return;
    }

    if (
        waitForTargetResponse(
            "SPI:PASS",
            3000
        )
    )
    {
        resultSPI = true;

        Serial.println(
            "SPI TEST: PASS"
        );
    }
    else
    {
        Serial.println(
            "SPI TEST: FAIL - TARGET RESULT"
        );
    }
}


/* ========================================
   ADC TEST
   ======================================== */

void adcTest()
{
    Serial.println();
    Serial.println(
        "========== ADC TEST =========="
    );

    bool overallPass = true;

    int referenceCount =
        sizeof(adcReferences) /
        sizeof(adcReferences[0]);

    for (
        int i = 0;
        i < referenceCount;
        i++
    )
    {
        int dacCode =
            adcReferences[i].dacCode;

        int expectedVoltage =
            adcReferences[i].expectedVoltage;

        Serial.println();

        Serial.print(
            "ADC POINT: "
        );

        Serial.println(
            dacCode
        );

        Serial.print(
            "EXPECTED: "
        );

        Serial.print(
            expectedVoltage
        );

        Serial.println(
            " mV"
        );

        dacWrite(
            DAC_PIN,
            dacCode
        );

        delay(200);

        sendCommand(
            "ADC_TEST:" +
            String(dacCode)
        );

        unsigned long startTime =
            millis();

        bool responseReceived =
            false;

        while (
            millis() - startTime <
            3000
        )
        {
            if (EmbedXSerial.available())
            {
                String response =
                    EmbedXSerial.readStringUntil(
                        '\n'
                    );

                response.trim();

                Serial.print(
                    "TARGET RESPONSE: "
                );

                Serial.println(
                    response
                );

                if (
                    response.startsWith(
                        "ADC_RESULT:"
                    )
                )
                {
                    int measuredVoltage =
                        response.substring(
                            11
                        ).toInt();

                    Serial.print(
                        "MEASURED: "
                    );

                    Serial.print(
                        measuredVoltage
                    );

                    Serial.println(
                        " mV"
                    );

                    int difference =
                        abs(
                            measuredVoltage -
                            expectedVoltage
                        );

                    Serial.print(
                        "ERROR: "
                    );

                    Serial.print(
                        difference
                    );

                    Serial.println(
                        " mV"
                    );

                    if (
                        difference <= 100
                    )
                    {
                        Serial.println(
                            "ADC POINT: PASS"
                        );
                    }
                    else
                    {
                        Serial.println(
                            "ADC POINT: FAIL"
                        );

                        overallPass =
                            false;
                    }

                    responseReceived =
                        true;

                    break;
                }
            }
        }

        if (!responseReceived)
        {
            Serial.println(
                "ADC POINT: NO RESPONSE"
            );

            overallPass =
                false;
        }
    }

    dacWrite(
        DAC_PIN,
        0
    );

    Serial.println();
    Serial.println(
        "------------------------------"
    );

    if (overallPass)
    {
        resultADC = true;

        Serial.println(
            "ADC TEST: PASS"
        );
    }
    else
    {
        resultADC = false;

        Serial.println(
            "ADC TEST: FAIL"
        );
    }

    Serial.println(
        "------------------------------"
    );
}


/* ========================================
   UNIFIED RESULT
   ======================================== */

void printUnifiedResult()
{
    int passedTests = 0;
    int failedTests = 0;

    if (resultCommunication)
        passedTests++;
    else
        failedTests++;

    if (resultIdentification)
        passedTests++;
    else
        failedTests++;

    if (resultGPIO)
        passedTests++;
    else
        failedTests++;

    if (resultADC)
        passedTests++;
    else
        failedTests++;

    if (resultUART)
        passedTests++;
    else
        failedTests++;

    if (resultI2C)
        passedTests++;
    else
        failedTests++;

    if (resultSPI)
        passedTests++;
    else
        failedTests++;

    bool overallResult =
        failedTests == 0;

    Serial.println();
    Serial.println(
        "========================================"
    );

    Serial.println(
        "          EMBEDX DIAGNOSTIC RESULT"
    );

    Serial.println(
        "========================================"
    );

    Serial.println();

    Serial.println(
        "TARGET INFORMATION"
    );

    Serial.println(
        "----------------------------------------"
    );

    Serial.print(
        "Communication       : "
    );

    Serial.println(
        resultCommunication
        ? "PASS"
        : "FAIL"
    );

    Serial.print(
        "Target              : "
    );

    Serial.println(
        targetName
    );

    Serial.print(
        "Firmware            : "
    );

    Serial.println(
        firmwareVersion
    );

    Serial.println();

    Serial.println(
        "DIAGNOSTIC TESTS"
    );

    Serial.println(
        "----------------------------------------"
    );

    Serial.print(
        "GPIO                : "
    );

    Serial.println(
        resultGPIO
        ? "PASS"
        : "FAIL"
    );

    Serial.print(
        "ADC                 : "
    );

    Serial.println(
        resultADC
        ? "PASS"
        : "FAIL"
    );

    Serial.print(
        "UART                : "
    );

    Serial.println(
        resultUART
        ? "PASS"
        : "FAIL"
    );

    Serial.print(
        "I2C                 : "
    );

    Serial.println(
        resultI2C
        ? "PASS"
        : "FAIL"
    );

    Serial.print(
        "SPI                 : "
    );

    Serial.println(
        resultSPI
        ? "PASS"
        : "FAIL"
    );

    Serial.println();

    Serial.println(
        "----------------------------------------"
    );

    Serial.println(
        "TEST SUMMARY"
    );

    Serial.println(
        "----------------------------------------"
    );

    Serial.print(
        "Tests Passed        : "
    );

    Serial.println(
        passedTests
    );

    Serial.print(
        "Tests Failed        : "
    );

    Serial.println(
        failedTests
    );

    Serial.println();

    Serial.print(
        "OVERALL RESULT      : "
    );

    Serial.println(
        overallResult
        ? "PASS"
        : "FAIL"
    );

    Serial.println(
        "========================================"
    );
}


/* ========================================
   RUN ALL TESTS
   ======================================== */

void runAllTests()
{
    /*
       Reset previous results
       before starting a new run.
    */

    resultCommunication = false;
    resultIdentification = false;
    resultGPIO = false;
    resultADC = false;
    resultUART = false;
    resultI2C = false;
    resultSPI = false;

    targetName = "UNKNOWN";
    firmwareVersion = "UNKNOWN";

    Serial.println();
    Serial.println(
        "================================"
    );

    Serial.println(
        "      EMBEDX DIAGNOSTIC RUN"
    );

    Serial.println(
        "================================"
    );

    pingTest();

    Serial.println();

    identifyTest();

    Serial.println();

    versionTest();

    Serial.println();

    gpioTest();

    Serial.println();

    uartTest();

    Serial.println();

    i2cTest();

    Serial.println();

    spiTest();

    Serial.println();

    adcTest();

    Serial.println();

    Serial.println(
        "================================"
    );

    Serial.println(
        "      DIAGNOSTIC RUN COMPLETE"
    );

    Serial.println(
        "================================"
    );

    /*
       Final unified report
    */

    printUnifiedResult();
}


/* ========================================
   MENU
   ======================================== */

void printMenu()
{
    Serial.println();
    Serial.println(
        "========== EMBEDX =========="
    );

    Serial.println(
        "1. PING"
    );

    Serial.println(
        "2. IDENTIFY"
    );

    Serial.println(
        "3. GET VERSION"
    );

    Serial.println(
        "4. GPIO TEST"
    );

    Serial.println(
        "5. ADC TEST"
    );

    Serial.println(
        "6. UART TEST"
    );

    Serial.println(
        "7. I2C TEST"
    );

    Serial.println(
        "8. SPI TEST"
    );

    Serial.println(
        "9. RUN ALL TESTS"
    );

    Serial.println(
        "============================"
    );

    Serial.println(
        "Enter option:"
    );
}


/* ========================================
   SETUP
   ======================================== */

void setup()
{
    Serial.begin(
        115200
    );

    EmbedXSerial.begin(
        TARGET_BAUD,
        SERIAL_8N1,
        TARGET_RX,
        TARGET_TX
    );

    Wire.onReceive(
        i2cReceive
    );

    Wire.onRequest(
        i2cRequest
    );

    Wire.begin(
        I2C_ADDRESS,
        I2C_SDA,
        I2C_SCL,
        100000
    );

    initializeI2C();

    prepareI2CResponse();

    if (
        initializeSPI()
    )
    {
        Serial.println(
            "SPI INITIALIZATION: PASS"
        );
    }
    else
    {
        Serial.println(
            "SPI INITIALIZATION: FAIL"
        );
    }

    delay(2000);

    clearTargetBuffer();

    Serial.println();

    Serial.println(
        "EMBEDX CONTROLLER READY"
    );

    delay(500);

    pingTest();

    printMenu();
}


/* ========================================
   LOOP
   ======================================== */

void loop()
{
    if (Serial.available())
    {
        String command =
            Serial.readStringUntil(
                '\n'
            );

        command.trim();

        if (command == "1")
        {
            pingTest();
        }
        else if (command == "2")
        {
            identifyTest();
        }
        else if (command == "3")
        {
            versionTest();
        }
        else if (command == "4")
        {
            gpioTest();
        }
        else if (command == "5")
        {
            adcTest();
        }
        else if (command == "6")
        {
            uartTest();
        }
        else if (command == "7")
        {
            i2cTest();
        }
        else if (command == "8")
        {
            spiTest();
        }
        else if (command == "9")
        {
            runAllTests();
        }
        else
        {
            Serial.println(
                "INVALID OPTION"
            );
        }

        printMenu();
    }

    if (i2cWriteReceived)
    {
        i2cWriteReceived = false;
    }
}