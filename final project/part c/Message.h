#pragma once

#include <stdio.h>
#include <string.h>
#include <iostream>

using namespace std;


struct Message {
    int msg_id = 0;
    int src_id = 0;
    int dest_id = 0;
    int num_trailing_msg = 0;
    int func_id = 0;
    const char* payload = "";
};


/* In all of the following functions we assume that the network is relatively small, 
meaning there are no more than 9999 nodes / messages */


/* get 4 chars (bytes) and return the int */
int bytesToInt(char a, char b, char c, char d) {
    char num_chars[] = { a, b, c, d }; 
    string num_str(num_chars);
    return stoi(num_str);
}


/* add zeros at the beginning of the number, according to it's length */
void addZero(string& s, int i) {
    if (i < 10) {
        s += "000";
    }
    else if (i < 100) {
        s += "00";
    }
    else if (i < 1000) {
        s += "0";
    }
}


/* get a message and make a string that represents it (to send it between nodes) */
string make_str_msg (Message msg) {
    string bytes;
    int int_fields[5] = {msg.msg_id, msg.src_id, msg.dest_id, msg.num_trailing_msg, msg.func_id};
    for(int i = 0; i < 5; i++) {
        addZero(bytes, int_fields[i]);
        bytes += to_string(int_fields[i]);
    }
    string payload_str(msg.payload);
    if (payload_str.length() != 0) {  // payload is not empty
        bytes += msg.payload;
    }
    return bytes;
}