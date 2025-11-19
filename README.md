# Multiplayer-Trivia-Game
**Author:** Michael Lamaze 

## Overview:
This is a trivia game that is ran through the terminal using TCP/IP sockets and `select()` for non-blocking, mulitplexed I/O. The server supports up to 3 simultaneous players and will handle any disconnects. It runs a full trivia quiz loaded from a question file, where the questions can be changed to any multiple choice questions you would like! After each question, the scores for each player get updated and a winner is announced at the end. 

## Features:

- **Multiplexed I/O:**  
Both the server and client use `select()` to monitor multiple file descriptors  
Client outputs terminal input with server messages, and the server handles player input without blocking on any connection

- **Dynamic Question Loading:**  
Reads prompts, answer choices, and correct answers from a plain-text question file (`-f questions.txt`)  
Easy customization and scalability of quiz content without modifying code  

- **Command-Line Arguments:**  
`-f`: Question file   
`-i`: IP to bind to  
`-p`: Port number  
`-h`: Help message  

- **Scoring:**  
Correct answer: +1 point  
Incorrect answer: -1 point  
Highest score at the end wins

- **Build System:**  
Includes a simple `Makefile` with `all` and `clean` targets, building with `-g -Wall` for debugging support and warnings

## Build & Run:
```
# Build both server and client
make

# Start the server (listen on all interfaces, port 25555):
./server -f questions.txt -i 0.0.0.0 -p 25555

# On each client machine, connect using the server's LAN IP (or host's IPv4 for online play):  
# Note: If running through a VM, ensure the host's network adapter is set to “Bridged”  
./client -i 10.0.2.15 -p 25555
```  
Parts of this code are credited to *Systems Programming, 3rd Edition* by Shudong Hao.
