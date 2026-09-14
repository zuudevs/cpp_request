#include <gtest/gtest.h>

#include "platform/native_socket.hpp"

#include <type_traits>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

using cpp_request::detail::platform::NativeSocket;
using cpp_request::detail::platform::NativeSocketHandle;
using cpp_request::detail::platform::kInvalidSocket;

static_assert(!std::is_copy_constructible_v<NativeSocket>);
static_assert(!std::is_copy_assignable_v<NativeSocket>);
static_assert(std::is_nothrow_move_constructible_v<NativeSocket>);
static_assert(std::is_nothrow_move_assignable_v<NativeSocket>);

#ifdef _WIN32
class WinsockScope final {
public:
    WinsockScope() noexcept {
        WSADATA data{};
        ready_ = ::WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }

    ~WinsockScope() noexcept {
        if (ready_) {
            ::WSACleanup();
        }
    }

    [[nodiscard]] bool ready() const noexcept {
        return ready_;
    }

private:
    bool ready_{false};
};
#endif

NativeSocketHandle create_test_socket() {
#ifdef _WIN32
    static WinsockScope winsock;
    if (!winsock.ready()) {
        return kInvalidSocket;
    }
    return ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
    return ::socket(AF_INET, SOCK_STREAM, 0);
#endif
}

bool is_closed(NativeSocketHandle handle) {
#ifdef _WIN32
    int socket_type = 0;
    int length = sizeof(socket_type);
    const int result = ::getsockopt(
        handle,
        SOL_SOCKET,
        SO_TYPE,
        reinterpret_cast<char*>(&socket_type),
        &length);
    return result == SOCKET_ERROR && ::WSAGetLastError() == WSAENOTSOCK;
#else
    errno = 0;
    return ::fcntl(handle, F_GETFD) == -1 && errno == EBADF;
#endif
}

TEST(NativeSocketTest, DefaultConstructedSocketIsInvalid) {
    const NativeSocket socket;

    EXPECT_FALSE(socket.valid());
    EXPECT_FALSE(static_cast<bool>(socket));
    EXPECT_EQ(socket.get(), kInvalidSocket);
}

TEST(NativeSocketTest, MoveConstructionTransfersOwnership) {
    const NativeSocketHandle raw = create_test_socket();
    ASSERT_NE(raw, kInvalidSocket);

    NativeSocket source{raw};
    NativeSocket destination{std::move(source)};

    EXPECT_FALSE(source.valid());
    EXPECT_TRUE(destination.valid());
    EXPECT_EQ(destination.get(), raw);
}

TEST(NativeSocketTest, MoveAssignmentClosesPreviousHandleAndTransfersOwnership) {
    const NativeSocketHandle first = create_test_socket();
    const NativeSocketHandle second = create_test_socket();
    ASSERT_NE(first, kInvalidSocket);
    ASSERT_NE(second, kInvalidSocket);

    NativeSocket destination{first};
    NativeSocket source{second};

    destination = std::move(source);

    EXPECT_TRUE(is_closed(first));
    EXPECT_FALSE(source.valid());
    EXPECT_EQ(destination.get(), second);
}

TEST(NativeSocketTest, ReleaseReturnsHandleWithoutClosingIt) {
    const NativeSocketHandle raw = create_test_socket();
    ASSERT_NE(raw, kInvalidSocket);

    NativeSocket socket{raw};
    const NativeSocketHandle released = socket.release();

    EXPECT_EQ(released, raw);
    EXPECT_FALSE(socket.valid());
    EXPECT_FALSE(is_closed(raw));

#ifdef _WIN32
    EXPECT_EQ(::closesocket(raw), 0);
#else
    EXPECT_EQ(::close(raw), 0);
#endif
}

TEST(NativeSocketTest, ResetClosesCurrentHandle) {
    const NativeSocketHandle raw = create_test_socket();
    ASSERT_NE(raw, kInvalidSocket);

    NativeSocket socket{raw};
    socket.reset();

    EXPECT_FALSE(socket.valid());
    EXPECT_TRUE(is_closed(raw));
}

TEST(NativeSocketTest, DestructorClosesOwnedHandle) {
    const NativeSocketHandle raw = create_test_socket();
    ASSERT_NE(raw, kInvalidSocket);

    {
        NativeSocket socket{raw};
        ASSERT_TRUE(socket.valid());
    }

    EXPECT_TRUE(is_closed(raw));
}

} // namespace
