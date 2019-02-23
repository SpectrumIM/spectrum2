#pragma once

#include <vector>
#include "Swiften/Queries/SetResponder.h"
#include "Swiften/Elements/CarbonsEnable.h"
#include "Swiften/Elements/CarbonsDisable.h"
#include "Swiften/SwiftenCompat.h"

#include "discoinforesponder.h"

namespace Transport {

typedef Swift::SetResponder<Swift::CarbonsEnable> CarbonsEnableResponder;
typedef Swift::SetResponder<Swift::CarbonsDisable> CarbonsDisableResponder;

class CarbonResponder : public CarbonsEnableResponder, public CarbonsDisableResponder {
	public:
		CarbonResponder(Swift::IQRouter *router);
		~CarbonResponder();
		void start();
		void setDiscoInfoResponder(DiscoInfoResponder *discoInfoResponder); //can be after start()

	private:
		virtual bool handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::CarbonsEnable> payload);
		virtual bool handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::CarbonsDisable> payload);
};

}
