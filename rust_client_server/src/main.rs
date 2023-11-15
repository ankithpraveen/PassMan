use std::net::{TcpListener, TcpStream};
use std::io::{Read, Write};
use rusqlite::{Connection, params};
use rand::Rng;
use std::time::Instant;
// use async_std::task;
use std::thread;
// use serde_json::Value;


fn main() {
    // Bind the server to 127.0.0.1:8080
    let listener = TcpListener::bind("127.0.0.1:8080").expect("Failed to bind to address");

    println!("Server listening on 127.0.0.1:8080");

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                println!("New connection: {}", stream.peer_addr().unwrap());
                thread::spawn(|| {
                    async_std::task::block_on(handle_client(stream));
                });
            },
            Err(e) => println!("Failed to connect. Error: {}", e),
        }
    }
}

async fn handle_client(mut stream: TcpStream) {
    // Send a welcome message to the client
    stream
        .write_all(b"Welcome to the Password Manager Server!\n")
        .expect("Failed to send welcome message");

    // Flush the stream to ensure the client receives the welcome message
    stream.flush().expect("Failed to flush stream");

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

    loop{
        // Read the values string from the client
        let mut buffer = [0u8; 1024];
        let n = stream.read(&mut buffer).expect("Failed to read welcome message");
        if n == 0{
            println!("Server didn't receive any input from client. Exiting...\n");
            break;
        }
        let buffer_str = String::from_utf8_lossy(&buffer[..n]);

        let split_values: Vec<&str> = buffer_str.split('|').collect();
        // Print the received values
        println!("Received values from client: {}", buffer_str);
        let choice = buffer[0];

        // Simulate processing on the server side (you can replace this with your logic)
        match choice {
            b'1' => simulate_add_password(&mut stream, &split_values, &connection).await,
            b'2' => simulate_retrieve_password(&mut stream, &split_values, &connection).await,
            b'3' => simulate_view_all_passwords(&mut stream, &split_values, &connection).await,
            b'4' => simulate_delete_password(&mut stream, &split_values, &connection).await,
            b'5' => break,
            _default => println!("Invalid choice from client.\n"),
        }
    }
    

}

// Simulate password manager functions
async fn simulate_add_password(stream: &mut TcpStream, split_values: &Vec<&str>, connection: &Connection) {
    // Simulate response to the client
    println!("Server received choice: 1 (Add Password)\n");
    let mut output: String = String::new();

    let website = split_values[1];
    let username = split_values[2];
    
    // output = format!("{}Choose how to set the password:", output);
    // output = format!("{}1. Enter Password Manually", output);
    // output = format!("{}2. Generate Random Password", output);
    // let choice = split_values[3];

    let password = split_values[3];
    add_password_with_credentials(connection, website.to_string(), username.to_string(), password.to_string(), &mut output).await;
    
    let output_bytes = output.clone().into_bytes();
    stream
        .write_all(&output_bytes)
        .expect("Failed to send response");
}

async fn add_password_with_credentials(connection: &Connection, website: String, username: String, password: String, output: &mut String) {
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
    let start_time = Instant::now();
    let encrypted_password = encrypt_password(&credentials);
    let end_time = Instant::now(); // Record the end time
    let duration = end_time.duration_since(start_time);

    println!("Time taken to encrypt password: {} nanoseconds", duration.as_nanos());
    output.push_str(&format!("{}Encrypted Password is: {}\n", output, encrypted_password));

    connection
        .execute(
            "INSERT OR REPLACE INTO passwords (website_username, username, password) VALUES (?1, ?2, ?3)",
            params![composite_key, username, encrypted_password],
        )
        .expect("Failed to insert data");

    // Modify the underlying String using a mutable reference
    output.push_str(&format!("{}Password added successfully!\n", output));
}

async fn simulate_retrieve_password(stream: &mut TcpStream, split_values: &Vec<&str>, connection: &Connection) {
    // Simulate response to the client
    println!("Server received choice: 2 (Retrieve Password)\n");
    // stream
    //     .write_all(b"\nServer received choice: 2 (Retrieve Password)")
    //     .expect("Failed to send response");

    let website = split_values[1];
    let username = split_values[2];

    let composite_key = format!("{}|{}", website, username);

    match connection.query_row(
        "SELECT username, password FROM passwords WHERE website_username = ?1",
        params![composite_key],
        |row| Ok((row.get::<usize, String>(0)?, row.get::<usize, String>(1)?)),
    ) {
        Ok((found_username, encrypted_password)) => {
            // Decrypt the password using RSA
            
            let start_time = Instant::now();
            let decrypted_password_with_nonce = decrypt_password(&encrypted_password);
            let end_time = Instant::now(); // Record the end time
            let duration = end_time.duration_since(start_time);

            println!("Time taken to decrypt password: {} nanoseconds", duration.as_nanos());

            // Extract the nonce and credentials
            let (_nonce, decrypted_password) = decrypted_password_with_nonce.split_at(5);
            
            let output = format!("\nCredentials for {} are: Username: {}, Password: {}\n", composite_key, found_username, decrypted_password);

            // Convert the string to a byte string
            let output_bytes = output.into_bytes();
            stream
                .write_all(&output_bytes)
                .expect("Failed to send response");
            println!("Credentials for {} are: Username: {}, Password: {}\n", composite_key, found_username, decrypted_password);
        }
        Err(_) => {
            let output = format!("Credentials not found for {}.\n", composite_key);
            let output_bytes = output.clone().into_bytes();
            stream
                .write_all(&output_bytes)
                .expect("Failed to send response");
        },
    }
}

async fn simulate_view_all_passwords(stream: &mut TcpStream, _split_values: &Vec<&str>, connection: &Connection) {
    // Simulate response to the client
    println!("Server received choice: 3 (View All Passwords)\n");

    let mut output: String = String::new();

    output = format!("{}Listing all stored credentials (website and username):\n", output);
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
            output = format!("{}Website: {}, Username: {}\n", output, website, username);
            found = true;
        }
    }

    if !found {
        output = format!("{}No passwords stored.", output);
    }
    let output_bytes = output.clone().into_bytes();
    stream
        .write_all(&output_bytes)
        .expect("Failed to send response");
}

async fn simulate_delete_password(stream: &mut TcpStream, split_values: &Vec<&str>, connection: &Connection) {
    // Simulate response to the client
    println!("Server received choice: 4 (Delete Password)\n");
    let mut output: String = String::new();

    let website = split_values[1];
    let username = split_values[2];   
    let composite_key = format!("{}|{}", website, username);
    match connection
        .execute("DELETE FROM passwords WHERE website_username = ?1", params![composite_key])
    {
        Ok(rows_affected) if rows_affected > 0 => output = format!("{}Password deleted successfully!\n", output),
        _ => output = format!("{}Password not found for {}.\n", output, composite_key),
    }
    let output_bytes = output.into_bytes();
    stream
        .write_all(&output_bytes)
        .expect("Failed to send response");
}

// fn simulate_change_master_password(stream: &mut TcpStream) {
//     // Simulate response to the client
//     stream
//         .write_all(b"Server received choice: 5 (Change Master Password)\n")
//         .expect("Failed to send response");
// }

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