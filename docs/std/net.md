# Networking (`std/net.zc`)

The `std/net` module provides basic TCP networking capabilities.

## Usage

```zc
import "std/net.zc"
```

## Types

### Type `TcpListener`

Represents a TCP socket listening for incoming connections.

#### Methods

- **`fn bind(host: char*, port: int) -> Result<TcpListener>`**
  Creates a new listener bound to the specified host and port.

- **`fn accept(self) -> Result<TcpStream>`**
  Blocks waiting for a new connection. Returns a `TcpStream` for the connected client.

- **`fn close(self)`**
  Closes the listening socket.

### Type `TcpStream`

Represents a TCP connection stream.

#### Methods

- **`fn connect(host: char*, port: int) -> Result<TcpStream>`**
  Connects to a remote host.

- **`fn read(self, buf: char*, len: usize) -> Result<usize>`**
  Reads up to `len` bytes into `buf`. Returns the number of bytes read.

- **`fn write(self, buf: char*, len: usize) -> Result<usize>`**
  Writes `len` bytes from `buf` to the stream. Returns the number of bytes written.

- **`fn close(self)`**
  Closes the connection.
