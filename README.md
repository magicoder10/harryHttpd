# HarryHttpd
Simple HTTP server in C

## Components
1. Server, Simple HTTP Server
1. Client, Simple HTTP Client

## Prerequisite

- Make

## Getting Started

1. Compile server and client:

   ```bash
   > make
   ```
   

1. Start the server:

   ```bash
   > ./server port_number
   ```
  
  eg.
  
  ```
  > ./server 80
  ```

1. Start the client:

   ```bash
   > ./client url port_number
   ```

   eg.
   
   ```bash
   > ./client localhost/TMDG.html 80
   ```

### Note
You can also visit `http://localhost/TMDG.html` in your web browser to verify the server is working as expected.

## Author

- **Yang Liu** - *Initial work* - [byliuyang](https://github.com/byliuyang)

## License
This repo is maintained under MIT license.
