## Packets
turntable communicates over port 14135. Packets must be at least as long as the header.

turntable's packet header is as follows:

| Bits     | Description        |
|----------|--------------------|
| `00-02`  | type               |
| `03-07`  | type-specific info |
| variable | data               |

### Packet types
There are currently three packet types:

| Bit Pattern | Type       |
|-------------|------------|
| `000`       | Command    |
| `001`       | Audio file |
| `010`       | Error      |

### Type-specific info
For the command type, there is the following info:

| Bit Pattern | Meaning |
|-------------|---------|
| `00000`     | Play    |
| `00001`     | Pause   |
| `00010`     | Skip    |

For the audio file type, there is the following info:

| Bit Pattern | Meaning          | Data               |
|-------------|------------------|--------------------|
| `00000`     | Start of data    | See "Data" section |
| `00001`     | Middle of data   | See "Data" section |
| `00010`     | End of data      | See "Data" section |
| `00011`     | Request to play  | File name          |
| `00100`     | Request for data | File name          |
| `00101`     | Clear queue      | N/A                |

For the error type, there is the following info:

| Bit Pattern | Meaning        |
|-------------|----------------|
| `00000`     | Invalid packet |
| `00001`     | File not found |

### Data
For the "\* of data" types, the first byte is the length of the index specifier in bytes.
The following [index length] bytes specify the index of the packet in the data stream in little-endian.
The remaining bytes in the packet are the file data.
It is ill-formed to change the index length during a single file transmission, and conforming implementations
may handle this case however they prefer.
