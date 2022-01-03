/*
 * --------------------------------------------------------------------------------------------------------------------
 * Example sketch/program showing how to detect when cards are removed.
 * Author: Martin Laclaustra
 * --------------------------------------------------------------------------------------------------------------------
 * This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid
 * 
 * Example sketch/program showing how to detect when a PICC (that is: a RFID Tag or Card) is present and when it is
 * removed from the field of detection using a MFRC522 based RFID Reader on the Arduino SPI interface.
 * 
 * When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
 * then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
 * you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
 * will show the NUID and report that it is "locked" onto that PICC. No other PICC will be detected until that one is
 * removed. The program will print "unlocked" when the card is not detected any more.
 * If a new card enters the range while locked into another card, it will be ignored, but it will be detected as soon
 * as the locked one is removed, and that will be the new locked card.
 * 
 * @license Released into the public domain.
 * 
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 * More pin layouts for other boards can be found here: https://github.com/miguelbalboa/rfid#pin-layout
 */

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

bool locked = false;

void setup() { 
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 
  delay(4);

  // Clear the information stored about locked cards.
  rfid.uid.size = 0;

  Serial.println(F("This code scan the MIFARE Classsic NUID and reports removal."));
}

void loop() {
  // Wake up all cards present within the sensor/reader range.
  bool cardPresent = PICC_IsAnyCardPresent();
  
  // Reset the loop if no card was locked an no card is present.
  // This saves the select process when no card is found.
  if (! locked && ! cardPresent)
    return;

  // When a card is present (locked) the rest ahead is intensive (constantly checking if still present).
  // Consider including code for checking only at time intervals.

  // Ask for the locked card (if rfid.uid.size > 0) or for any card if none was locked.
  // (Even if there was some error in the wake up procedure, attempt to contact the locked card.
  // This serves as a double-check to confirm removals.)
  // If a card was locked and now is removed, other cards will not be selected until next loop,
  // after rfid.uid.size has been set to 0.
  MFRC522::StatusCode result = rfid.PICC_Select(&rfid.uid,8*rfid.uid.size);

  if(!locked && result == MFRC522::STATUS_OK)
  {
    locked=true;
    // Action on card detection.
    Serial.print(F("locked! NUID tag: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  } else if(locked && result != MFRC522::STATUS_OK)
  {
    locked=false;
    rfid.uid.size = 0;
    // Action on card removal.
    Serial.print(F("unlocked! Reason for unlocking: "));
    Serial.println(rfid.GetStatusCodeName(result));
  } else if(!locked && result != MFRC522::STATUS_OK)
  {
    // Clear locked card data just in case some data was retrieved in the select procedure
    // but an error prevented locking.
    rfid.uid.size = 0;
  }

  rfid.PICC_HaltA();
}

/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(((buffer[i])>>4)&0x0F,  HEX);
    Serial.print(buffer[i]&0x0F, HEX);
    Serial.print(" ");
  }
}

// This convenience function could be added to the library in the future

/**
 * Returns true if a PICC responds to PICC_CMD_WUPA.
 * All cards in state IDLE or HALT are invited.
 * 
 * @return bool
 */
bool PICC_IsAnyCardPresent() {
  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);
  
  // Reset baud rates
  rfid.PCD_WriteRegister(rfid.TxModeReg, 0x00);
  rfid.PCD_WriteRegister(rfid.RxModeReg, 0x00);
  // Reset ModWidthReg
  rfid.PCD_WriteRegister(rfid.ModWidthReg, 0x26);
  
  MFRC522::StatusCode result = rfid.PICC_WakeupA(bufferATQA, &bufferSize);
  return (result == MFRC522::STATUS_OK || result == MFRC522::STATUS_COLLISION);
} // End PICC_IsAnyCardPresent()
