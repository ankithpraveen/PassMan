# PassMan

## Overview

Welcome to the Password Manager Project, a comprehensive exploration of password management implementations with a focus on Rust and C++. This repository comprises various components, including basic password managers, benchmarking tools, concurrent password managers, and simple encryption examples.

## Project Structure

### Password Manager Implementations

- **Rust Password Manager**: A basic password manager written in Rust.
- **C++ Password Manager**: A basic password manager written in C++.

### Benchmarking Tools

- **Rust Password Manager Benchmark**: Benchmarking code in Rust, showcasing memory usage for the password manager program.
- **C++ Password Manager Benchmark**: Similar to its Rust counterpart, this folder contains benchmarking tools implemented in C++.

### Concurrent Password Managers

- **Concurrent Rust Password Manager**: Rust implementation featuring server and client code with threading for concurrent operation.
- **Concurrent C++ Password Manager**: C++ counterpart demonstrating concurrency with server and client threads.

### Simple Encryption Examples

- **Rust Simple Encryption**: Basic encryption and decryption in Rust for performance comparison.
- **C++ Simple Encryption**: A parallel simple encryption and decryption program written in C++.

## Results

### Rust Password Manager Benchmark

#### Memory Usage

Sampled Average Memory Utilizatoin: 3.48 MB

### C++ Password Manager Benchmark

#### Memory Usage

Sampled Average Memory Utilizatoin: 1824 MB

### Simple Encryption-Decryption Comparison

#### Rust Simple Encryption-Decryption Time

Sampled Average Encryption Time: 8460 ns<br>
Sampled Average Decryption Time: 3260 ns

#### C++ Simple Encryption-Decryption Time

Sampled Average Encryption Time: 6600 ns<br>
Sampled Average Decryption Time: 1400 ns

#### Rust Client Creation and Termination Time

Sampled Average Time: 2205320 ns

#### C++ Client Creation and Termination Time

Sampled Average Time: 3465340 ns

## Conclusion

### Memory Security in Rust vs. C++

Memory security is notably better in Rust compared to C++ for several reasons. Rust's ownership system and borrow checker ensure strict control over memory access, preventing common issues like null pointer dereferencing and data races. This enhanced memory safety is crucial for security-sensitive applications, such as password managers, where vulnerabilities can lead to severe consequences.

### Memory Management Comparison via Benchmark

Benchmarking results consistently demonstrate superior memory management and usage in Rust when compared to C++. Rust's ownership model enables efficient memory allocation and deallocation, minimizing unnecessary overhead and reducing the risk of memory-related vulnerabilities. This advantage is particularly crucial in security-focused applications like password managers.

### Time Difference for Encryption and Decryption

The time difference for encryption and decryption of a fixed string is comparable between Rust and C++. This similarity can be attributed to the nature of cryptographic operations, where the performance is influenced by algorithmic efficiency rather than language-specific optimizations. While Rust introduces a slight overhead, the observed differences are marginal in scenarios where cryptographic tasks are the primary concern.

### Time Difference for Client Creation and Immediate Termination

When creating and immediately terminating clients, the time difference is comparable between Rust and C++, with Rust exhibiting a slightly higher execution time. This suggests that the initialization and cleanup procedures, involving less computation and memory-intensive operations, are optimized slightly more effectively in Rust. This phenomenon can be attributed to Rust's emphasis on safety and initialization procedures, introducing a small computational overhead. However, in scenarios involving frequent client initiation and termination, the differences are minimal and may not significantly impact overall performance.

### Easier Concurrency Implementation in Rust

Concurrency was notably easier to implement in Rust compared to C++ for various reasons. Rust's ownership system and borrowing rules facilitate safe concurrent programming without data races or shared-memory vulnerabilities. Additionally, Rust's ownership model eliminates the need for manual memory management in concurrent scenarios, simplifying code and reducing the likelihood of bugs associated with parallel execution. Moreover, it was easier to implement concurrency in Rust than C++, as Rust gave rise to compilation errors which were easier to debug, while C++ gave rise to runtime errors which required a lot more effort to rectify.

## Contributors

- Ankith Praveen (2020B4A70625G)
- Ishaan Kudchadkar (2020B5A70775G)
- Lalita Pulavarti (2020B4A71884G)
- Aditi Kashyap (2020B4A71654G)