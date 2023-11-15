@echo off

rem Run 10 instances of the Rust client code using Cargo
for /l %%i in (1,1,5) do (
  start "" cargo run --bin client
)