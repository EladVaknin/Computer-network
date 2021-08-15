#include "Node.h" 
#include "Message.h"
#include "HelperFunctions.h"

using namespace std;


/* open a socket, listen to inputs */
void Node::listen_to_inputs() {
    int ret, opt = 1;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);  
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(this->Port);
    // Forcefully attaching socket to the port
    if (bind(listenfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("adding fd1(%d) to monitoring\n", listenfd);
    add_fd_to_monitoring(listenfd);
    if (listen(listenfd, 10) < 0) {  // 10 = the max length to which the queue of pending connections for sockfd may grow
        perror("listen");
        exit(EXIT_FAILURE);
    }
    vector<int> disconnected = {};
    bool flag = true;
    while(1) {
        Function response;
        if (flag) printf("waiting for input...\n");
	    ret = wait_for_input();
        if (count(disconnected.begin(), disconnected.end(), ret) || (ret == -1)) {  // the socket is irrelevant
            flag = false; 
            continue;
        }
        flag = true;
	    printf("fd: %d is ready. reading...\n", ret);
	    int valread = read(ret, buff, SIZE);
        if (ret == 0) {  // is a command from the keyboard
            // cout << "keyboard, buff = " << buff << endl;
            response = do_command(buff);
            memset(buff, 0, sizeof buff);
        }
        else if (ret == listenfd) {  // someone wants to connect me  
            // cout << "listenfd, buff = " << buff << endl;
            int addrlen = sizeof(my_addr);
            if ((new_sock = accept(listenfd, (struct sockaddr*)&my_addr, (socklen_t*)&addrlen)) < 0) {
                response = Nack;
            }
            printf("adding fd1(%d) to monitoring\n", new_sock);
            add_fd_to_monitoring(new_sock);
            response = Ack;
            memset(buff, 0, sizeof buff);
        }
        else {  // another message (in the given form, start with id)
            // cout << "else, buff = " << buff << endl;
            if (valread == 0) {  // check if the node has disconnected
                int index = getIndexByVal(sockets, ret);
                disconnect(index);
                disconnected.push_back(ret); 
                response = Ack;
            } 
            else {
                response = check_msg(buff, ret);
                memset(buff, 0, sizeof buff);
            }
        }
        if(response == Ack) printf("Ack\n");
        else printf("Nack\n");  
    }
}


void Node::disconnect(int index) {
    cout << "some node has disconnected" << endl;
    vector<int>::iterator it = sockets.begin();
    sockets.erase(it+index);
    auto neighbor = neighbors[index];
    int id = stoi(neighbor.front());
    vector<list<string>>::iterator it1 = neighbors.begin();
    neighbors.erase(it1+index);
    vector<vector<int>>::iterator it2 = paths.begin();
    for(int i = 0; i < paths.size(); i++) {
        if(paths[i].back() == id) {  // this is a path to the disconnected node
            paths.erase(it2+i);
        }
    }
}


Function Node::open_tcp_socket(const char* ip, int port) {
    if (strcmp(ip, this->IP) && (port == this->Port)) {
        printf("Can not connect yourself"); 
        return Nack; 
    }
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock < 0) {
        return Nack;
    }
    printf("Successfully open a TCP socket.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    // server_addr.sin_addr.s_addr = inet_addr(ip);

    if(inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        printf("\nInvalid address or port / Address not supported \n");
        return Nack;
    }
    return Ack; 
}


Function Node::do_command(string command) {

    size_t pos = command.find(",");
    size_t pos2;
    string command_name = command.substr(0, pos);
    string info = command.substr(pos+1);  // after the ","
    char* char_arr;

    string err_msg = "Wrong command syntax, please try again";

    switch (hashit(command_name)) {
    case _setid:
        try {
            setID(stoi(info));  // info contains the id only
            return Ack;
        }
        catch(const std::exception& e) {
            cout << err_msg << endl;
            return Nack;
        }
    case _connect:
        {
            try {
                pos2 = info.find(":");
                string ip = info.substr(0, pos2);
                char_arr = &ip[0];
                string port = info.substr(pos2+1); 
                if (open_tcp_socket(char_arr, stoi(port)) == Nack) {
                    return Nack; 
                }
                return myconnect();  
            }
            catch(const std::exception& e) {
                cout << err_msg << endl;
                return Nack;
            }
        }
    case _send:
        {
            try {
                pos = info.find(",");
                int dest_id = stoi(info.substr(0, pos));  // destination node id
                info = info.substr(pos+1);  // override info
                pos = info.find(",");
                int len = stoi(info.substr(0, pos));
                string message = info.substr(pos+1);
                if (len != message.length()-1) {
                    return Nack;
                }
                vector<int> path = getPath(dest_id); 
                if (path.empty())  // no path has found
                    return Nack;
                int num_relay = path.size()-2; 
                string payload; 
                for (int i = 1; i < path.size(); i++) {  // convert the path to string for payload
                    string bytes;                        // starting with 1 (next node)
                    addZero(bytes, path[i]); 
                    bytes += to_string(path[i]);
                    payload += bytes;
                }
                int next = path.at(1);  // the first node in the path after me
                if (next != dest_id) {  
                    return relay(next, num_relay, dest_id, len, message, payload);
                }
                else {  // the destination is a neighbor of mine
                    return mysend(dest_id, len, message); 
                }
            } 
            catch (const std::exception& e) {
                cout << err_msg << endl;
                return Nack;
            }
        }
    case _route:
        try {
            return route(stoi(info));  // info contains the id only
        }
        catch (const std::exception& e) {
            cout << err_msg << endl;
            return Nack;
        }
    case _peers:
        try { 
            return peers();
        }
        catch (const std::exception& e) {
            cout << err_msg << endl;
            return Nack;
        }
    default:
        cout << err_msg << endl;
        return Nack;
    }
}


Function Node::check_msg(string msg, int ret) {
    // cout << "check msg: " << msg << endl;
    string buff_str(buff);
    int src_id = bytesToInt(buff[4], buff[5], buff[6], buff[7]); 
    int dest_id = bytesToInt(buff[8], buff[9], buff[10], buff[11]);  
    int func_id = bytesToInt(buff[16], buff[17], buff[18], buff[19]);  
    if (func_id == Function::Connect) {  // check if its a connect message
        char payload[4];
        for (int i = 0; i < 4; i++) {  // copy msg_id (of the connect msg) to the payload 
            payload[i] = buff[i];      // check it .... &*(%#)
        }
        Function response = Ack;
        struct Message msg = {MSG_ID, this->ID, src_id, 0, response, payload};  // ack ot nack message
        MSG_ID++;
        string str_msg = make_str_msg(msg);
        const char* chars_msg = str_msg.c_str();
        if (send(ret, chars_msg, str_msg.length(), 0) == -1) {
            return Nack;
        }
        // save the id, ip and port of the node
        int src_port = bytesToInt(buff[20], buff[21], buff[22], buff[23]);
        int ip_len = bytesToInt(buff[24], buff[25], buff[26], buff[27]);
        string src_ip = buff_str.substr(28,28+ip_len);
        list<string> l = {to_string(src_id), src_ip, to_string(src_port)};
        this->neighbors.push_back(l);
        this->sockets.push_back(ret);
        cout << "Connected to Node with ID = " << src_id << endl;
        return Ack; 
    } 
    if (func_id == Function::Discover) {  // check if its a discover message
        int i; 
        for(i = 24; i < buff_str.length(); i+=4) {  // move on the payload
            int next = bytesToInt(buff[i], buff[i+1], buff[i+2], buff[i+3]); 
            if(next == src_id) break;
        }
        string payload_str = buff_str.substr(20,i+4); 
        int original_dest = bytesToInt(buff[20], buff[21], buff[22], buff[23]);
        if (this->ID == original_dest) {  // I'm the destination
            string bytes; 
            addZero(bytes, this->ID);  // add myself to the payload
            bytes += to_string(this->ID);
            string concat = payload_str+bytes; 
            const char* payload = concat.c_str();
            int trail = 0;  // compute this ... 
            struct Message msg = {MSG_ID, this->ID, src_id, trail, Function::Route, payload}; 
            MSG_ID++;
            string str_msg = make_str_msg(msg);
            const char* chars_msg = str_msg.c_str();
            send(ret, chars_msg, str_msg.length(), 0);
            return Ack;
        }
        return discover(original_dest, src_id, payload_str);
    }
    if (func_id == Function::Send) {  
        Function response;
        if (dest_id != this->ID) {  // the message is not for me 
            response = Nack;
        }
        else {
            response = Ack;
            int len = bytesToInt(buff[20], buff[21], buff[22], buff[23]);  // first 4 bytes on payload
            cout << "len is: " << len << endl;
            // string send_msg = buff_str.substr(24,24+len);
            string send_msg = buff_str.substr(24,buff_str.size()); 
            size_t pos = send_msg.find("0");
            string pure_msg = send_msg.substr(0, pos);
            cout << "new message from socket " << ret << " : " << pure_msg << endl;
        }
        struct Message msg = {MSG_ID, this->ID, src_id, 0, response, ""};  // ack ot nack message
        MSG_ID++;
        string str_msg = make_str_msg(msg);
        const char* chars_msg = str_msg.c_str();
        if (send(ret, chars_msg, str_msg.length(), 0) == -1) {
            return Nack;
        }
        return Ack; 
    }
    if (func_id == Function::Relay) { 
        int num_next_relay = bytesToInt(buff[20], buff[21], buff[22], buff[23]);
        int msg_len = bytesToInt(buff[24], buff[25], buff[26], buff[27]);
        string message = buff_str.substr(28,28+msg_len); 
        int len = buff_str.length();
        string path_bytes = buff_str.substr(28+msg_len,len);
        int dest = bytesToInt(buff[len-4], buff[len-3], buff[len-2], buff[len-1]);
        struct Message msg = {MSG_ID, this->ID, src_id, 0, Function::Ack, ""};
        MSG_ID++;
        string str_msg = make_str_msg(msg);
        const char* chars_msg = str_msg.c_str();
        if (send(ret, chars_msg, str_msg.length(), 0) == -1) {
            return Nack;
        }
        if (num_next_relay == 0) {  // the next node is the destination
            return mysend(dest, msg_len, message);
        }
        string path = buff_str.substr(28+msg_len+4,len);  // cut my id from payload
        string next = path.substr(0,4);
        const char* next_bytes = next.c_str();
        int nextID = bytesToInt(next_bytes[0], next_bytes[1], next_bytes[2], next_bytes[3]);
        return relay(nextID, num_next_relay, dest, msg_len, message, path);
    }
    return Nack;
}


Function Node::myconnect() {
    int e = connect(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(e == -1) {
        return Nack;
    }
    printf("adding fd1(%d) to monitoring\n", server_sock);
    add_fd_to_monitoring(server_sock);
    string payload_str = create_connect_payload(this->Port, this->IP); 
    const char* payload = payload_str.c_str();
    struct Message msg = {MSG_ID, this->ID, 0, 0, Function::Connect, payload};  
    MSG_ID++;
    string str_msg = make_str_msg(msg);
    const char* chars_msg = str_msg.c_str();
    if (send(server_sock, chars_msg, str_msg.length(), 0) == -1) {
        perror("send");
    }
    int valread = read(server_sock, buff, SIZE);
    // cout << "the message is : " << buff << endl;
    int func_id = bytesToInt(buff[16], buff[17], buff[18], buff[19]); 
    if (func_id == Function::Ack) {  // check if its an Ack message
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &server_addr.sin_addr, ip, sizeof(ip));
        int port = ntohs(server_addr.sin_port);
        int src_id = bytesToInt(buff[4], buff[5], buff[6], buff[7]);
        // cast all to string and insert to the list
        string str_ip(ip);
        list<string> l = {to_string(src_id), str_ip, to_string(port)};
        this->neighbors.push_back(l);
        this->sockets.push_back(server_sock);
        cout << "Connected to Node with ID = " << src_id << endl;
        return Ack;
    }
    return Nack;
}


vector<int> Node::getPath(int destID) {
    for(vector<int> &path : paths) {
        if(path.back() == destID)  // this is a path to the destination node
            return path;
    }
    // a path to destination does not exist yet
    string bytes;
    addZero(bytes, destID);
    bytes += to_string(destID);  // add the destID to the 4 first bytes on payload
    if(discover(destID, -1, bytes) == Ack) 
        return paths.back();  // the last path is the last one that added 
    else 
        return vector<int>();
}


void Node::addThePath(int destID, string buff) {
    vector<int> path = {};
    for(int i = 24; i < buff.length(); i+=4) {  // move on the payload
        int next = bytesToInt(buff[i], buff[i+1], buff[i+2], buff[i+3]);
        path.push_back(next);
        if(next == destID) break;
    }
    paths.push_back(path);
}


Function Node::discover(int destID, int father, string payload_str) {
    string bytes;
    addZero(bytes, this->ID);
    bytes += to_string(this->ID);  
    payload_str += bytes;  // add myself to the payload (path)
    // send a discover message to all the neighbors (that are not in the payload)
    int i = 0;
    bool neigExists = false;
    int destIndx = getIndexByID(neighbors, destID); 
    if (destIndx != -1) {  // the destination is a neighbor of this node
        i = destIndx;  // start there to find the shortest path
    }
    while (i < neighbors.size()) {  // neighbor is a list {id,ip,port}, all strings
        auto neighbor = neighbors[i];
        int neig_id = stoi(neighbor.front());  // the id is the first one on the list
        if (isInPayload(payload_str, neig_id)) {
            continue;
        }
        neigExists = true;  // if we've ever got here, change to true
        int trial = ceil(payload_str.length()/492.0);  // the number of message (pieces) to send 
        for (unsigned j = 0; j < payload_str.length(); j += 492) {
            string str = payload_str.substr(j, 492);
            const char* payload = str.c_str();  
            struct Message msg = {MSG_ID, this->ID, neig_id, trial-1, Function::Discover, payload};
            MSG_ID++;
            string str_msg = make_str_msg(msg);
            const char* chars_msg = str_msg.c_str();
            if (send(sockets[i], chars_msg, str_msg.length(), 0) == -1) {  // send a discover message to the neighbor
                perror("send");
            }
        }
        read(sockets[i], buff, SIZE);
        int func_id = bytesToInt(buff[16], buff[17], buff[18], buff[19]); 
        if (func_id == Function::Route) {  // check if its a route message --> path has found!
            int original_src = bytesToInt(buff[24], buff[25], buff[26], buff[27]);
            string buff_str(buff);
            string str = buff_str.substr(20,SIZE);
            const char* payload = str.c_str();  
            if (this->ID == original_src) {  // I'm the original source node
                addThePath(destID, buff);
                return Ack;
            } 
            else {
                // send route to the node who sent me the discover msg
                int trial = 0; 
                struct Message msg = {MSG_ID, this->ID, father, trial, Function::Route, payload};
                MSG_ID++;
                string str_msg = make_str_msg(msg);
                const char* chars_msg = str_msg.c_str();
                int index = getIndexByID(neighbors, father);
                send(sockets[index], chars_msg, str_msg.length(), 0);
                return Ack;
            }
        }
        i++;
    }
    if (!neigExists) {  // no neighbors
        struct Message msg = {MSG_ID, this->ID, father, 0, Function::Nack, ""};
        MSG_ID++;
        string str_msg = make_str_msg(msg);
        const char* chars_msg = str_msg.c_str();
        int index = getIndexByID(neighbors, father);
        send(sockets[index], chars_msg, str_msg.length(), 0);  // sending Nack to my father
    }
    return Nack; 
}


/* send a message between neighbors */
Function Node::mysend(int dest, int len, string message) {
    string bytes; 
    addZero(bytes, len);  // add myself to the payload
    bytes += to_string(len);
    string concat = bytes+message;
    const char* payload = concat.c_str();
    struct Message msg = {MSG_ID, this->ID, dest, 0, Function::Send, payload};  
    MSG_ID++;
    string str_msg = make_str_msg(msg);
    const char* chars_msg = str_msg.c_str();
    int index = getIndexByID(neighbors, dest); 
    int dest_sock = sockets[index];
    if (send(dest_sock, chars_msg, str_msg.length(), 0) == -1) {
        perror("send");
    }
    int valread = read(dest_sock, buff, SIZE);
    int func_id = bytesToInt(buff[16], buff[17], buff[18], buff[19]); 
    if (func_id == Function::Ack) {  // check if its an Ack message
        return Ack;
    }
    return Nack;
}


Function Node::relay(int nextID, int num_msgs, int destID, int len, string message, string path) {
    string message2 = message.substr(0,len);
    string concat = create_relay_payload(num_msgs, len, message2, path);
    const char* payload = concat.c_str();
    int trial = 0;  // compute it ....
    struct Message msg = {MSG_ID, this->ID, nextID, trial, Function::Relay, payload};  
    MSG_ID++;
    string str_msg = make_str_msg(msg);
    const char* chars_msg = str_msg.c_str();
    int index = getIndexByID(neighbors, nextID); 
    if (index == -1) {  // asked to send a message to a node that is not his neighbor
        return Nack;
    }
    int dest_sock = sockets[index];
    if (send(dest_sock, chars_msg, str_msg.length(), 0) == -1) {
        perror("send");
    }
    int valread = read(dest_sock, buff, SIZE);
    int func_id = bytesToInt(buff[16], buff[17], buff[18], buff[19]); 
    if (func_id == Function::Ack) {  // check if its an Ack message
        return Ack;
    }
    return Nack;
}


Function Node::peers() {
    if (neighbors.size() == 0) {
        cout << "no neighbors yet" << endl;
        return Ack; 
    }
    int n = neighbors[0].size();
    if (n != 0) {
        for(int i = 0; i < neighbors.size()-1; i++) {  
            auto neighbor = neighbors[i];
            int neig_id = stoi(neighbor.front());  // the id is the first one from the id list
            cout << neig_id  << ",";
        }
        auto neighbor = neighbors[neighbors.size()-1];
        int neig_id = stoi(neighbor.front());  // the id is the first one from the id list
        cout << neig_id << endl;
        return Ack;
    }
    return Nack;
}


Function Node::route(int node_id) {
    for (int i = 0; i < paths.size(); i++) {  // over on the vectors inside the vectors and check whit itaretor if node_id contains in the paths
        auto path = paths[i]; 
        if (path[path.size()-1] != node_id) {  // it's not a path to node_id
            continue;
        }
        for (int j = 0; j < path.size()-1; j++) {
            int way = path[j];
            std::cout << way  << "->";
        }  
        int way = path[path.size()-1];
        std::cout << way << endl;
        return Ack;
    }
    cout << "No route found" << endl;
    return Nack;
}

