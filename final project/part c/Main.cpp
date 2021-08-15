#include "Node.cpp"
// #include "select.cpp"


int main() {
    
    // init();

    char buf[20];

    cout << "Welcome! Now build a Node on the Network" << endl;

    cout << "Please Enter Node Port: " << endl;
    fgets(buf, 20, stdin);
    int port = stoi(buf);

    cout << "Please Enter Node IP: " << endl;
    fgets(buf, 20, stdin);
    char* ip = buf;

    Node n(ip, port);  
    return 0;
}