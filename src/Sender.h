/*
 * Sender.h
 *
 *  Created on: May 4, 2025
 *      Author: Kenzy
 */

#ifndef SENDER_H_
#define SENDER_H_

#include <omnetpp.h>
#include <string>
#include <vector>
#include <bitset>

using namespace omnetpp;

class Sender : public cSimpleModule {
  public:
    struct Frame {
        int id;
        simtime_t sendTime;
        int type;
        std::string payload;
        char parity;
        std::bitset<4> errorFlags;
    };

    std::vector<Frame> messageQueue;
    int currentFrameId;
    simtime_t timeout;
    simtime_t delayTime;
    simtime_t startTime;
    int totalTransmissions;
    cMessage* timeoutEvent;

    void readMessages();
    void ByteStuffing(std::string& data);
    char Parity(const std::string& data);
    void NoisyChannel(Frame f);
    void sendFrame(Frame& f, simtime_t time);
    virtual void initialize() override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;
};

#endif /* SENDER_H_ */
