use std::net::{TcpListener, TcpStream};
use std::io::{Read, Write};
use std::thread;

fn main() {
    // Bind the server to 127.0.0.1:8080
    let listener = TcpListener::bind("127.0.0.1:8080").expect("Failed to bind to address");

    println!("Server listening on 127.0.0.1:8080");

    // Accept incoming connections
    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                println!("New connection: {}", stream.peer_addr().unwrap());
                handle_client(stream);
            }
            Err(e) => println!("Error accepting connection: {:?}", e),
        }
    }
}

fn handle_client(mut stream: TcpStream) {
    // Read the client's choice
    let mut buffer = [0u8; 1];
    stream
        .read_exact(&mut buffer)
        .expect("Failed to read client choice");

    let choice = buffer[0];

    // Send a welcome message to the client
    stream
        .write_all(b"Welcome to the Password Manager Server!\n")
        .expect("Failed to send welcome message");

    // Send the choice to the client
    stream.write_all(&[choice]).expect("Failed to send choice");

    // Simulate processing on the server side (you can replace this with your logic)
    match choice {
        b'1' => simulate_add_password(&mut stream),
        b'2' => simulate_retrieve_password(&mut stream),
        b'3' => simulate_view_all_passwords(&mut stream),
        b'4' => simulate_delete_password(&mut stream),
        b'5' => simulate_change_master_password(&mut stream),
        _ => println!("Invalid choice from client.\n"),
    }
}

// Simulate password manager functions
fn simulate_add_password(stream: &mut TcpStream) {
    // Simulate response to the client
    stream.write_all(b"Server received choice: 1 (Add Password)\n")
        .expect("Failed to send response");
}

fn simulate_retrieve_password(stream: &mut TcpStream) {
    // Simulate response to the client
    stream
        .write_all(b"Server received choice: 2 (Retrieve Password)\n")
        .expect("Failed to send response");
}

fn simulate_view_all_passwords(stream: &mut TcpStream) {
    // Simulate response to the client
    stream
        .write_all(b"Server received choice: 3 (View All Passwords)\n")
        .expect("Failed to send response");
}

fn simulate_delete_password(stream: &mut TcpStream) {
    // Simulate response to the client
    stream
        .write_all(b"Server received choice: 4 (Delete Password)\n")
        .expect("Failed to send response");
}

fn simulate_change_master_password(stream: &mut TcpStream) {
    // Simulate response to the client
    stream
        .write_all(b"Server received choice: 5 (Change Master Password)\n")
        .expect("Failed to send response");
}