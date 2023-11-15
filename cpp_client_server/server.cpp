#include <iostream>
#include <iomanip>
#include <winsock2.h>
#include <sqlite3.h>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>
using namespace std::chrono;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "sqlite3.lib")

using namespace std;

void clientHandler(SOCKET clientSocket);
void addPassword(sqlite3* connection, SOCKET clientSocket);
void retrievePassword(sqlite3* connection, SOCKET clientSocket);
void viewAllPasswords(sqlite3* connection, SOCKET clientSocket);
void deletePassword(sqlite3* connection, SOCKET clientSocket);
string encryptPassword(const string& credentials);
string decryptPassword(const string& encryptedCredentials);

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Failed to initialize winsock." << endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Failed to create socket." << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8888);  // Choose any available port

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cerr << "Failed to bind socket." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Error in listen function." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server is listening for connections..." << endl;

    std::vector<std::thread> threads;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Error in accept function." << endl;
            continue; // Continue listening for the next connection
        }

        // Create a new thread for each client
        thread clientThread(clientHandler, clientSocket);
        threads.push_back(move(clientThread));
    }

    // Join all threads to wait for them to finish
    for (auto& thread : threads) {
        thread.join();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

void clientHandler(SOCKET clientSocket) {
    sqlite3* connection;
    int result = sqlite3_open("password_manager.db", &connection);

    if (result != SQLITE_OK) {
        cerr << "Failed to open database" << endl;
        closesocket(clientSocket);
        sqlite3_close(connection);
        return; // Exit the thread
    }

    char* errMsg;

    // Create a table to store passwords
        result = sqlite3_exec(
        connection,
        "CREATE TABLE IF NOT EXISTS passwords (website_username TEXT PRIMARY KEY, username TEXT, password TEXT)",
        nullptr,
        nullptr,
        nullptr
    );

        if (result != SQLITE_OK) {
            cerr << "Failed to create passwords table: " << errMsg << endl;
            sqlite3_free(errMsg);
            sqlite3_close(connection);
            closesocket(clientSocket);
            //continue; // Continue listening for the next connection
        }
        bool flag = true;
        while(flag) {
            int choice;
            int bytesReceived = recv(clientSocket, reinterpret_cast<char*>(&choice), sizeof(choice), 0);
            if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "Failed to receive data." << std::endl;
            break;
            }  
            else {
                cout << "choice receiver: " << choice << " ";
                switch (choice) {
                    case 1:
                        addPassword(connection, clientSocket);
                        break;
                    case 2:
                        retrievePassword(connection, clientSocket);
                        break;
                    case 3:
                        viewAllPasswords(connection, clientSocket);
                        break;
                    case 4:
                        deletePassword(connection, clientSocket);
                        break;
                    /*case 5:
                        changeMasterPassword(connection);
                        break;*/
                    case 5:
                        cout << "Exiting password manager. Goodbye!" << endl;
                        flag = false;
                        closesocket(clientSocket);
                        sqlite3_close(connection);
                        break;
                    default:
                        cout << "Invalid choice. Please try again." << endl;
                        break;
                }
            }
        }
}

void addPassword(sqlite3* connection, SOCKET clientSocket) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Receive website, username, and password from the client
    if (recv(clientSocket, buffer, sizeof(buffer), 0) == SOCKET_ERROR) {
        cerr << "Error receiving data from client." << endl;
        return;
    }

    string dataReceived(buffer);

    cout << dataReceived << endl;
    size_t pos = dataReceived.find('|');
    if (pos == string::npos) {
        cerr << "Invalid data format received." << endl;
        return;
    }

    string website = dataReceived.substr(0, pos);
    dataReceived.erase(0, pos + 1);

    pos = dataReceived.find('|');
    if (pos == string::npos) {
        cerr << "Invalid data format received." << endl;
        return;
    }

    string username = dataReceived.substr(0, pos);
    dataReceived.erase(0, pos + 1);

    string password = dataReceived;

    cout << "Received: Website: " << website << ", Username: " << username << ", Password: " << password << endl;

    const string characterSet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    string nonce;
    for (int i = 0; i < 5; i++) {
        char asciiChar = characterSet[rand() % 62];
        nonce += asciiChar;
    }

    // Combine the nonce with the credentials
    string credentials = nonce + password;

    string compositeKey = website + "|" + username;

    // Encrypt the password using RSA
    auto start = high_resolution_clock::now();
    string encryptedPassword = encryptPassword(credentials);
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(stop - start);
    cout << duration.count() << endl;
    send(clientSocket, encryptedPassword.c_str(), encryptedPassword.size(), 0);

    string insertQuery = "INSERT OR REPLACE INTO passwords (website_username, username, password) VALUES ('" + compositeKey + "', '" + username + "', '" + encryptedPassword + "')";
    char* errMsg;
    int result = sqlite3_exec(connection, insertQuery.c_str(), nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK) {
        cerr << "Failed to insert data: " << errMsg << endl;
        sqlite3_free(errMsg);  // Free the allocated error message
        closesocket(clientSocket);
        return;
    }

    // Send a response to the client
    const char* response = "Password added successfully!";
    send(clientSocket, response, strlen(response), 0);
}

void retrievePassword(sqlite3* connection, SOCKET clientSocket) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Receive website, username, and password from the client
    if (recv(clientSocket, buffer, sizeof(buffer), 0) == SOCKET_ERROR) {
        cerr << "Error receiving data from client." << endl;
        return;
    }
    string dataReceived(buffer);
    cout << dataReceived << endl;
    size_t pos = dataReceived.find('|');
    if (pos == string::npos) {
        cerr << "Invalid data format received." << endl;
        return;
    }
    string website = dataReceived.substr(0, pos);
    dataReceived.erase(0, pos + 1);
    string username = dataReceived.substr(0, pos);

    if (std::all_of(website.begin(), website.end(), ::isdigit)) {
        website = "'" + website + "'";
    }
    string searchKey = website + "|" + username;

    char* errMsg;
    pair<string, string> resultPair;
    string query = "SELECT username, password FROM passwords WHERE website_username = '" + searchKey + "'";
    int result = sqlite3_exec(
        connection,
        query.c_str(),
        [](void* data, int argc, char** argv, char** colName) -> int {
            pair<string, string>* resultPair = static_cast<pair<string, string>*>(data);
            resultPair->first = argv[0];
            resultPair->second = argv[1];
            return 0;
        },
        &resultPair,
        &errMsg
    );

    if (result == SQLITE_OK) {
    // Decrypt the password using RSA
    auto start = high_resolution_clock::now();
    string decryptedPasswordWithNonce = decryptPassword(resultPair.second);
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(stop - start);
    cout << duration.count() << endl;

    // Check if the decrypted password is empty before using substr
        if (!decryptedPasswordWithNonce.empty() && decryptedPasswordWithNonce.size() >= 5) {
            // Extract the nonce and credentials
            string nonce = decryptedPasswordWithNonce.substr(0, 5);
            string decryptedPassword = decryptedPasswordWithNonce.substr(5);
            send(clientSocket, decryptedPassword.c_str(), decryptedPassword.size(), 0);
            cout << "Credentials for " << searchKey << " are: Username: " << resultPair.first << ", Password: " << decryptedPassword << endl;
        }
        else {
            string pass = "not found";
            send(clientSocket, pass.c_str(), pass.size(), 0);
        }
    } 
    else {
        string pass = "not found";
        send(clientSocket, pass.c_str(), pass.size(), 0);
    }
}

void viewAllPasswords(sqlite3* connection, SOCKET clientSocket) {
    string query = "SELECT website_username, username, password FROM passwords";
    char* errMsg;
    //string passwordEntry;
    // Execute the query and send each password entry to the client
    int result = sqlite3_exec(
        connection,
        query.c_str(),
        [](void* data, int argc, char** argv, char** colName) -> int {
            SOCKET clientSocket = *static_cast<SOCKET*>(data);
            // Prepare the password entry string
            string passwordEntry = "Website|Username: " + string(argv[0]) + " , Username: " + string(argv[1]);
            passwordEntry += "\n";
            // Send the password entry to the client
            send(clientSocket, passwordEntry.c_str(), passwordEntry.size(), 0);
            return 0;
        },
        &clientSocket,
        &errMsg
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to retrieve passwords: " << errMsg << endl;
        sqlite3_free(errMsg);
    }

    // Signal the end of password entries to the client
    const char* endSignal = "End Of Passwords";
    send(clientSocket, endSignal, strlen(endSignal), 0);
    //cout << "here";

    // Wait for client acknowledgment
    char ackBuffer[1024];
    memset(ackBuffer, 0, sizeof(ackBuffer));
    recv(clientSocket, ackBuffer, sizeof(ackBuffer), 0);
    string ack(ackBuffer);
    //cout << ack;

    if (strcmp(ackBuffer, "ClientReceivedPasswords") == 0) {
        cout << "Client received all password entries." << endl;
    }

    // Close the client socket
    //closesocket(clientSocket);
}

void deletePassword(sqlite3* connection, SOCKET clientSocket) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Receive website, username from the client
    if (recv(clientSocket, buffer, sizeof(buffer), 0) == SOCKET_ERROR) {
        cerr << "Error receiving data from client." << endl;
        return;
    }
    string dataReceived(buffer);
    cout << dataReceived << endl;
    size_t pos = dataReceived.find('|');
    if (pos == string::npos) {
        cerr << "Invalid data format received." << endl;
        return;
    }
    string website = dataReceived.substr(0, pos);
    dataReceived.erase(0, pos + 1);
    string username = dataReceived.substr(0, pos);
    string searchKey = website + "|" + username;

    char* errMsg;
    pair<string, string> resultPair;
    string query = "DELETE FROM passwords WHERE website_username = '" + searchKey + "'";
    int result = sqlite3_exec(
        connection,
        query.c_str(),
        [](void* data, int argc, char** argv, char** colName) -> int {
            pair<string, string>* resultPair = static_cast<pair<string, string>*>(data);
            resultPair->first = argv[0];
            resultPair->second = argv[1];
            return 0;
        },
        &resultPair,
        &errMsg
    );

    string pass;
    if (result == SQLITE_OK) {
        pass = "Password deleted successfully!";
    } else {
        pass = "Password not found for " + searchKey + ".";
    }
    send(clientSocket, pass.c_str(), pass.size(), 0);
}

std::string encryptPassword(const std::string& credentials) {
    const int shift = 3;
    std::string encrypted_credentials;

    for (char c : credentials) {
        if (isgraph(c)) {
            char base = 32;
            encrypted_credentials.push_back(((c - base + shift) % 94 + base));
        } else {
            encrypted_credentials.push_back(c);
        }
    }

    return encrypted_credentials;
}

std::string decryptPassword(const std::string& encrypted_credentials) {
    const int shift = 3;
    std::string decrypted_credentials;

    for (char c : encrypted_credentials) {
        if (isgraph(c)) {
            char base = 32;
            decrypted_credentials.push_back(((c - base + 94 - shift) % 94 + base));
        } else {
            decrypted_credentials.push_back(c);
        }
    }

    return decrypted_credentials;
}
