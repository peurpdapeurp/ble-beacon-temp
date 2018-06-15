
//*** THIS CODE WAS ADAPTED FROM HERE: https://github.com/named-data/ndn-cpp/tree/master/examples/arduino

#include <Bridge.h>

#include <ndn-cpp/lite/data-lite.hpp>
#include <ndn-cpp/lite/interest-lite.hpp>
#include <ndn-cpp/lite/util/crypto-lite.hpp>
#include <ndn-cpp/lite/encoding/tlv-0_2-wire-format-lite.hpp>

using namespace ndn;

// the read modes are as follows:
// mode 0: means that the arduino is waiting for a '~' to put it into length reading mode (mode 1)
// mode 1: means we got a '~' character, we are now waiting for a packet length (integer in short format), once we get the length we go into packet reading mode (mode 2)
// mode 2: means we got the packet length, we are now reading as many bytes as the packet length, and then doing processing on the full packet, after
//         which we go back into mode 0
byte currentReadMode = 0;


char beaconLocation[16] = "Random location";

unsigned int currentPacketLength = 0;

// variables to check if we are first entering a mode, since the program
// runs on a loop and want to know when we first enter a mode
byte firstTime0 = 1;
byte firstTime1 = 1;
byte firstTime2 = 1;

// the packet size of BLE is 20 bytes (https://stackoverflow.com/questions/38913743/maximum-packet-length-for-bluetooth-le)
int blePacketSize = 20;

// variable to keep track of index of current packet being read; has to be outside of loop
// to be remembered
unsigned int currentPacketIndex = 0;

// this is the byte array that reads in the packet we get
byte *incoming_packet_bytes;

// array that stores the incoming bytes of the packet length
byte packetLengthReadArray[2];

// data structures for signing the data packet sent out with the hmacKey, hard coded in, same key is in BLEApp3 and mobile terminal
uint8_t hmacKeyDigest[ndn_SHA256_DIGEST_SIZE];
byte hmacKey[4] = {
  1, 2, 3, 4
};

void setup() {

  //pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);

}

void loop() {

  if (currentReadMode == 0) {
    if (firstTime0 == 1) {
      printNCharacterMessage(blePacketSize, "we in mode 0");

      unsigned int testNum = (unsigned int) ndn_SHA256_DIGEST_SIZE;

      printUnsignedInt(testNum);

      firstTime0 = 0;
      firstTime1 = 1;
      firstTime2 = 1;
    }

    while ( Serial.available() > 0 )
    {
      if (Serial.read() == '~') {
        printNCharacterMessage(blePacketSize, "we got a ~");
        currentReadMode = 1;
      }
      else if (Serial.read() == '1') {
        printNCharacterMessage(blePacketSize, "we got a 1");
      }
    }


  }
  else if (currentReadMode == 1) {
    if (firstTime1 == 1) {
      printNCharacterMessage(blePacketSize, "mode 1");

      firstTime0 = 1;
      firstTime1 = 0;
      firstTime2 = 1;
    }

    while ( Serial.available() < 2) {
      // hang in this loop until we get 2 bytes of packet length data
    }

    for (int i = 0; i < 2; i++) {
      packetLengthReadArray[i] = Serial.read();
    }

    currentPacketLength = (0x0000) | (packetLengthReadArray[0] << 8) | packetLengthReadArray[1];

    printUnsignedInt(currentPacketLength);

    currentReadMode = 2;


  }
  else if (currentReadMode == 2) {

    if (firstTime2 == 1) {

      printNCharacterMessage(blePacketSize, "mode 2");

      incoming_packet_bytes = malloc(currentPacketLength);

      if (incoming_packet_bytes == NULL) {
        printNCharacterMessage(blePacketSize, "SPin was null");
        exit(0);
      }

      firstTime0 = 1;
      firstTime1 = 1;
      firstTime2 = 0;
    }

    // Serial buffer is only 64 bytes, so we have to do processing byte by byte rather than waiting for all bytes to come in first
    while (Serial.available() > 0) {
      incoming_packet_bytes[currentPacketIndex] = Serial.read();
      currentPacketIndex++;
    }

    if (currentPacketIndex == currentPacketLength) {

      printNCharacterMessage(blePacketSize, "finished receiving packet");

      // echo the received array back to the android phone for debugging purposes
      //printReceivedArray(incoming_packet_bytes);

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
        printNCharacterMessage(blePacketSize, "intrst_bad_decode");
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
        printNCharacterMessage(blePacketSize, "got read interest");

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
          printNCharacterMessage(blePacketSize, "dataEncodeErrorNoSig");

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
          printNCharacterMessage(blePacketSize, "dataEncodeErrorSig");

        // the '~' character indicates to BLEApp3 that we are sending it a data packet; the '-' character
        // indicates an interest packet, you can see that in the commented out code below
        sendByteArrayOverBLE(encoding, encodingLength, '~');

        // the code below was just to test sending an interest from the arduino to BLEApp3
        /*
          delay(2000);

          writeNBytes(blePacketSize);

          ndn_NameComponent tempInterestNameComponents[10];
          struct ndn_ExcludeEntry tempExcludeEntries[9];

          InterestLite interest
          (tempInterestNameComponents, sizeof(tempInterestNameComponents) / sizeof(tempInterestNameComponents[0]),
          tempExcludeEntries, sizeof(tempExcludeEntries) / sizeof(tempExcludeEntries[0]), 0, 0);

          NameLite interestName(tempInterestNameComponents, sizeof(tempInterestNameComponents));
          interestName.append("boop");
          interestName.append("test");

          interest.setName(interestName);

          uint8_t tempStore[120];
          size_t temp_encoding_length;
          DynamicUInt8ArrayLite buffer(tempStore, sizeof(tempStore), 0);
          size_t tempSignedPortionBeginOffset, tempSignedPortionEndOffset;
          Tlv0_2WireFormatLite::encodeInterest(interest, &tempSignedPortionBeginOffset, &tempSignedPortionEndOffset, buffer, &temp_encoding_length);
          printNCharacterMessage(blePacketSize, "intrst len:");
          printUnsignedInt(temp_encoding_length);
          //sendByteArrayOverBLE(tempStore, temp_encoding_length, '-');
        */
      }

      currentPacketLength = 0;
      currentReadMode = 0;
      currentPacketIndex = 0;

      free(incoming_packet_bytes);
    }
  }

}

void sendByteArrayOverBLE (byte *arr, unsigned int arraySize, unsigned char c) {

  for (int i = 0; i < blePacketSize; i++) {
    Serial.write(c);
  }
  
  writeUnsignedIntAsBytes(arraySize);
  writeNBytes(18);

  // this is the extra amount of bytes we'll have to send to reach the 20 byte boundary (the ble packet size)
  // since the ble module only sends data out once its buffer gets to 20 bytes (the ble packet size)
  int leftOverCount = blePacketSize - (arraySize % blePacketSize);

  for (int i = 0; i < arraySize; i++) {
    Serial.write(arr[i]);
  }

  for (int i = 0; i < leftOverCount; i++) {
    Serial.write(32);
  }
}

void writeUnsignedIntAsBytes(unsigned int num) {
  byte arraySizeShort[2];

  Serial.write(((num >> 8) & 0xff));
  Serial.write(num & 0xff);
}

void printNCharacterMessage(byte length, String message) {

  int messageSize = message.length();

  if (messageSize > length) {
    Serial.print("printNChar error   /");
    return;
  }

  String finalMessage = message;

  for (int i = 0; i < length - messageSize; i++) {
    finalMessage += " ";
  }

  if (messageSize != length) {
    finalMessage[length - 1] = '/';
  }

  Serial.print(finalMessage);

}

// function used to echo the received array back to the sender for testing purposes
void printReceivedArray(byte *arr) {

  for (int i = 0; i < currentPacketIndex; i++) {

    String begMessage = String("recvd ") + i + ": " + arr[i];

    printNCharacterMessage(blePacketSize, begMessage);
  }

}

void printUnsignedInt(unsigned int &num) {
  if (num > 1000) {
    Serial.print("printUnsign too big ");
  }

  char intStr[4];
  itoa(num, intStr, 10);
  String str = String(intStr);
  printNCharacterMessage(blePacketSize, str);
}

void writeNBytes(int n) {
  for (int i = 0; i < n; i++) {
    Serial.write(32);
  }
}

