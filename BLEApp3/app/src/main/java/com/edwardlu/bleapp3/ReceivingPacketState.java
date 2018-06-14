package com.edwardlu.bleapp3;

import java.util.logging.Handler;

public class ReceivingPacketState {

    boolean receivingLengthMode = false;
    boolean receivingFragmentsMode = false;
    int currentReceivingPacketLength = 0;
    byte[] currentReceivedByteArray = {};
    int currentReceivedFragmentOffset = 0;
    // 0 means receiving data, 1 means receiving interest
    int receivingDataOrInterest = 0;

    byte[] copyOfLastReceivedByteArray = {};

}
