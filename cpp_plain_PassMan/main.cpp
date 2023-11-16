#include <iostream>
#include <iomanip>
#include <fstream>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <random>
#include <chrono>

using namespace std;

// Function declarations
void run();
void changeMasterPassword(sqlite3* connection);
void addPassword(sqlite3* connection);
void addPasswordWithCredentials(sqlite3* connection, const string& website, const string& username, const string& password);
string generateRandomPassword();
void retrievePassword(sqlite3* connection);
void viewAllPasswords(sqlite3* connection);
void deletePassword(sqlite3* connection);
unsigned int getChoice(const string& prompt);
string getInput(const string& prompt);
string getPassword(const string& prompt);
string encryptPassword(const string& credentials);
string decryptPassword(const string& encryptedCredentials);

int main() {
    run();
    return 0;
}

void run() {
    sqlite3* connection;
    int result = sqlite3_open("password_manager.db", &connection);

    if (result != SQLITE_OK) {
        cerr << "Failed to open database" << endl;
        exit(EXIT_FAILURE);
    }

    result = sqlite3_exec(
        connection,
        "CREATE TABLE IF NOT EXISTS master_password (password TEXT)",
        nullptr,
        nullptr,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to create master_password table" << endl;
        exit(EXIT_FAILURE);
    }

    char* errMsg;
    string masterPassword;

    int rowCount = 0;
    result = sqlite3_exec(
        connection,
        "SELECT password FROM master_password",
        [](void* data, int argc, char** argv, char** colName) -> int {
            int* rowCount = static_cast<int*>(data);
            (*rowCount)++;
            return 0;
        },
        &rowCount,
        &errMsg
    );

    if (rowCount == 0) {
        cout << "Welcome! This is the first time you are using PassMan. You need to set a new master password." << endl;
        masterPassword = getPassword("Enter a new master password: ");
        string insertQuery = "INSERT INTO master_password (password) VALUES ('" + masterPassword + "')";
        result = sqlite3_exec(connection, insertQuery.c_str(), nullptr, nullptr, &errMsg);
        if (result != SQLITE_OK) {
            cerr << "Failed to insert master password" << endl;
            exit(EXIT_FAILURE);
        }
    }

    result = sqlite3_exec(
        connection,
        "SELECT password FROM master_password",
        [](void* data, int argc, char** argv, char** colName) -> int {
            string* password = static_cast<string*>(data);
            *password = argv[0];
            return 0;
        },
        &masterPassword,
        &errMsg
    );

    int masterPasswordAttempts = 3;

    while (true) {
        string enteredPassword = getPassword("Enter the master password: ");

        if (enteredPassword == masterPassword) {
            break; // Correct master password, exit the loop
        } else {
            cout << "Incorrect master password. You have " << --masterPasswordAttempts << " attempts remaining." << endl;

            if (masterPasswordAttempts == 0) {
                cout << "Exceeded maximum number of attempts. Exiting." << endl;
                return;
            }
        }
    }

    cout << "Master password correct. Access granted.\n" << endl;

    result = sqlite3_exec(
        connection,
        "CREATE TABLE IF NOT EXISTS passwords (website_username TEXT PRIMARY KEY, username TEXT, password TEXT)",
        nullptr,
        nullptr,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to create passwords table" << endl;
        exit(EXIT_FAILURE);
    }

    while (true) {
        cout << "1. Add Password" << endl;
        cout << "2. Retrieve Password" << endl;
        cout << "3. View All Passwords" << endl;
        cout << "4. Delete Password" << endl;
        cout << "5. Change Master Password" << endl;
        cout << "6. Exit" << endl;

        unsigned int choice = getChoice("Enter your choice: ");

        switch (choice) {
            case 1:
                addPassword(connection);
                break;
            case 2:
                retrievePassword(connection);
                break;
            case 3:
                viewAllPasswords(connection);
                break;
            case 4:
                deletePassword(connection);
                break;
            case 5:
                changeMasterPassword(connection);
                break;
            case 6:
                cout << "Exiting password manager. Goodbye!" << endl;
                sqlite3_close(connection);
                exit(EXIT_SUCCESS);
            default:
                cout << "Invalid choice. Please try again." << endl;
        }
    }
}

void changeMasterPassword(sqlite3* connection) {
    string currentPassword = getPassword("Enter the current master password: ");

    string storedPassword;
    char* errMsg;

    int result = sqlite3_exec(
        connection,
        "SELECT password FROM master_password",
        [](void* data, int argc, char** argv, char** colName) -> int {
            string* password = static_cast<string*>(data);
            *password = argv[0];
            return 0;
        },
        &storedPassword,
        &errMsg
    );

    if (result != SQLITE_OK) {
        cerr << "Master password not found in the database. You can't change it now." << endl;
        return;
    }

    if (currentPassword != storedPassword) {
        cout << "Incorrect current master password. Change password operation aborted." << endl;
        return;
    }

    string newPassword = getPassword("Enter a new master password: ");
    string updateQuery = "UPDATE master_password SET password = '" + newPassword + "'";
    result = sqlite3_exec(connection, updateQuery.c_str(), nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK) {
        cerr << "Failed to update master password" << endl;
        exit(EXIT_FAILURE);
    }

    cout << "Master password changed successfully." << endl;
}

void addPassword(sqlite3* connection) {
    string website = getInput("Enter the website or app name: ");
    string username = getInput("Enter the username: ");

    cout << "Choose how to set the password:" << endl;
    cout << "1. Enter Password Manually" << endl;
    cout << "2. Generate Random Password" << endl;

    unsigned int choice = getChoice("Enter your choice: ");

    switch (choice) {
        case 1: {
            string password = getPassword("Enter the password: ");
            addPasswordWithCredentials(connection, website, username, password);
            break;
        }
        case 2: {
            string randomPassword = generateRandomPassword();
            addPasswordWithCredentials(connection, website, username, randomPassword);
            break;
        }
        default:
            cout << "Invalid choice. Please try again." << endl;
    }
}

void addPasswordWithCredentials(sqlite3* connection, const string& website, const string& username, const string& password) {
    // Generate a random 5-character nonce with characters from the specified set
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
    string encryptedPassword = encryptPassword(credentials);
    cout << "Encrypted Password is: " << encryptedPassword << endl;

    string insertQuery = "INSERT OR REPLACE INTO passwords (website_username, username, password) VALUES ('" + compositeKey + "', '" + username + "', '" + encryptedPassword + "')";
    char* errMsg;
    int result = sqlite3_exec(connection, insertQuery.c_str(), nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK) {
        cerr << "Failed to insert data" << endl;
        exit(EXIT_FAILURE);
    }

    cout << "Password added successfully!" << endl;
}

string generateRandomPassword() {
    const string characterSet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    string password;

    for (int i = 0; i < 16; i++) {
        int randomIndex = rand() % characterSet.length();
        password += characterSet[randomIndex];
    }

    cout << "Your random password is: " << password << endl;
    return password;
}

void retrievePassword(sqlite3* connection) {
    string website = getInput("Enter the website or app name: ");
    string username = getInput("Enter the username: ");

    string compositeKey = website + "|" + username;

    char* errMsg;
    pair<string, string> resultPair;

    string query = "SELECT username, password FROM passwords WHERE website_username = '" + compositeKey + "'";
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
    string decryptedPasswordWithNonce = decryptPassword(resultPair.second);

    // Check if the decrypted password is empty before using substr
    if (!decryptedPasswordWithNonce.empty() && decryptedPasswordWithNonce.size() >= 5) {
        // Extract the nonce and credentials
        string nonce = decryptedPasswordWithNonce.substr(0, 5);
        string decryptedPassword = decryptedPasswordWithNonce.substr(5);

        cout << "Credentials for " << compositeKey << " are: Username: " << resultPair.first << ", Password: " << decryptedPassword << endl;
    } else {
        cout << "Credentials not found for " << compositeKey << "." << endl;
    }
} else {
    cout << "Credentials not found for " << compositeKey << "." << endl;
}

}

void viewAllPasswords(sqlite3* connection) {
    cout << "Listing all stored credentials (website and username):" << endl;
    char* errMsg;
    bool found = false;

    int result = sqlite3_exec(
        connection,
        "SELECT website_username FROM passwords",
        [](void* data, int argc, char** argv, char** colName) -> int {
            cout << "Website: " << argv[0] << endl;
            *static_cast<bool*>(data) = true;
            return 0;
        },
        &found,
        &errMsg
    );

    if (!found) {
        cout << "No passwords stored." << endl;
    }

    cout << endl;
}

void deletePassword(sqlite3* connection) {
    string website = getInput("Enter the website or app name: ");
    string username = getInput("Enter the username: ");

    string compositeKey = website + "|" + username;

    char* errMsg;
    string deleteQuery = "DELETE FROM passwords WHERE website_username = '" + compositeKey + "'";
    int result = sqlite3_exec(connection, deleteQuery.c_str(), nullptr, nullptr, &errMsg);

    if (result == SQLITE_OK) {
        cout << "Password deleted successfully!" << endl;
    } else {
        cout << "Password not found for " << compositeKey << "." << endl;
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