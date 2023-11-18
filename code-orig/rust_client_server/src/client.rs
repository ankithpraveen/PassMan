use std::io::{Read, Write};
use std::net::TcpStream;
use std::io;
use rand::Rng;
// use serde_json::json; // Add serde_json crate to your dependencies

fn main() {
    // Connect to the server
    println!("Connecting to the server...");
    let mut stream = TcpStream::connect("127.0.0.1:8080").expect("Failed to connect to server");

    // Receive the welcome message from the server
    let mut welcome_buffer = [0u8; 1024];
    let n = stream.read(&mut welcome_buffer).expect("Failed to read welcome message");
    let welcome_message = String::from_utf8_lossy(&welcome_buffer[..n]);
    println!("Server says: {}", welcome_message);
    loop {

        println!("1. Add Password\n2. Retrieve Password\n3. View All Entries\n4. Delete Password\n5. Exit");
        // let mut input_to_server: String = String::new();
        let mut input = String::new();
        io::stdin().read_line(&mut input).expect("Failed to read line");
        
        let mut input_to_server = format!("{}", input.trim());
        match input.trim() {
            "1" => add_password(&mut input_to_server),
            "2" => retrieve_password(&mut input_to_server),
            "3" => {},
            "4" => delete_password(&mut input_to_server),
            "5" => {},
            _ => println!("Invalid choice. Please try again.\n"),
        }

        println!("input to server: {}", input_to_server);
        // Send values to the server as a string with '|' as the delimiter
        // let values = "1|amazon|abd|2";
        // let values = "2|amazon|abd";
        // let values = "3";
        // let values = "4|google|ap";
        let values = input_to_server;
        if values.starts_with('5'){
            stream.write_all(b"5\n").expect("Failed to send exit message");
            println!("Client exiting.");
            break;
        }
        println!("Sending values to server: {}", values);
        stream.write_all(values.as_bytes()).expect("Failed to send values");

        // Simulate receiving a response from the server
        let mut response_buffer = [0u8; 1024];
        let n = stream.read(&mut response_buffer).expect("Failed to read response");
        if n == 0 {
            // Server closed the connection
            println!("Server closed the connection");
            break;
        }
        let response_message = String::from_utf8_lossy(&response_buffer[..n]);
        println!("Server response: {}\n", response_message);

        // stream.write_all(b"Waiting for next input from client...\n").expect("Failed to send values");

    }
}

fn add_password(input_to_server: &mut String) {
    let mut website = String::new();
    println!("Enter the website or app name: ");
    let _ = io::stdin().read_line(&mut website);
    let mut username = String::new();
    println!("Enter the username: ");
    let _ = io::stdin().read_line(&mut username);

    
    println!("Choose how to set the password:");
    println!("1. Enter Password Manually");
    println!("2. Generate Random Password");
    
    let mut choice = String::new();
    println!("Enter your choice: ");
    let _ = io::stdin().read_line(&mut choice);

    input_to_server.push_str(&format!("|{}|{}", website.trim(), username.trim()));
    
    match choice.trim() {
        "1" => {
            let mut password = String::new();
            println!("Enter the password: ");
            let _ = io::stdin().read_line(&mut password);
            input_to_server.push_str(&format!("|{}", password.trim()));
        }
        "2" => {
            let random_password = generate_random_password();
            input_to_server.push_str(&format!("|{}", random_password.trim()));

        }
        _ => println!("Invalid choice. Please try again.\n"),
    }
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

fn retrieve_password(input_to_server: &mut String) {
    let mut website = String::new();
    println!("Enter the website or app name: ");
    let _ = io::stdin().read_line(&mut website);
    let mut username = String::new();
    println!("Enter the username: ");
    let _ = io::stdin().read_line(&mut username);
    
    input_to_server.push_str(&format!("|{}|{}", website.trim(), username.trim()));
}

fn delete_password(input_to_server: &mut String) {
    let mut website = String::new();
    println!("Enter the website or app name: ");
    let _ = io::stdin().read_line(&mut website);
    let mut username = String::new();
    println!("Enter the username: ");
    let _ = io::stdin().read_line(&mut username);
    
    input_to_server.push_str(&format!("|{}|{}", website.trim(), username.trim()));
}