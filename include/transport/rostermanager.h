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

#pragma once

#include <string>
#include <algorithm>
#include <map>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/object_pool.hpp>
#include "Swiften/Swiften.h"
// #include "rosterstorage.h"

namespace Transport {

class Buddy;
class User;
class Component;
class StorageBackend;
class RosterStorage;

// TODO: Once Swiften GetRosterRequest will support setting to="", this can be removed
class AddressedRosterRequest : public Swift::GenericRequest<Swift::RosterPayload> {
	public:
		typedef boost::shared_ptr<AddressedRosterRequest> ref;

		AddressedRosterRequest(Swift::IQRouter* router, Swift::JID to) :
				Swift::GenericRequest<Swift::RosterPayload>(Swift::IQ::Get, to, boost::shared_ptr<Swift::Payload>(new Swift::RosterPayload()), router) {
		}
};

/// Manages roster of one XMPP user.
class RosterManager {
	public:
		typedef std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<std::string, Buddy *> > > BuddiesMap;
		/// Creates new RosterManager.
		/// \param user User associated with this RosterManager.
		/// \param component Transport instance associated with this roster.
		RosterManager(User *user, Component *component);

		/// Destructor.
		virtual ~RosterManager();

		/// Associates the buddy with this roster,
		/// and if the buddy is not already in XMPP user's server-side roster, the proper requests
		/// are sent to XMPP user (subscribe presences, Roster Item Exchange stanza or
		/// the buddy is added to server-side roster using remote-roster protoXEP).
		/// \param buddy Buddy
		void setBuddy(Buddy *buddy);

		/// Deassociates the buddy with this roster.
		/// \param buddy Buddy.
		void unsetBuddy(Buddy *buddy);

		Buddy *getBuddy(const std::string &name);

		void setStorageBackend(StorageBackend *storageBackend);

		void storeBuddy(Buddy *buddy);

		Swift::RosterPayload::ref generateRosterPayload();

		/// Returns user associated with this roster.
		/// \return User
		User *getUser() { return m_user; }

		const BuddiesMap &getBuddies() {
			return m_buddies;
		}

		bool isRemoteRosterSupported() {
			return m_supportRemoteRoster;
		}

		/// Called when new Buddy is added to this roster.
		/// \param buddy newly added Buddy
		boost::signal<void (Buddy *buddy)> onBuddySet;

		/// Called when Buddy has been removed from this roster.
		/// \param buddy removed Buddy
		boost::signal<void (Buddy *buddy)> onBuddyUnset;

		boost::signal<void (Buddy *buddy)> onBuddyAdded;
		
		boost::signal<void (Buddy *buddy)> onBuddyRemoved;

		void handleBuddyChanged(Buddy *buddy);

		void handleSubscription(Swift::Presence::ref presence);

		void sendBuddyRosterPush(Buddy *buddy);

		void sendBuddySubscribePresence(Buddy *buddy);

		void sendCurrentPresences(const Swift::JID &to);

		void sendCurrentPresence(const Swift::JID &from, const Swift::JID &to);

		void sendUnavailablePresences(const Swift::JID &to);

	private:
		void setBuddyCallback(Buddy *buddy);

		void sendRIE();
		void handleBuddyRosterPushResponse(Swift::ErrorPayload::ref error, Swift::SetRosterRequest::ref request, const std::string &key);
		void handleRemoteRosterResponse(boost::shared_ptr<Swift::RosterPayload> roster, Swift::ErrorPayload::ref error);

		std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<std::string, Buddy *> > > m_buddies;
		Component *m_component;
		RosterStorage *m_rosterStorage;
		User *m_user;
		Swift::Timer::ref m_setBuddyTimer;
		Swift::Timer::ref m_RIETimer;
		std::list <Swift::SetRosterRequest::ref> m_requests;
		bool m_supportRemoteRoster;
		AddressedRosterRequest::ref m_remoteRosterRequest;
};

}
