
#include <RFduinoBLE.h>
#include <stdlib.h>

#include </home/edward/Desktop/test-ndn-cpp/include/ndn-cpp/lite/data-lite.hpp>
#include </home/edward/Desktop/test-ndn-cpp/include/ndn-cpp/lite/interest-lite.hpp>
#include </home/edward/Desktop/test-ndn-cpp/include/ndn-cpp/lite/util/crypto-lite.hpp>
#include </home/edward/Desktop/test-ndn-cpp/include/ndn-cpp/lite/encoding/tlv-0_2-wire-format-lite.hpp>

using namespace ndn;

// be careful about making the name too long; the loop that sends out
// the data fragments is an unrolled loop because having it unrolled caused
// the arduino to crash sometimes for reasons not figured out yet; if you want
// to make the name a lot longer, you will have to manually code in another loop
char beaconLocation[17] = "ICEAR Location 3";
char beaconName[15] = "ICEAR Beacon 3";

// pin 3 on the RGB shield is the green led
// (shows when the RFduino is advertising or not)
int advertisement_led = 3;

// pin 2 on the RGB shield is the red led
// (goes on when the RFduino has a connection from the iPhone, and goes off on disconnect)
int connection_led = 2;

const int blePacketSize = 20;

// int to indicate whether we are in:
//  - mode 0 (waiting to receive ~ character to indicate we are about to receive a packet length)
//  - mode 1 (received a ~ character, now waiting for 2 bytes of packet length)
//  - mode 2 (received packet length, now waiting to receive the actual packet with the packet length amount of bytes)
int mode = 0;
// length of packet currently being received
int currentPacketLength;
// buffer to hold bytes of incoming packet (just set to 100, the interest for /icear/beacon/read/MAC-Address is 49 bytes,
// so thought that 100 bytes would be reasonable limit)
byte incoming_packet_bytes[100];
// index of bytes of packet currently being received
int currentPacketIndex;

// data structures for signing the data packet sent out with the hmacKey, hard coded in, same key is in BLEApp3 and mobile terminal
uint8_t hmacKeyDigest[ndn_SHA256_DIGEST_SIZE];
byte hmacKey[4] = {
  1, 2, 3, 4
};

// maximum amount of time rfduino will stay connected to a device before automatically disconnecting
const int maxConnectionTime = 4;

int currentlyConnected = 0;
int shouldRestartBLE = 0;
int secondsCounter = 0;
uint_fast16_t volatile number_of_ms = 1000;

void startTimer(void)                    // directly pass the value you want the cycle to be in mS
{
  NVIC_SetPriority( TIMER2_IRQn, 2 );
  NRF_TIMER2->TASKS_STOP = 1;  // Stop timer
  NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;  // taken from Nordic dev zone
  NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
  NRF_TIMER2->PRESCALER = 9;  // 32us resolution
  NRF_TIMER2->TASKS_CLEAR = 1; // Clear timer
  // With 32 us ticks, we need to multiply by 31.25 to get milliseconds
  NRF_TIMER2->CC[0] = number_of_ms * 31;
  NRF_TIMER2->CC[0] += number_of_ms / 4;
  NRF_TIMER2->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
  NRF_TIMER2->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);
  attachInterrupt(TIMER2_IRQn, TIMER2_Interrupt);
  NRF_TIMER2->TASKS_START = 1;  // Start TIMER


}

void TIMER2_Interrupt(void)
{

  //Serial.println(F("entered interrupt"));

  if (NRF_TIMER2->EVENTS_COMPARE[0] != 0)
  {
    secondsCounter++;
    //Serial.println(secondsCounter);

    if (secondsCounter > maxConnectionTime) {
      secondsCounter = 0;

      if (currentlyConnected == 1) {

        shouldRestartBLE = 1;

        currentlyConnected = 0;
      }
      else {
        //Serial.println(F("We weren't connected, so not resetting"));
      }

      NRF_TIMER2->TASKS_START = 0;
      NRF_TIMER2->TASKS_STOP = 1;
    }

    NRF_TIMER2->EVENTS_COMPARE[0] = 0;

  }
}

void setup() {

  // led used to indicate that the RFduino is advertising
  pinMode(advertisement_led, OUTPUT);

  // led used to indicate that the RFduino is connected
  pinMode(connection_led, OUTPUT);

  Serial.begin(9600);

  // 128 bit base uuid
  // (generated with http://www.uuidgenerator.net)
  RFduinoBLE.customUUID = "ccc433f0-be8f-4dc8-b6f0-5343e6100eb4";

  RFduinoBLE.deviceName = beaconName;

  // this must match the customUUID in the iPhone app declared
  // in +Load method of AppViewController.m

  // start the BLE stack
  RFduinoBLE.begin();
}

void loop() {
  // switch to lower power mode
  RFduino_ULPDelay(5000);

  if (shouldRestartBLE == 1) {

    Serial.println("Got signal to restart BLE.");

    shouldRestartBLE = 0;

    currentlyConnected = 0;

    mode = 0;
    currentPacketLength = 0;
    currentPacketIndex = 0;

    digitalWrite(advertisement_led, LOW);
    digitalWrite(connection_led, LOW);

    RFduinoBLE.end();
    RFduinoBLE.begin();
  }
}

void RFduinoBLE_onAdvertisement(bool start)
{
  // turn the green led on if we start advertisement, and turn it
  // off if we stop advertisement

  if (start)
    digitalWrite(advertisement_led, HIGH);
  else
    digitalWrite(advertisement_led, LOW);
}

void RFduinoBLE_onConnect()
{
  digitalWrite(connection_led, HIGH);
  Serial.println(F("got connected"));

  currentlyConnected = 1;

  startTimer();
}

void RFduinoBLE_onDisconnect()
{
  digitalWrite(connection_led, LOW);
  Serial.println(F("got disconnected"));

  currentlyConnected = 0;

  mode = 0;
  currentPacketLength = 0;
  currentPacketIndex = 0;
}

void RFduinoBLE_onReceive(char *data, int len) {

  Serial.print("Got data: ");
  Serial.println(data);
  byte firstByte = data[0];

  if (mode == 0) {
    if (firstByte == '~') {
      Serial.println("Got ~, going into mode 1.");

      mode = 1;

      sendNCharacterMessage(blePacketSize, "mode 1");
    }
    else {
      Serial.print("Got this while in mode 0: ");
      Serial.println(data);
    }
  }
  else if (mode == 1) {
    if (len == 2) {
      currentPacketLength = (0x0000) | (data[0] << 8) | data[1];

      Serial.print("Got packet length: ");
      Serial.println(currentPacketLength);

      if (currentPacketLength > 100) {
        Serial.println("Packet length greater than 100, ignoring this packet.");
        mode = 0;
        return;
      }

      Serial.println("Going into mode 2.");

      mode = 2;

      sendNCharacterMessage(blePacketSize, "mode 2");
    }
    else {
      Serial.print("Got this while in mode 1: ");
      Serial.println(data);
    }
  }
  else if (mode == 2) {
    for (int i = 0; i < len; i++) {
      incoming_packet_bytes[currentPacketIndex++] = data[i];
    }

    if (currentPacketIndex == currentPacketLength) {
      Serial.println("Done receiving packet in mode 2.");
      mode = 0;

      ndn_NameComponent interestNameComponents[10];
      struct ndn_ExcludeEntry excludeEntries[9];

      InterestLite interest
      (interestNameComponents, sizeof(interestNameComponents) / sizeof(interestNameComponents[0]),
       excludeEntries, sizeof(excludeEntries) / sizeof(excludeEntries[0]), 0, 0);
      size_t signedPortionBeginOffset, signedPortionEndOffset;
      ndn_Error error;
      if ((error = Tlv0_2WireFormatLite::decodeInterest
                   (interest, incoming_packet_bytes, currentPacketLength, &signedPortionBeginOffset,
                    &signedPortionEndOffset))) {
        Serial.println("Error when decoding interest.");
      }

      if (interest.getName().size() < 4) {
        return;
      }

      // Create the response data packet.
      ndn_NameComponent dataNameComponents[4];
      DataLite data(dataNameComponents, sizeof(dataNameComponents) / sizeof(dataNameComponents[0]), 0, 0);
      data.getName().append("icear");
      data.getName().append("beacon");
      data.getName().append("read");
      // this appends the mac address of the BLE module to the data packet name
      data.getName().append(interest.getName().get(3));

      if (interest.getName().equals(data.getName())) {
        Serial.println("Got read interest.");

        // Set the content to a string, location.
        data.setContent(BlobLite((const uint8_t*)beaconLocation, strlen(beaconLocation)));

        // Set up the signature with the hmacKeyDigest key locator digest.
        // TODO: Change to ndn_SignatureType_HmacWithSha256Signature when
        //   SignatureHmacWithSha256 is in the NDN-TLV Signature spec:
        //   http://named-data.net/doc/ndn-tlv/signature.html
        data.getSignature().setType(ndn_SignatureType_Sha256WithRsaSignature);
        data.getSignature().getKeyLocator().setType(ndn_KeyLocatorType_KEY_LOCATOR_DIGEST);
        data.getSignature().getKeyLocator().setKeyData(BlobLite(hmacKeyDigest, sizeof(hmacKeyDigest)));

        // Encode once to get the signed portion.
        uint8_t encoding[240];
        DynamicUInt8ArrayLite output(encoding, sizeof(encoding), 0);
        size_t encodingLength;
        if ((error = Tlv0_2WireFormatLite::encodeData
                     (data, &signedPortionBeginOffset, &signedPortionEndOffset,
                      output, &encodingLength)))
          Serial.println("Failure encoding once to get signed portion.");

        // Get the signature for the signed portion.
        uint8_t signatureValue[ndn_SHA256_DIGEST_SIZE];
        CryptoLite::computeHmacWithSha256
        (hmacKey, sizeof(hmacKey), encoding + signedPortionBeginOffset,
         signedPortionEndOffset - signedPortionBeginOffset, signatureValue);
        data.getSignature().setSignature(BlobLite(signatureValue, ndn_SHA256_DIGEST_SIZE));

        // Encode again to include the signature.
        if ((error = Tlv0_2WireFormatLite::encodeData
                     (data, &signedPortionBeginOffset, &signedPortionEndOffset,
                      output, &encodingLength)))
          Serial.println("Failure encoding again to include the signature.");

        // the '~' character indicates to BLEApp3 that we are sending it a data packet (the '-' character
        // would indicate an interest packet)
        sendByteArrayOverBLE(encoding, encodingLength, '~');

        currentPacketLength = 0;
        currentPacketIndex = 0;
      }
    }
  }

}

void sendByteArrayOverBLE (byte * arr, unsigned int arraySize, unsigned char c) {

  char signalCharacterArray[blePacketSize];

  for (int i = 0; i < blePacketSize; i++) {
    signalCharacterArray[i] = c;
  }

  while (! RFduinoBLE.send(signalCharacterArray, blePacketSize));

  char arraySizeShort[blePacketSize];
  arraySizeShort[0] = ((arraySize >> 8) & 0xff);
  arraySizeShort[1] = arraySize & 0xff;
  for (int i = 2; i < blePacketSize; i++) {
    arraySizeShort[i] = ' ';
  }

  while (! RFduinoBLE.send(arraySizeShort, blePacketSize));

  // manually coded loop to send out the data packet, just left it here in case it's needed for some reason again
  char arrayToSend1[blePacketSize];
  char arrayToSend2[blePacketSize];
  char arrayToSend3[blePacketSize];
  char arrayToSend4[blePacketSize];
  char arrayToSend5[blePacketSize];
  char arrayToSend6[blePacketSize];
  char arrayToSend7[blePacketSize];
  int currentSendIndex = 0;
  unsigned int leftOverCount = blePacketSize - (arraySize % blePacketSize);
  if (arraySize % blePacketSize == 0)
    leftOverCount = 0;

  for (int i = 0; i < blePacketSize; i++) {
    arrayToSend1[i] = arr[currentSendIndex++];
  }

  while (! RFduinoBLE.send(arrayToSend1, blePacketSize));

  Serial.print("Current send index after array 1: ");
  Serial.println(currentSendIndex);

  for (int i = 0; i < blePacketSize; i++) {
    arrayToSend2[i] = arr[currentSendIndex++];
  }

  while (! RFduinoBLE.send(arrayToSend2, blePacketSize));

  Serial.print("Current send index after array 2: ");
  Serial.println(currentSendIndex);

  for (int i = 0; i < blePacketSize; i++) {
    arrayToSend3[i] = arr[currentSendIndex++];
  }

  while (! RFduinoBLE.send(arrayToSend3, blePacketSize));

  Serial.print("Current send index after array 3: ");
  Serial.println(currentSendIndex);

  for (int i = 0; i < blePacketSize; i++) {
    arrayToSend4[i] = arr[currentSendIndex++];
  }

  while (! RFduinoBLE.send(arrayToSend4, blePacketSize));

  Serial.print("Current send index after array 4: ");
  Serial.println(currentSendIndex);

  for (int i = 0; i < blePacketSize; i++) {
    arrayToSend5[i] = arr[currentSendIndex++];
  }

  while (! RFduinoBLE.send(arrayToSend5, blePacketSize));

  Serial.print("Current send index after array 5: ");
  Serial.println(currentSendIndex);

  for (int i = 0; i < blePacketSize; i++) {
    arrayToSend6[i] = arr[currentSendIndex++];
  }

  while (! RFduinoBLE.send(arrayToSend6, blePacketSize));

  Serial.print("Current send index after array 6: ");
  Serial.println(currentSendIndex);

  for (int i = 0; currentSendIndex < arraySize; i++) {
    arrayToSend7[i] = arr[currentSendIndex++];
  }

  Serial.println("Before sending last chunk.");

  while (! RFduinoBLE.send(arrayToSend7, blePacketSize));

  Serial.println("After sending last chunk.");

}


void sendNCharacterMessage(int length, String message) {

  int messageSize = message.length();

  if (messageSize > length) {
    Serial.print("printNChar error   /");
    return;
  }

  String finalMessage = message;

  for (int i = 0; i < length - messageSize; i++) {
    finalMessage += ' ';
  }

  if (messageSize != length) {
    finalMessage[length - 1] = '/';
  }

  char finalMessageCharArray[length];

  for (int i = 0 ; i < length; i++) {
    finalMessageCharArray[i] = finalMessage[i];
  }

  while (! RFduinoBLE.send(finalMessageCharArray, length));

}
