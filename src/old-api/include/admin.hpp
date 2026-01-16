#pragma once

#include "_internal.hpp"

namespace globed::admin {
// Returns whether the user is connected to a server, and has moderation powers on the server.
Result<bool> isModerator();

// Returns whether the user is connected to a server, has moderation powers on the server, and is logged into the admin
// panel.
Result<bool> isAuthorizedModerator();

// Opens the moderation panel. Does nothing if the user is not a moderator.
Result<void> openModPanel();
} // namespace globed::admin

// Implementation

namespace globed::admin {
inline Result<bool> isModerator()
{
    return _internal::request<bool>(_internal::Type::IsMod);
}

inline Result<bool> isAuthorizedModerator()
{
    return _internal::request<bool>(_internal::Type::IsAuthMod);
}

inline Result<void> openModPanel()
{
    return _internal::requestVoid(_internal::Type::OpenModPanel);
}
} // namespace globed::admin