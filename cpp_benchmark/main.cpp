#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <benchmark/benchmark.h>
#include <sys/resource.h>

void print_memory_usage() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        std::cout << "Memory used: " << usage.ru_maxrss / 1024.0 << " MB" << std::endl;
    } else {
        std::cerr << "Failed to get memory usage information." << std::endl;
    }
}

std::string get_input(const std::string &prompt) {
    std::string input;
    do {
        std::cout << prompt;
        std::getline(std::cin, input);
    } while (input.empty());

    return input;
}

int get_choice(const std::string &prompt) {
    int choice;
    do {
        std::cout << prompt;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // clear the input buffer
    } while (std::cin.fail());

    return choice;
}

std::string get_password(const std::string &prompt) {
    std::string password;
    do {
        std::cout << prompt;
        std::cin >> password;
    } while (password.empty());

    return password;
}

std::string encrypt_password(const std::string &credentials) {
    const int shift = 3;
    std::string encrypted;
    for (char c : credentials) {
        if (std::isgraph(c, std::locale())) {
            char encrypted_char = ((c - 32 + shift) % 94 + 32);
            encrypted.push_back(encrypted_char);
        } else {
            encrypted.push_back(c);
        }
    }
    return encrypted;
}

std::string decrypt_password(const std::string &encrypted_credentials) {
    const int shift = 3;
    std::string decrypted;
    for (char c : encrypted_credentials) {
        if (std::isgraph(c, std::locale())) {
            char decrypted_char = ((c - 32 + 94 - shift) % 94 + 32);
            decrypted.push_back(decrypted_char);
        } else {
            decrypted.push_back(c);
        }
    }
    return decrypted;
}

void create_table(sqlite3 *db) {
    char *errMsg = nullptr;
    const char *query = "CREATE TABLE IF NOT EXISTS passwords ("
                        "website_username TEXT PRIMARY KEY, "
                        "username TEXT, "
                        "password TEXT)";
    if (sqlite3_exec(db, query, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        exit(EXIT_FAILURE);
    }
}

void add_password(sqlite3 *db) {
    std::string website = get_input("Enter the website or app name: ");
    std::string username = get_input("Enter the username: ");
    std::string password = get_password("Enter the password: ");

    std::string composite_key = website + "|" + username;
    std::string encrypted_password = encrypt_password(password);

    std::string query = "INSERT OR REPLACE INTO passwords (website_username, username, password) "
                        "VALUES ('" + composite_key + "', '" + username + "', '" + encrypted_password + "')";

    if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to insert data" << std::endl;
    } else {
        std::cout << "Password added successfully!" << std::endl;
    }
}

void retrieve_password(sqlite3 *db) {
    std::string website = get_input("Enter the website or app name: ");
    std::string username = get_input("Enter the username: ");

    std::string composite_key = website + "|" + username;
    std::string query = "SELECT username, password FROM passwords WHERE website_username = '" + composite_key + "'";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string found_username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            std::string encrypted_password = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

            std::string decrypted_password = decrypt_password(encrypted_password);
            std::cout << "Credentials for " << composite_key << " are: Username: " << found_username
                      << ", Password: " << decrypted_password << std::endl;
        } else {
            std::cout << "Credentials not found for " << composite_key << "." << std::endl;
        }

        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Failed to execute query" << std::endl;
    }
}

void delete_password(sqlite3 *db) {
    std::string website = get_input("Enter the website or app name: ");
    std::string username = get_input("Enter the username: ");

    std::string composite_key = website + "|" + username;
    std::string query = "DELETE FROM passwords WHERE website_username = '" + composite_key + "'";

    char *errMsg = nullptr;
    if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg) == SQLITE_OK) {
        int rows_affected = sqlite3_changes(db);
        if (rows_affected > 0) {
            std::cout << "Password deleted successfully!" << std::endl;
        } else {
            std::cout << "Password not found for " << composite_key << "." << std::endl;
        }
    } else {
        std::cerr << "Failed to execute query: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

static void BM_EncryptPassword(benchmark::State& state) {
    // Prepare input data for benchmarking
    std::string credentials = "example_credentials";

    for (auto _ : state) {
        // Benchmark the encrypt_password function
        std::string encrypted_password = encrypt_password(credentials);
        benchmark::DoNotOptimize(encrypted_password);
    }
}

BENCHMARK(BM_EncryptPassword);

static void BM_DecryptPassword(benchmark::State& state) {
    // Prepare input data for benchmarking
    std::string encrypted_credentials = encrypt_password("example_credentials");

    for (auto _ : state) {
        // Benchmark the decrypt_password function
        std::string decrypted_password = decrypt_password(encrypted_credentials);
        benchmark::DoNotOptimize(decrypted_password);
    }
}

BENCHMARK(BM_DecryptPassword);


int main(int argc, char** argv) {
    // Initialize Google Benchmark
    ::benchmark::Initialize(&argc, argv);

    sqlite3 *db;
    if (sqlite3_open("password_manager.db", &db) != SQLITE_OK) {
        std::cerr << "Failed to open database" << std::endl;
        return EXIT_FAILURE;
    }

    create_table(db);

    while (true) {
        std::cout << "1. Add Password\n"
                     "2. Retrieve Password\n"
                     "3. Delete Password\n"
                     "4. Benchmark Encrypt Password\n" 
                     "5. Benchmark Decrypt Password\n" // Added benchmark option
                     "6. Exit\n";

        int choice = get_choice("Enter your choice: ");

        switch (choice) {
            case 1:
                add_password(db);
                print_memory_usage();
                break;
            case 2:
                retrieve_password(db);
                print_memory_usage();
                break;
            case 3:
                delete_password(db);
                print_memory_usage();
                break;
            case 4:
                // Run the benchmark for encrypt_password function
                ::benchmark::RunSpecifiedBenchmarks();
                break;
            case 5:
                ::benchmark::RunSpecifiedBenchmarks();
                break;
            case 6:  // Add a new case for decrypt_password benchmark
                std::cout << "Exiting password manager. Goodbye!" << std::endl;
                sqlite3_close(db);
                print_memory_usage();
                return EXIT_SUCCESS;
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
        }
    }
}
