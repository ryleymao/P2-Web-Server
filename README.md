Project 2: Web Server

This project is a lightweight web-based chat server implemented in C. It simulates the core functionality of a group messaging app (like Slack or Discord) but operates entirely over HTTP using low-level system calls, without the use of any external frameworks or libraries. The main goal of this project is to build a deeper understanding of how web servers work at the systems level—specifically, how HTTP requests are received, parsed, and responded to using socket programming in C. It also highlights how dynamic data (like chat messages and user reactions) can be managed manually in memory.

Overview
The server can be compiled and launched with:

make chat-server
./chat-server [optional-port]

It listens for incoming HTTP requests and supports the following endpoints:

/chats
Responds with a plain-text list of all chat messages and any associated reactions. The format includes an ID, timestamp, username, message, and formatted reactions. Example:

[#5 2024-10-06 09:07] joe: I pushed an example, go check it out
(aaron) 👍🏻

/post
Creates a new chat message with a username and message string:
/post?user=<username>&message=<message>
The server timestamps the post when received. It validates input to ensure usernames are no longer than 15 bytes and messages no longer than 255 bytes. If the total number of chats exceeds 100,000, the server will return an error.

/react
Allows users to add a short reaction to a specific chat by ID:
/react?user=<username>&message=<reaction>&id=<id>
Reactions are limited to 15 bytes, and each message can have up to 100 reactions. The server checks for valid input and responds with an updated chat log.

/edit
Enables users to edit a previously posted message:
/edit?id=<id>&message=<new_message>
This replaces the message of the chat with the specified ID. Input is validated to ensure the chat ID exists and the new message is within the 255-byte limit.

/reset
Clears all messages and reactions, resetting the server to its initial state. This is useful for testing and debugging. Memory is properly freed and reused after a reset.

Why This Project Matters
This project is important for several reasons:

Systems-level understanding: It demystifies how real web servers process HTTP traffic using low-level socket programming.

Memory management: Since this project avoids using global variables and handles dynamic memory manually, it sharpens skills in memory allocation, deallocation, and safety.

Input parsing: Working with raw HTTP GET requests and URL query parameters helps develop strong debugging and parsing techniques.

Design trade-offs: The project includes reflections on data structure choices (e.g., using char** arrays vs. structs) and their impact on features like /edit.

Scalability concerns: The design enforces limits (like max chats and reaction counts), offering insight into how large-scale systems protect themselves from overload.

Overall, this project is an excellent introduction to building networked software from scratch using only core OS and C language capabilities.
