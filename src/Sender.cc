/*
 * Sender.cc
 *
 *  Created on: May 4, 2025
 *      Author: Kenzy
 */
#include "Sender.h"
#include <fstream>
#include <sstream>
Define_Module(Sender);
void Sender::initialize() {
    currentFrameId = 0;
    totalTransmissions = 0;
    timeout = par("to");
    delayTime = par("td");
    startTime = par("st");
    timeoutEvent = new cMessage("timeout");
    readMessages();
    scheduleAt(startTime, new cMessage("start"));
}

void Sender::readMessages() {
    std::ifstream infile(par("inputFile").stringValue());
    std::string line;
    while (std::getline(infile, line)) {
        Frame f;
        f.id = currentFrameId++;
        f.sendTime = simTime();
        f.type = 0;
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        f.errorFlags = std::bitset<4>(prefix);
        std::getline(iss, f.payload);
        f.payload = f.payload.substr(1);

        ByteStuffing(f.payload);
        f.parity = Parity(f.payload);
        messageQueue.push_back(f);
    }
}

char Sender::Parity(const std::string& data) {
    int ones = 0;
    for (char c : data) {
        std::bitset<8> b(c);
        ones += b.count();
    }
    return (ones % 2 == 0) ? 0 : 1;
}

void Sender::ByteStuffing(std::string& data) {
    std::string stuff = "$";
    for (char c : data) {
        if (c == '$' || c == '/') {
            stuff += '/';
        }
        stuff += c;
    }
    stuff += "$";
    data = stuff;
}

void Sender::NoisyChannel(Frame f) {
    if (f.errorFlags[0]) {
           int byteIndex = intuniform(0, f.payload.size() - 1);
           int bitIndex = intuniform(0, 7);
           f.payload[byteIndex] ^= (1 << bitIndex);
           EV << "[MODIFY] Bit flipped at payload byte " << byteIndex << "\n";
       }

    simtime_t sendAt = simTime();
    if (f.errorFlags[1]) {
           Frame dup = f;
           sendFrame(dup, sendAt + 0.1);
       }

        if (f.errorFlags[2]) sendAt += delayTime;

    if (f.errorFlags[3]) {
        EV << "[LOSS] Frame ID=" << f.id << " was dropped.\n";
        return;
    }
    sendFrame(f, sendAt);

}

void Sender::sendFrame(Frame& f, simtime_t time) {
    cMessage* msg = new cMessage("frame");
    msg->addPar("id") = f.id;
    msg->addPar("type") = f.type;
    msg->addPar("data") = f.payload.c_str();
    msg->addPar("parity") = f.parity;
    scheduleAt(time, msg);
    totalTransmissions++;
    EV << "[SEND] Frame ID=" << f.id << " at time=" << time << "\n";
}

void Sender::handleMessage(cMessage* msg) {
    if (msg == timeoutEvent) {
        EV << "[TIMEOUT] Retransmitting...\n";
        Frame f = messageQueue.front();
        NoisyChannel(f);
        scheduleAt(simTime() + timeout, timeoutEvent);
    } else if (strcmp(msg->getName(), "start") == 0) {
        Frame f = messageQueue.front();
        NoisyChannel(f);
        scheduleAt(simTime() + timeout, timeoutEvent);
    } else {
        int ackId = msg->par("id");
        if (ackId == messageQueue.front().id) {
            messageQueue.erase(messageQueue.begin());
            cancelEvent(timeoutEvent);
            if (!messageQueue.empty()) {
                Frame f = messageQueue.front();
                NoisyChannel(f);
                scheduleAt(simTime() + timeout, timeoutEvent);
            }
        }
        delete msg;
    }
}

void Sender::finish() {
    cancelAndDelete(timeoutEvent);
}



