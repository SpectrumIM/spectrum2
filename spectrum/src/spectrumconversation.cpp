/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include "spectrumconversation.h"
#include "transport/user.h"

#define Log(X, STRING) std::cout << "[SPECTRUM] " << X << " " << STRING << "\n";

SpectrumConversation::SpectrumConversation(ConversationManager *conversationManager, const std::string &legacyName, PurpleConversation *conv) : AbstractConversation(conversationManager, legacyName), m_conv(conv) {
}

SpectrumConversation::~SpectrumConversation() {
}

void SpectrumConversation::sendMessage(boost::shared_ptr<Swift::Message> &message) {
	
}


