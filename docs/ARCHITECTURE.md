/\*\*

- @file ARCHITECTURE.md
- @brief Clean Architecture Overview for cpp_request
  \*/

# Clean Architecture - cpp_request Library

## Overview

The cpp_request library now follows clean architecture principles with proper separation of concerns and hidden internal implementation details.

## Directory Structure

```
cpp_request/
├── include/cpp_request/          # PUBLIC API (exposed to users)
│   ├── request.h                 # Main public interface
│   ├── error.h                   # Public error types
│
└── src/                           # INTERNAL IMPLEMENTATION (hidden)
    ├── request.cpp              # Public API wrapper implementation
    ├── core/
    │   ├── socket.hpp           # Internal socket wrapper (header)
    │   └── socket.cpp           # Internal socket implementation
    ├── transport/
    │   ├── winsock_init.hpp     # Internal WSA init (header)
    │   └── winsock_init.cpp     # Internal WSA init implementation
    ├── platform/
    │   ├── winsock.hpp          # Internal Windows socket ops (header)
    │   └── winsock.cpp          # Internal Windows socket implementation
    └── main.cpp                 # Example usage
```

## Architecture Layers

### 1. **Public API Layer** (`include/cpp_request/`)

- **Exposure:** PUBLIC - Users only see this
- **Files:** `request.h`, `error.h`
- **Contains:**
  - `HttpClient` class - Main public interface
  - `Error` enum - Error handling
  - Designed for end users

```cpp
// Users only interact with this
zd::crq::HttpClient client;
client.connect("example.com", 80);
client.send(request);
auto response = client.receive_all();
```

### 2. **Implementation Layer** (`src/`)

- **Exposure:** PRIVATE/HIDDEN - Users should NOT see these
- **Subdivided into layers:**

#### Platform Layer (`src/platform/`)

- **Purpose:** Windows socket operations
- **Classes:** `WinSock` (internal)
- **Responsibility:** Connection, send, receive operations
- **Visibility:** Hidden - only accessible internally

#### Transport Layer (`src/transport/`)

- **Purpose:** Socket initialization
- **Classes:** `WinSockInit` (internal)
- **Responsibility:** WSA startup/cleanup
- **Visibility:** Hidden - only accessible internally

#### Core Layer (`src/core/`)

- **Purpose:** Low-level socket operations
- **Classes:** `Socket` (internal)
- **Responsibility:** Socket creation, connection, I/O
- **Visibility:** Hidden - only accessible internally

## Key Clean Architecture Principles Implemented

### 1. **Dependency Inversion**

```
Public API (HttpClient)
    ↓ delegates to
Internal Implementation (WinSock via Impl)
    ↓ uses
Platform Abstraction (Socket wrapper)
    ↓ uses
System APIs (Windows Sockets)
```

### 2. **Hiding Internal Details**

- Internal headers (`.hpp`) are in `src/` directory - NOT in `include/`
- Users can only see what's in `include/cpp_request/`
- Pimpl (Pointer to Implementation) pattern used in `HttpClient`

### 3. **Single Responsibility**

- `Socket` class: Low-level socket operations only
- `WinSockInit` class: Initialization only
- `WinSock` class: High-level I/O operations only
- `HttpClient` class: Public API delegation only

### 4. **Open/Closed Principle**

- Easy to extend with new transports (could add Unix sockets, etc.)
- No need to modify public API

## Build Configuration

The CMakeLists.txt properly manages visibility:

```cmake
# Public headers - users can include these
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Internal headers - only for implementation
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# All internal implementations linked
set(CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/core/socket.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/winsock_init.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/winsock.cpp
    ${CMAKE_SOURCE_DIR}/src/request.cpp
)
```

## Usage Example

```cpp
#include "cpp_request/request.h"

int main() {
    // Only public API visible here
    zd::crq::HttpClient client;

    auto result = client.connect("example.com", 80);
    if (result) {
        // Send request, receive response, etc.
    }

    // All internal classes (WinSock, Socket, WinSockInit) are hidden
    // Users cannot directly access them
}
```

## Benefits

✅ **Clean separation of concerns** - Each layer has one responsibility
✅ **Hidden implementation details** - Internal APIs are inaccessible
✅ **Easy to maintain** - Changes to internals don't affect users
✅ **Easy to test** - Clear interfaces between layers
✅ **Easy to extend** - New platforms can be added without changing public API
✅ **Proper encapsulation** - No leaked internal symbols
✅ **Professional structure** - Follows industry best practices

## Conclusion

The refactored cpp_request library now demonstrates clean architecture principles with:

- Proper layering (Platform → Transport → Core → Public API)
- Hidden internal implementation details
- Clear separation between public and private interfaces
- Professional build configuration
- Easy to understand and maintain codebase
