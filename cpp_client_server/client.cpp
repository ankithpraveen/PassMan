#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <winsock2.h>

#ifdef _WIN32
#include <windows.h>
#endif

#pragma comment(lib, "ws2_32.lib")

using namespace std;

//void addPassword(SOCKET serverSocket, const string& website, const string& username, const string& password);
void addPassword(SOCKET serverSocket);
void retrievePassword(SOCKET serverSocket);
void viewAllPasswords(SOCKET serverSocket);
void deletePassword(SOCKET serverSocket);
string getInput(const string& prompt);
unsigned int getChoice(const string& prompt);
string getPassword(const string& prompt);
string generateRandomPassword();

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Failed to initialize winsock." << endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Failed to create socket." << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8888);  // Use the same port as the server
    //inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cerr << "Error in connect function." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Simulate adding a password

    //addPassword(clientSocket, "example.com", "ankith", "StrongPassword123");

    // Implement other password manager operations here

    while (true) {
        cout << "1. Add Password" << endl;
        cout << "2. Retrieve Password" << endl;
        cout << "3. View All Passwords" << endl;
        cout << "4. Delete Password" << endl;
        cout << "5. Exit" << endl;

        unsigned int choice = getChoice("Enter your choice: ");
        int op1;

        switch (choice) {
            case 1:
                addPassword(clientSocket);
                break;
            case 2:
                retrievePassword(clientSocket);
                break;
            case 3:
                viewAllPasswords(clientSocket);
                break;
            case 4:
                deletePassword(clientSocket);
                break;
            case 5:
                op1 = 5;
                send(clientSocket, reinterpret_cast<const char*>(&op1), sizeof(op1), 0);
                cout << "Exiting password manager. Goodbye!" << endl;
                closesocket(clientSocket);
                WSACleanup();
                exit(EXIT_SUCCESS);
            default:
                cout << "Invalid choice. Please try again." << endl;
        }
    }

    return 0;
}

void addPassword(SOCKET serverSocket) {
    // Send website, username, and password to the server
    int op = 1;
    send(serverSocket, reinterpret_cast<const char*>(&op), sizeof(op), 0);
    string website = getInput("Enter the website or app name: ");
    string username = getInput("Enter the username: ");
    string password;

    cout << "Choose how to set the password:" << endl;
    cout << "1. Enter Password Manually" << endl;
    cout << "2. Generate Random Password" << endl;
    unsigned int choice = getChoice("Enter your choice: ");
    switch (choice) {
        case 1: {
            password = getPassword("Enter the password: ");
            break;
        }
        case 2: {
            password = generateRandomPassword();
            break;
        }
        default:
            cout << "Invalid choice. Please try again." << endl;
    }

    string dataToSend = website + "|" + username + "|" + password;
    char response[100];
    memset(response, 0, sizeof(response));
    send(serverSocket, dataToSend.c_str(), dataToSend.size(), 0);
    recv(serverSocket, response, sizeof(response), 0);
    string encrypass(response);
    cout << "Encrypted Password is: " << encrypass << endl;
    recv(serverSocket, response, sizeof(response), 0);
    cout << response << endl;
}

void retrievePassword(SOCKET serverSocket) {
    int op = 2;
    char response[100];
    send(serverSocket, reinterpret_cast<const char*>(&op), sizeof(op), 0);
    string website = getInput("Enter the website or app name: ");
    string username = getInput("Enter the username: ");
    string compositeKey = website + "|" + username;
    cout << compositeKey.c_str() << endl;
    send(serverSocket, compositeKey.c_str(), compositeKey.size(), 0);
    recv(serverSocket, response, sizeof(response), 0);
    string retrpass(response);
    // Find the position of the null terminator in the received data
    size_t nullTerminatorPos = retrpass.find('\0');

    if (nullTerminatorPos != string::npos) {
        // If a null terminator is found, only print the characters before it
        retrpass = retrpass.substr(0, nullTerminatorPos);
    }

    cout << "Credentials for " << compositeKey << " are: Username: " << username << ", Password: " << retrpass << endl;

}

void viewAllPasswords(SOCKET serverSocket) {
    int op = 3;  // Operation code for viewing all passwords
    send(serverSocket, reinterpret_cast<const char*>(&op), sizeof(op), 0);

    char response[1024];
    memset(response, 0, sizeof(response));
    bool check = true;
    int i = 1;
    while (check) {
        recv(serverSocket, response, sizeof(response), 0);
        string res(response);
        //cout << i;

        // Check for the end signal
        if (res.find("End Of Passwords") != std::string::npos) {
            cout << response << endl;
            break;
            //check = false;
        }

        // Print the password entry
        cout << response;
        i++;
        // Clear the response buffer for the next iteration
        memset(response, 0, sizeof(response));
        //cout << "ab" << i;
        //cout << check;
    }
    // Inform the server that the client has received all password entries
    //cout << "here2";
    const char* clientAck = "ClientReceivedPasswords";
    send(serverSocket, clientAck, strlen(clientAck), 0);
}

void deletePassword(SOCKET serverSocket) {
    int op = 4;
    send(serverSocket, reinterpret_cast<const char*>(&op), sizeof(op), 0);

    string website = getInput("Enter the website or app name: ");
    string username = getInput("Enter the username: ");

    string compositeKey = website + "|" + username;
    send(serverSocket, compositeKey.c_str(), compositeKey.size(), 0);

    char response[100];
    recv(serverSocket, response, sizeof(response), 0);
    string retrpass(response);
    cout << retrpass << endl;

}


string generateRandomPassword() {
    const string characterSet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    string password;

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    default_random_engine generator(seed);

    for (int i = 0; i < 16; i++) {
        int randomIndex = generator() % characterSet.length();
        password += characterSet[randomIndex];
    }

    cout << "Your random password is: " << password << endl;
    return password;
}


string getInput(const string& prompt) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);

        if (!input.empty()) {
            return input;
        } else {
            cout << "Invalid input. Please enter a non-empty string." << endl;
        }
    }
}

unsigned int getChoice(const string& prompt) {
    while (true) {
        cout << prompt;
        unsigned int choice;
        cin >> choice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a valid number." << endl;
        } else {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return choice;
        }
    }
}

string getPassword(const string& prompt) {
    while (true) {
        cout << prompt;
        string password;
        getline(cin, password);

        if (!password.empty()) {
            return password;
        } else {
            cout << "Invalid input. Please enter a non-empty password." << endl;
        }
    }
}
