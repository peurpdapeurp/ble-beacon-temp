package com.edwardlu.bleapp3;

import java.util.logging.Handler;

public class SendingPacketState {

    byte[] currentSendingByteArray = {};
    boolean sendingOutFragmentsMode = false;
    int fragmentOffset = 0;

    byte[] currentInterestByteArrayToSend = {};
}
