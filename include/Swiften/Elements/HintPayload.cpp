/*
 * Implements XEP-0334: Message Processing Hints
 * Licensed under the Simplified BSD license.
 * See Documentation/Licenses/BSD-simplified.txt for more information.
 */

#include <Swiften/Elements/HintPayload.h>

namespace Swift {

HintPayload::HintPayload(Type type)
: type_(type) {
}

}
