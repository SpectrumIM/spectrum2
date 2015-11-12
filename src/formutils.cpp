/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2015, Jan Kaluza <hanzz.k@gmail.com>
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

#include "transport/formutils.h"

#if HAVE_SWIFTEN_3
#include <Swiften/Elements/Form.h>
#endif

using namespace Swift;

namespace Transport {
namespace FormUtils {

void addHiddenField(Form::ref form, const std::string &name, const std::string &value) {
#if HAVE_SWIFTEN_3
	FormField::ref field = boost::make_shared<FormField>(FormField::HiddenType, value);	
#else
	HiddenFormField::ref field = HiddenFormField::create();
	field->setValue(value);
#endif
	field->setName(name);
	form->addField(field);
}

void addTextSingleField(Swift::Form::ref form, const std::string &name, const std::string &value, const std::string &label, bool required) {
#if HAVE_SWIFTEN_3
	FormField::ref field = boost::make_shared<FormField>(FormField::TextSingleType, value);
#else
	TextSingleFormField::ref field = TextSingleFormField::create();
	field->setValue(value);
#endif
	field->setName(name);
	field->setLabel(label);
	field->setRequired(required);
	form->addField(field);
}

void addTextPrivateField(Swift::Form::ref form, const std::string &name, const std::string &label, bool required) {
#if HAVE_SWIFTEN_3
	FormField::ref field = boost::make_shared<FormField>(FormField::TextPrivateType);
#else
	TextPrivateFormField::ref field = TextPrivateFormField::create();
#endif
	field->setName(name);
	field->setLabel(label);
	field->setRequired(required);
	form->addField(field);
}

void addListSingleField(Swift::Form::ref form, const std::string &name, Swift::FormField::Option value, const std::string &label, const std::string &def, bool required) {
#if HAVE_SWIFTEN_3
	FormField::ref field = boost::make_shared<FormField>(FormField::ListSingleType);
#else
	ListSingleFormField::ref field = ListSingleFormField::create();
#endif
	field->setName(name);
	field->setLabel(label);
	field->addOption(value);
#if HAVE_SWIFTEN_3
	field->addValue(def);
#else
	field->setValue(def);
#endif
	form->addField(field);
}


void addBooleanField(Swift::Form::ref form, const std::string &name, const std::string &value, const std::string &label, bool required) {
#if HAVE_SWIFTEN_3
	FormField::ref field = boost::make_shared<FormField>(FormField::BooleanType, value);
#else
	BooleanFormField::ref field = BooleanFormField::create();
	field->setValue(value == "0");
#endif
	field->setName(name);
	field->setLabel(label);
	field->setRequired(required);
	form->addField(field);
}

std::string fieldValue(Swift::Form::ref form, const std::string &key, const std::string &def) {
	const std::vector<FormField::ref> fields = form->getFields();
	for (std::vector<FormField::ref>::const_iterator it = fields.begin(); it != fields.end(); it++) {
#if HAVE_SWIFTEN_3
		FormField::ref field = *it;
		if (field->getName() == key) {
			return field->getValues()[0];
		}
#else
		TextSingleFormField::ref textSingle = boost::dynamic_pointer_cast<TextSingleFormField>(*it);
		if (textSingle && textSingle->getName() == key) {
			return textSingle->getValue();
		}

		TextPrivateFormField::ref textPrivate = boost::dynamic_pointer_cast<TextPrivateFormField>(*it);
		if (textPrivate && textPrivate->getName() == key) {
			return textPrivate->getValue();
		}

		ListSingleFormField::ref listSingle = boost::dynamic_pointer_cast<ListSingleFormField>(*it);
		if (listSingle && listSingle->getName() == key) {
			return listSingle->getValue();
		}

		BooleanFormField::ref boolean = boost::dynamic_pointer_cast<BooleanFormField>(*it);
		if (boolean && boolean->getName() == key) {
			return boolen->getValue() ? "1" : "0";
		}
#endif
	}

	return def;
}


}
}
