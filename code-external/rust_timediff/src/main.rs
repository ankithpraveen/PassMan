use std::time::Instant;
// use base64::{encode, decode};

fn main() {
    let credentials = "abcd";
    let start_time1 = Instant::now();
    let encrypted_password = encrypt_password(&credentials);
    let end_time1 = Instant::now(); // Record the end time
    let duration1 = end_time1.duration_since(start_time1);
    println!("Time taken to encrypt password: {} nanoseconds", duration1.as_nanos());
    println!("Encrypted Password is: {}", encrypted_password);

    let start_time2 = Instant::now();
    let decrypted_password = decrypt_password(&encrypted_password);
    let end_time2 = Instant::now(); // Record the end time
    let duration2 = end_time2.duration_since(start_time2);
    println!("Time taken to decrypt password: {} nanoseconds", duration2.as_nanos());
    println!("Decrypted Password is: {}", decrypted_password);
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

// fn encrypt_password(credentials: &str) -> String {
//     let encoded = encode(credentials);
//     encoded
// }

// fn decrypt_password(encrypted_credentials: &str) -> String {
//     match decode(encrypted_credentials) {
//         Ok(decoded) => String::from_utf8_lossy(&decoded).into_owned(),
//         Err(_) => String::from("Error decoding password"),
//     }
// }