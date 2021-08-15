#pragma once

#include <iostream>
#include <vector>
// #include "Message.h"

using namespace std;


/* for the switch-case on do_command */
Command hashit (std::string const& inString) {
    string first5 = inString.substr(0,5);
    if (inString == "setid") return _setid;
    if (inString == "connect") return _connect;
    if (inString == "send") return _send;
    if (first5 == "route") return _route;
    if (first5 == "peers") return _peers;
    return _illegle_command;
}


/* get the index of an element in vector by the id of the node */
int getIndexByID(vector<list<string>> v, int id) {
    int id2; 
    for (int i = 0; i < v.size(); i++) {
        id2 = stoi(v[i].front());  // the id is the first one on the list
        if (id == id2) {
            return i;
        }
    }
    return -1;
}


int getIndexByVal(vector<int> v, int val) {
    for (int i = 0; i < v.size(); i++) {
        if (v[i] == val) {
            return i;
        }
    }
    return -1;
}


/* to not send a discover message to nodes that already got it */
bool isInPayload(string payload_str, int id) {
    const char* payload = payload_str.c_str();
    int next;
    for (int i = 4; i < payload_str.size(); i+=4) {  // start with 4, cause the first 4 bytes are destID
        next = bytesToInt(payload[i], payload[i+1], payload[i+2], payload[i+3]); 
        if (next == id) {
            return true; 
        }
    }
    return false;
}


string create_connect_payload(int port, char* IP) {
    string port_bytes, ip_len_bytes;
    addZero(port_bytes, port);
    port_bytes += to_string(port);
    string ip(IP);
    addZero(ip_len_bytes, ip.length());
    ip_len_bytes += to_string(ip.length());
    return port_bytes+ip_len_bytes+ip;
}


string create_relay_payload(int num_msgs, int len, string message, string path) {
    string num, length; 
    addZero(num, num_msgs-1);  // the num of next relay messages
    num += to_string(num_msgs-1);
    addZero(length, len);  // the num of next relay messages
    length += to_string(len);
    string concat = num+length+message+path;
    return concat; 
}