#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>

using namespace std;
using namespace std::chrono;

string encryptPassword(const string& credentials);
string decryptPassword(const string& encryptedCredentials);

int main() {
    string pass = "abcd";
    auto start1 = high_resolution_clock::now();
    string encryptedPassword = encryptPassword(pass);
    auto stop1 = high_resolution_clock::now();
    auto duration1 = duration_cast<nanoseconds>(stop1 - start1);
    cout << "Encrypted Password is: " << encryptedPassword << endl;
    cout << "Encryption time: " << duration1.count() << " ns" << endl;
    auto start2 = high_resolution_clock::now();
    string decryptedPasswordWithNonce = decryptPassword(encryptedPassword);
    auto stop2 = high_resolution_clock::now();
    auto duration2 = duration_cast<nanoseconds>(stop2 - start2);
    cout << "Decrypted Password is: " << decryptedPasswordWithNonce << endl;
    cout << "Decryption time: " << duration2.count() << " ns" << endl;
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