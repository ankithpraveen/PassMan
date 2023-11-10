use rusqlite::{Connection, params};
use std::io;
use async_std::task;
use std::time::Instant;
use rand::Rng;

fn main() {
    task::block_on(run());
}

async fn run() {
    let connection = Connection::open("password_manager.db").expect("Failed to open database");

    connection
        .execute(
            "CREATE TABLE IF NOT EXISTS master_password (
                password TEXT
            )",
            params![],
        )
        .expect("Failed to create master_password table");

    let master_password: String = match connection.query_row(
        "SELECT password FROM master_password",
        params![],
        |row| Ok(row.get(0)?),
    ) {
        Ok(password) => password,
        Err(_) => {
            println!("Welcome! This is the first time you are using PassMan. You need to set a new master password.");
            let new_master_password = get_password("Enter a new master password: ");
            connection
                .execute(
                    "INSERT INTO master_password (password) VALUES (?1)",
                    params![new_master_password],
                )
                .expect("Failed to insert master password");
            new_master_password
        }
    };

    let mut master_password_attempts = 2;

    loop {
        let entered_password = get_password("Enter the master password: ");
        
        if entered_password == master_password {
            break; // Correct master password, exit the loop
        } else {
            println!("Incorrect master password. You have {} attempts remaining.", master_password_attempts);
            master_password_attempts -= 1;

            if master_password_attempts == 0 {
                println!("Exceeded maximum number of attempts. Exiting.");
                return;
            }
        }
    }

    println!("Master password correct. Access granted.\n");

    connection
        .execute(
            "CREATE TABLE IF NOT EXISTS passwords (
                website_username TEXT PRIMARY KEY,
                username TEXT,
                password TEXT
            )",
            params![],
        )
        .expect("Failed to create table");

    loop {
        println!("1. Add Password");
        println!("2. Retrieve Password");
        println!("3. View All Passwords");
        println!("4. Delete Password");
        println!("5. Change Master Password");
        println!("6. Exit");

        let choice: u32 = get_choice("Enter your choice: ");

        match choice {
            1 => add_password(&connection).await,
            2 => retrieve_password(&connection).await,
            3 => view_all_passwords(&connection).await,
            4 => delete_password(&connection).await,
            5 => change_master_password(&connection).await,
            6 => {
                println!("Exiting password manager. Goodbye!");
                break;
            }
            _ => println!("Invalid choice. Please try again.\n"),
        }
    }
}

async fn change_master_password(connection: &Connection) {
    let current_password = get_password("Enter the current master password: ");

    let stored_password: String = match connection.query_row(
        "SELECT password FROM master_password",
        params![],
        |row| Ok(row.get(0)?),
    ) {
        Ok(password) => password,
        Err(_) => {
            println!("Master password not found in the database. You can't change it now.");
            return;
        }
    };

    if current_password != stored_password {
        println!("Incorrect current master password. Change password operation aborted.\n");
        return;
    }

    let new_password = get_password("Enter a new master password: ");
    connection
        .execute(
            "UPDATE master_password SET password = ?1",
            params![new_password],
        )
        .expect("Failed to update master password");
    
    println!("Master password changed successfully.\n");
}

async fn add_password(connection: &Connection) {
    let website = get_input("Enter the website or app name: ");
    let username = get_input("Enter the username: ");
    
    println!("Choose how to set the password:");
    println!("1. Enter Password Manually");
    println!("2. Generate Random Password");

    let choice: u32 = get_choice("Enter your choice: ");

    match choice {
        1 => {
            let password = get_password("Enter the password: ");
            add_password_with_credentials(connection, website, username, password).await;
        }
        2 => {
            let random_password = generate_random_password();
            add_password_with_credentials(connection, website, username, random_password).await;
        }
        _ => println!("Invalid choice. Please try again.\n"),
    }
}

async fn add_password_with_credentials(connection: &Connection, website: String, username: String, password: String) {
    // Generate a random 5-character nonce with characters from the specified set
    let nonce: String = (0..5)
        .map(|_| {
            let ascii_char = rand::thread_rng().gen_range(32..127) as u8;
            char::from(ascii_char)
        })
        .collect();

    // Combine the nonce with the credentials
    let credentials = format!("{}{}", nonce, password);

    let composite_key = format!("{}|{}", website, username);

    // Encrypt the password using RSA
    let encrypted_password = encrypt_password(&credentials);
    println!("Encrypted Password is: {}\n", encrypted_password);

    connection
        .execute(
            "INSERT OR REPLACE INTO passwords (website_username, username, password) VALUES (?1, ?2, ?3)",
            params![composite_key, username, encrypted_password],
        )
        .expect("Failed to insert data");

    println!("Password added successfully!\n");
}

fn generate_random_password() -> String {
    // Define a set of characters from which to generate the random password
    let character_set = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    let password: String = (0..16)  // You can adjust the length of the password as needed
        .map(|_| {
            let random_index = rand::thread_rng().gen_range(0..character_set.len());
            character_set.chars().nth(random_index).unwrap()
        })
        .collect();
    
    println!("Your random password is: {}\n", password);
    password
}

async fn retrieve_password(connection: &Connection) {
    let website = get_input("Enter the website or app name: ");
    let username = get_input("Enter the username: ");

    let composite_key = format!("{}|{}", website, username);

    match connection.query_row(
        "SELECT username, password FROM passwords WHERE website_username = ?1",
        params![composite_key],
        |row| Ok((row.get::<usize, String>(0)?, row.get::<usize, String>(1)?)),
    ) {
        Ok((found_username, encrypted_password)) => {
            // Decrypt the password using RSA
            let decrypted_password_with_nonce = decrypt_password(&encrypted_password);

            // Extract the nonce and credentials
            let (_nonce, decrypted_password) = decrypted_password_with_nonce.split_at(5);

            println!("Credentials for {} are: Username: {}, Password: {}\n", composite_key, found_username, decrypted_password);
        }
        Err(_) => println!("Credentials not found for {}.\n", composite_key),
    }
}

async fn view_all_passwords(connection: &Connection) {
    println!("Listing all stored credentials (website and username):");
    let mut statement = connection
        .prepare("SELECT website_username FROM passwords")
        .expect("Failed to prepare statement");

    let mut rows = statement
        .query(params![])
        .expect("Failed to query for stored passwords");

    let mut found = false;

    while let Some(row) = rows.next().expect("Failed to fetch next row") {
        let website_username: String = row.get(0).expect("Failed to get website_username");

        let parts: Vec<&str> = website_username.split('|').collect();
        if parts.len() == 2 {
            let website = parts[0];
            let username = parts[1];
            println!("Website: {}, Username: {}", website, username);
            found = true;
        }
    }

    if !found {
        println!("No passwords stored.");
    }
    println!();
}


async fn delete_password(connection: &Connection) {
    let website = get_input("Enter the website or app name: ");
    let username = get_input("Enter the username: ");

    let composite_key = format!("{}|{}", website, username);
    match connection
        .execute("DELETE FROM passwords WHERE website_username = ?1", params![composite_key])
    {
        Ok(rows_affected) if rows_affected > 0 => println!("Password deleted successfully!\n"),
        _ => println!("Password not found for {}.\n", composite_key),
    }
}

fn get_choice(prompt: &str) -> u32 {
    loop {
        println!("{}", prompt);
        let mut input = String::new();
        io::stdin().read_line(&mut input).expect("Failed to read line");

        match input.trim().parse() {
            Ok(num) => return num,
            Err(_) => println!("Invalid input. Please enter a valid number.\n"),
        }
    }
}

fn get_input(prompt: &str) -> String {
    loop {
        println!("{}", prompt);
        let mut input = String::new();
        io::stdin().read_line(&mut input).expect("Failed to read line");

        let trimmed_input = input.trim().to_string();

        if !trimmed_input.is_empty() {
            return trimmed_input;
        } else {
            println!("Invalid input. Please enter a non-empty string.\n");
        }
    }
}

fn get_password(prompt: &str) -> String {
    loop {
        println!("{}", prompt);
        let password = rpassword::read_password().expect("Failed to read password");

        if !password.trim().is_empty() {
            return password.trim().to_string();
        } else {
            println!("Invalid input. Please enter a non-empty password.\n");
        }
    }
}

fn encrypt_password(credentials: &str) -> String {
    let shift: u8 = 3;
    credentials
        .chars()
        .map(|c| {
            if c.is_ascii_graphic() {
                let base = 32;
                ((c as u8 - base + shift) % 94 + base) as char
            } else {
                c
            }
        })
        .collect()
}

fn decrypt_password(encrypted_credentials: &str) -> String {
    let shift: u8 = 3;
    encrypted_credentials
        .chars()
        .map(|c| {
            if c.is_ascii_graphic() {
                let base = 32; 
                ((c as u8 - base + 94 - shift) % 94 + base) as char
            } else {
                c
            }
        })
        .collect()
}