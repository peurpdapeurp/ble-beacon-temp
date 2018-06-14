
#define STR1(x) #x
#define STR2(x) STR1(x)
#define CONCAT(x, y) x ## y

// This makes a string for #include from the NDN-CPP root directory.
// It should include the absolute path up to ndn-cpp, except put "ndn-c"
// instead of "ndn-cpp". Use it like this:
// #include NDN_CPP_ROOT(pp/c/name.c)
// We split "ndn-cpp" into "ndn-c" and "pp" because the Arduino compiler won't
// accept NDN_CPP_SRC(/src/c/name.c) with a starting slash.
// We have to use an absolute path because the Arduino compiler won't
// include a relative file.
#define NDN_CPP_ROOT(x) STR2(CONCAT(/home/edward/Desktop/test-ndn-c, x))








  /*
    // manually coded loop to send out the data packet, just left it here in case it's needed for some reason again
    char * arrayToSend1;
    int currentSendIndex = 0;

    arrayToSend1 = (char *) malloc(blePacketSize);

    for (int i = 0; i < blePacketSize; i++) {
    arrayToSend1[i] = arr[currentSendIndex++];
    }

    while (! RFduinoBLE.send(arrayToSend1, blePacketSize));

    char * arrayToSend2;

    arrayToSend2 = (char *) malloc(blePacketSize);

    for (int i = 0; i < blePacketSize; i++) {
    arrayToSend2[i] = arr[currentSendIndex++];
    }

    while (! RFduinoBLE.send(arrayToSend2, blePacketSize));

    char * arrayToSend3;

    arrayToSend3 = (char *) malloc(blePacketSize);

    for (int i = 0; i < blePacketSize; i++) {
    arrayToSend3[i] = arr[currentSendIndex++];
    }

    while (! RFduinoBLE.send(arrayToSend3, blePacketSize));

    char * arrayToSend4;

    arrayToSend4 = (char *) malloc(blePacketSize);

    for (int i = 0; i < blePacketSize; i++) {
    arrayToSend4[i] = arr[currentSendIndex++];
    }

    while (! RFduinoBLE.send(arrayToSend4, blePacketSize));

    char * arrayToSend5;

    arrayToSend5 = (char *) malloc(blePacketSize);

    for (int i = 0; i < blePacketSize; i++) {
    arrayToSend5[i] = arr[currentSendIndex++];
    }

    while (! RFduinoBLE.send(arrayToSend5, blePacketSize));

    char * arrayToSend6;

    arrayToSend6 = (char *) malloc(blePacketSize);

    for (int i = 0; i < blePacketSize; i++) {
    arrayToSend6[i] = arr[currentSendIndex++];
    }

    while (! RFduinoBLE.send(arrayToSend6, blePacketSize));

    char * arrayToSend7;

    arrayToSend7 = (char *) malloc(blePacketSize);
    int lastIndexOfRealData = 0;

    for (int i = 0; i < leftOverCount; i++) {
    arrayToSend7[i] = arr[currentSendIndex++];
    lastIndexOfRealData = i;
    }

    for (int i = lastIndexOfRealData; i < blePacketSize; i++) {
    arrayToSend7[i] = ' ';
    }

    while (! RFduinoBLE.send(arrayToSend7, blePacketSize));
  */

  /*
    // this is the number of blePacketSize byte chunks to split the packet into for sending
    unsigned int numChunks = arraySize / blePacketSize;
    if (arraySize % blePacketSize != 0)
      numChunks++;

    Serial.print("Num Chunks: ");
    Serial.print(numChunks);
    Serial.print("\n");

    // this is the extra amount of bytes for the last chunk we'll have to send to reach the 20 byte boundary (the ble packet size)
    // since the ble module only sends data out once its buffer gets to 20 bytes (the ble packet size)
    unsigned int leftOverCount = blePacketSize - (arraySize % blePacketSize);
    if (arraySize % blePacketSize == 0)
      leftOverCount = 0;

    Serial.print("Leftover Count: ");
    Serial.print(leftOverCount);
    Serial.print("\n");

    char arrayToSend[blePacketSize];
    int currentSendIndex = 0;
    int lastIndexOfRealData = 0;

    for (int i = 0; i < numChunks; i++) {
      for (int j = 0; j < blePacketSize; j++) {
        arrayToSend[j] = arr[currentSendIndex++];
      }

      Serial.print(F("Sending chunk: "));
      Serial.println(i);

      Serial.print("Current send index: ");
      Serial.println(currentSendIndex);

      while (! RFduinoBLE.send(arrayToSend, blePacketSize));
    }

    if (leftOverCount != 0) {
      for (int i = 0; i < leftOverCount; i++) {
        arrayToSend[i] = arr[currentSendIndex++];
        lastIndexOfRealData = i;
      }

      Serial.print("Left over count after sending all but last chunk: ");
      Serial.println(leftOverCount);

      for (int i = lastIndexOfRealData + 1; i < leftOverCount; i++) {
        arrayToSend[i] = ' ';
      }

      Serial.println("Sending last chunk");

      Serial.print("Current send index: ");
      Serial.println(currentSendIndex);
      Serial.print("Current last index of real data: ");
      Serial.println(lastIndexOfRealData);

      while (! RFduinoBLE.send(arrayToSend, blePacketSize));

      Serial.println("Just testing sending another random long message to serial to see if that also causes a crash?");

      Serial.println("After sending last chunk.");
    }
  */
