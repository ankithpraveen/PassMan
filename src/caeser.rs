use rusqlite::{Connection, params};
use std::io;
use async_std::task;

fn main() {
    task::block_on(run());
}

async fn run() {
    let connection = Connection::open("password_manager.db").expect("Failed to open database");

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
        println!("3. Delete Password");
        println!("4. Exit");

        let choice: u32 = get_choice("Enter your choice: ");

        match choice {
            1 => add_password(&connection).await,
            2 => retrieve_password(&connection).await,
            3 => delete_password(&connection).await,
            4 => {
                println!("Exiting password manager. Goodbye!");
                break;
            }
            _ => println!("Invalid choice. Please try again."),
        }
    }
}


async fn add_password(connection: &Connection) {
    let website = get_input("Enter the website or app name: ");
    let username = get_input("Enter the username: ");
    let password = get_password("Enter the password: ");

    let composite_key = format!("{}|{}", website, username);
    let encrypted_password = encrypt_password(&password);

    println!("Encrypted Password: {}", encrypted_password); // Print the encrypted password

    connection
        .execute(
            "INSERT OR REPLACE INTO passwords (website_username, username, password) VALUES (?1, ?2, ?3)",
            params![composite_key, username, encrypted_password],
        )
        .expect("Failed to insert data");

    println!("Password added successfully!");
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
            let decrypted_password = decrypt_password(&encrypted_password);
            println!("Credentials for {} are: Username: {}, Password: {}", composite_key, found_username, decrypted_password);
        }
        Err(_) => println!("Credentials not found for {}.", composite_key),
    }
}

async fn delete_password(connection: &Connection) {
    let website = get_input("Enter the website or app name: ");
    let username = get_input("Enter the username: ");

    let composite_key = format!("{}|{}", website, username);
    match connection
        .execute("DELETE FROM passwords WHERE website_username = ?1", params![composite_key])
    {
        Ok(rows_affected) if rows_affected > 0 => println!("Password deleted successfully!"),
        _ => println!("Password not found for {}.", composite_key),
    }
}

fn get_choice(prompt: &str) -> u32 {
    loop {
        println!("{}", prompt);
        let mut input = String::new();
        io::stdin().read_line(&mut input).expect("Failed to read line");

        match input.trim().parse() {
            Ok(num) => return num,
            Err(_) => println!("Invalid input. Please enter a valid number."),
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
            println!("Invalid input. Please enter a non-empty string.");
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
            println!("Invalid input. Please enter a non-empty password.");
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