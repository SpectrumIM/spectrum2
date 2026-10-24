/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace Transport {

/// Single-listener event. Holds at most one listener; assign a lambda to
/// subscribe and nullptr to unsubscribe. Calling an event with no listener
/// does nothing, so emit sites are a plain call.
///
/// The constructors and assignments are written out (not inherited from
/// std::function) so that an Event is never wrapped inside another std::function.
template <typename Sig>
class Event : public std::function<Sig> {
	public:
		Event() = default;
		Event(std::nullptr_t) {}

		template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, Event>>>
		Event(F &&f) : std::function<Sig>(std::forward<F>(f)) {}

		Event &operator=(std::nullptr_t) {
			std::function<Sig>::operator=(nullptr);
			return *this;
		}

		template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, Event>>>
		Event &operator=(F &&f) {
			std::function<Sig>::operator=(std::forward<F>(f));
			return *this;
		}

		template <typename... Args>
		void operator()(Args &&...args) const {
			if (*this) {
				std::function<Sig>::operator()(std::forward<Args>(args)...);
			}
		}
};

}