/**
 * @file error.h
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Public error types for cpp_request
 * @version 0.2.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include <cstdint>
#include <winsock2.h>

namespace zd::crq {

enum class Error : uint8_t {
    None,

    // WSAStartup
    SystemIsNotReady,
    VersionUnsupported,
    NotInitialized,

    // Resource & state
    NotEnoughMemory,
    OperationInterrupted,
    OperationInProgress,
    OperationAlreadyInProgress,
    ProcessLimitReached,
    NoBufferSpace,

    // Address
    BadAddress,
    AddressInUse,
    AddressNotAvailable,
    AddressFamilyUnsupported,

    // Socket
    NotASocket,
    SocketTypeUnsupported,
    AlreadyConnected,
    ListeningSocket,
    InvalidSocket,
    SocketShutdown,

    // Network
    NetworkDown,
    NetworkUnreachable,
    NetworkReset,
    HostUnreachable,

    // Connection
    NotConnected,
    ConnectionRefused,
    ConnectionTimedOut,
    ConnectionAborted,
    ConnectionReset,

    // Name resolution
    HostNotFound,
    NoData,
    NoRecovery,
    TryAgain,
    ServiceTypeNotFound,

    // Arguments
    InvalidArgument,

    // Permissions
    AccessDenied,

    // Non-blocking
    WouldBlock,

    // Data
    MessageTooLarge,

    Unknown,
};

/// Convert a raw WinSock error code (from WSAGetLastError / getaddrinfo) into Error.
constexpr Error to_error(int errc) noexcept {
    switch (errc) {
    case 0:                      return Error::None;
    case WSASYSNOTREADY:         return Error::SystemIsNotReady;
    case WSAVERNOTSUPPORTED:     return Error::VersionUnsupported;
    case WSAEINPROGRESS:         return Error::OperationInProgress;
    case WSAEPROCLIM:            return Error::ProcessLimitReached;
    case WSAEFAULT:              return Error::BadAddress;
    case WSA_NOT_ENOUGH_MEMORY:  return Error::NotEnoughMemory;
    case WSAEAFNOSUPPORT:        return Error::AddressFamilyUnsupported;
    case WSAEINVAL:              return Error::InvalidArgument;
    case WSAESOCKTNOSUPPORT:     return Error::SocketTypeUnsupported;
    case WSAHOST_NOT_FOUND:      return Error::HostNotFound;
    case WSANO_DATA:             return Error::NoData;
    case WSANO_RECOVERY:         return Error::NoRecovery;
    case WSANOTINITIALISED:      return Error::NotInitialized;
    case WSATRY_AGAIN:           return Error::TryAgain;
    case WSATYPE_NOT_FOUND:      return Error::ServiceTypeNotFound;
    case WSAENETDOWN:            return Error::NetworkDown;
    case WSAEADDRINUSE:          return Error::AddressInUse;
    case WSAEINTR:               return Error::OperationInterrupted;
    case WSAEALREADY:            return Error::OperationAlreadyInProgress;
    case WSAEADDRNOTAVAIL:       return Error::AddressNotAvailable;
    case WSAECONNREFUSED:        return Error::ConnectionRefused;
    case WSAEISCONN:             return Error::AlreadyConnected;
    case WSAENETUNREACH:         return Error::NetworkUnreachable;
    case WSAEHOSTUNREACH:        return Error::HostUnreachable;
    case WSAENOBUFS:             return Error::NoBufferSpace;
    case WSAENOTSOCK:            return Error::NotASocket;
    case WSAETIMEDOUT:           return Error::ConnectionTimedOut;
    case WSAEWOULDBLOCK:         return Error::WouldBlock;
    case WSAEACCES:              return Error::AccessDenied;
    case WSA_INVALID_HANDLE:     return Error::InvalidSocket;
    case WSAENETRESET:           return Error::NetworkReset;
    case WSAENOTCONN:            return Error::NotConnected;
    case WSAESHUTDOWN:           return Error::SocketShutdown;
    case WSAEMSGSIZE:            return Error::MessageTooLarge;
    case WSAECONNABORTED:        return Error::ConnectionAborted;
    case WSAECONNRESET:          return Error::ConnectionReset;
    default:                     return Error::Unknown;
    }
}

} // namespace zd::crq