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

#include <iostream>
#include "transport/conversation.h"
#include "transport/conversationmanager.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/buddy.h"
#include "transport/rostermanager.h"
#include "log4cxx/logger.h"

using namespace log4cxx;

namespace Transport {

// static LoggerPtr logger = Logger::getLogger("Conversation");

Conversation::Conversation(ConversationManager *conversationManager, const std::string &legacyName, bool isMUC) : m_conversationManager(conversationManager) {
	m_legacyName = legacyName;
	m_conversationManager->addConversation(this);
	m_muc = isMUC;
	m_jid = m_conversationManager->getUser()->getJID().toBare();
}

Conversation::~Conversation() {
}

void Conversation::setRoom(const std::string &room) {
	m_room = room;
	m_legacyName = m_room + "/" + m_legacyName;
}

void Conversation::handleMessage(boost::shared_ptr<Swift::Message> &message, const std::string &nickname) {
	if (m_muc) {
		message->setType(Swift::Message::Groupchat);
	}
	else {
		message->setType(Swift::Message::Chat);
	}

	std::string n = nickname;
	if (n.empty() && !m_room.empty() && !m_muc) {
		n = m_nickname;
	}

	if (message->getType() != Swift::Message::Groupchat) {
		message->setTo(m_jid);
		// normal message
		if (n.empty()) {
			Buddy *buddy = m_conversationManager->getUser()->getRosterManager()->getBuddy(m_legacyName);
			if (buddy) {
				message->setFrom(buddy->getJID());
			}
			else {
				message->setFrom(Swift::JID(Swift::JID::getEscapedNode(m_legacyName), m_conversationManager->getComponent()->getJID().toBare()));
			}
		}
		// PM message
		else {
			if (m_room.empty()) {
				message->setFrom(Swift::JID(n, m_conversationManager->getComponent()->getJID().toBare(), "user"));
			}
			else {
				message->setFrom(Swift::JID(m_room, m_conversationManager->getComponent()->getJID().toBare(), n));
			}
		}
		m_conversationManager->getComponent()->getStanzaChannel()->sendMessage(message);
	}
	else {
		std::string legacyName = m_legacyName;
		if (legacyName.find_last_of("@") != std::string::npos) {
			legacyName.replace(legacyName.find_last_of("@"), 1, "%"); // OK
		}
		message->setTo(m_jid);
		message->setFrom(Swift::JID(legacyName, m_conversationManager->getComponent()->getJID().toBare(), nickname));
		m_conversationManager->getComponent()->getStanzaChannel()->sendMessage(message);
	}
}

void Conversation::handleParticipantChanged(const std::string &nick, int flag, int status, const std::string &statusMessage, const std::string &newname) {
	std::string nickname = nick;
	Swift::Presence::ref presence = Swift::Presence::create();
	std::string legacyName = m_legacyName;
	if (m_muc) {
		if (legacyName.find_last_of("@") != std::string::npos) {
			legacyName.replace(legacyName.find_last_of("@"), 1, "%"); // OK
		}
	}
	presence->setFrom(Swift::JID(legacyName, m_conversationManager->getComponent()->getJID().toBare(), nickname));
	presence->setTo(m_jid);
	presence->setType(Swift::Presence::Available);

	if (!statusMessage.empty())
		presence->setStatus(statusMessage);

	Swift::StatusShow s((Swift::StatusShow::Type) status);

	if (s.getType() == Swift::StatusShow::None) {
		presence->setType(Swift::Presence::Unavailable);
	}

	presence->setShow(s.getType());

	Swift::MUCUserPayload *p = new Swift::MUCUserPayload ();
	if (m_nickname == nickname) {
		Swift::MUCUserPayload::StatusCode c;
		c.code = 110;
		p->addStatusCode(c);
	}

	
	Swift::MUCItem item;
	
	item.affiliation = Swift::MUCOccupant::Member;
	item.role = Swift::MUCOccupant::Participant;

	if (flag & Moderator) {
		item.affiliation = Swift::MUCOccupant::Admin;
		item.role = Swift::MUCOccupant::Moderator;
	}

	if (!newname.empty()) {
		item.nick = newname;
		Swift::MUCUserPayload::StatusCode c;
		c.code = 303;
		p->addStatusCode(c);
		presence->setType(Swift::Presence::Unavailable);
	}

	p->addItem(item);

	presence->addPayload(boost::shared_ptr<Swift::Payload>(p));
	m_conversationManager->getComponent()->getStanzaChannel()->sendPresence(presence);
	if (!newname.empty()) {
		handleParticipantChanged(newname, flag, status, statusMessage);
	}
}

}
