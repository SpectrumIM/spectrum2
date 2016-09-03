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

#include "formutils.h"
#include "adhoccommand.h"

#include "Swiften/SwiftenCompat.h"

#if HAVE_SWIFTEN_3
#include <Swiften/Elements/Form.h>
#endif

using namespace Swift;

namespace Transport {
namespace FormUtils {

static 
#if HAVE_SWIFTEN_3
FormField::ref
#else
HiddenFormField::ref
#endif
createHiddenField(const std::string &name, const std::string &value) {
#if HAVE_SWIFTEN_3
	FormField::ref field = SWIFTEN_SHRPTR_NAMESPACE::make_shared<FormField>(FormField::HiddenType, value);
#else
	HiddenFormField::ref field = HiddenFormField::create();
	field->setValue(value);
#endif
	field->setName(name);
	return field;
}

static 
#if HAVE_SWIFTEN_3
FormField::ref
#else
TextSingleFormField::ref
#endif
createTextSingleField(const std::string &name, const std::string &value, const std::string &label, bool required) {
#if HAVE_SWIFTEN_3
	FormField::ref field = SWIFTEN_SHRPTR_NAMESPACE::make_shared<FormField>(FormField::TextSingleType, value);
#else
	TextSingleFormField::ref field = TextSingleFormField::create();
	field->setValue(value);
#endif
	field->setName(name);
	field->setLabel(label);
	field->setRequired(required);
	return field;
}

static 
#if HAVE_SWIFTEN_3
FormField::ref
#else
TextPrivateFormField::ref
#endif
createTextPrivateField(const std::string &name, const std::string &label, bool required) {
#if HAVE_SWIFTEN_3
	FormField::ref field = SWIFTEN_SHRPTR_NAMESPACE::make_shared<FormField>(FormField::TextPrivateType);
#else
	TextPrivateFormField::ref field = TextPrivateFormField::create();
#endif
	field->setName(name);
	field->setLabel(label);
	field->setRequired(required);
	return field;
}

static 
#if HAVE_SWIFTEN_3
FormField::ref
#else
ListSingleFormField::ref
#endif
createListSingleField(const std::string &name, Swift::FormField::Option value, const std::string &label, const std::string &def, bool required) {
#if HAVE_SWIFTEN_3
	FormField::ref field = SWIFTEN_SHRPTR_NAMESPACE::make_shared<FormField>(FormField::ListSingleType);
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
	return field;
}

static 
#if HAVE_SWIFTEN_3
FormField::ref
#else
BooleanFormField::ref
#endif
createBooleanField(const std::string &name, const std::string &value, const std::string &label, bool required) {
#if HAVE_SWIFTEN_3
	FormField::ref field = SWIFTEN_SHRPTR_NAMESPACE::make_shared<FormField>(FormField::BooleanType, value);
#else
	BooleanFormField::ref field = BooleanFormField::create();
	field->setValue(value == "1");
#endif
	field->setName(name);
	field->setLabel(label);
	field->setRequired(required);
	return field;
}

static 
#if HAVE_SWIFTEN_3
FormField::ref
#else
FixedFormField::ref
#endif
createTextFixedField(const std::string &value) {
#if HAVE_SWIFTEN_3
	FormField::ref field = SWIFTEN_SHRPTR_NAMESPACE::make_shared<FormField>(FormField::FixedType, value);
#else
	FixedFormField::ref field = FixedFormField::create(value);
#endif
	return field;
}

void addHiddenField(Form::ref form, const std::string &name, const std::string &value) {
	form->addField(createHiddenField(name, value));
}

void addTextSingleField(Swift::Form::ref form, const std::string &name, const std::string &value, const std::string &label, bool required) {
	form->addField(createTextSingleField(name, value, label, required));
}

void addTextPrivateField(Swift::Form::ref form, const std::string &name, const std::string &label, bool required) {
	form->addField(createTextPrivateField(name, label, required));
}

void addListSingleField(Swift::Form::ref form, const std::string &name, Swift::FormField::Option value, const std::string &label, const std::string &def, bool required) {
	form->addField(createListSingleField(name, value, label, def, required));
}

void addBooleanField(Swift::Form::ref form, const std::string &name, const std::string &value, const std::string &label, bool required) {
	form->addField(createBooleanField(name, value, label, required));
}

void addTextFixedField(Swift::Form::ref form, const std::string &value) {
	form->addField(createTextFixedField(value));
}


std::string fieldValue(Swift::FormField::ref field) {
#if HAVE_SWIFTEN_3
	const std::vector<std::string> values = field->getValues();
	return values.empty() ? "" : values[0];
#else
	TextSingleFormField::ref textSingle = std::dynamic_pointer_cast<TextSingleFormField>(field);
	if (textSingle) {
		return textSingle->getValue();
	}

	TextPrivateFormField::ref textPrivate = std::dynamic_pointer_cast<TextPrivateFormField>(field);
	if (textPrivate) {
		return textPrivate->getValue();
	}

	ListSingleFormField::ref listSingle = std::dynamic_pointer_cast<ListSingleFormField>(field);
	if (listSingle) {
		return listSingle->getValue();
	}

	BooleanFormField::ref boolean = std::dynamic_pointer_cast<BooleanFormField>(field);
	if (boolean) {
		return boolean->getValue() ? "1" : "0";
	}
	
	return "";
#endif
}

std::string fieldValue(Swift::Form::ref form, const std::string &key, const std::string &def) {
	const std::vector<FormField::ref> fields = form->getFields();
	for (std::vector<FormField::ref>::const_iterator it = fields.begin(); it != fields.end(); it++) {
#if HAVE_SWIFTEN_3
		FormField::ref field = *it;
		const std::vector<std::string> values = field->getValues();
		if (field->getName() == key) {
			return values.empty() ? "" : values[0];
		}
#else
		TextSingleFormField::ref textSingle = std::dynamic_pointer_cast<TextSingleFormField>(*it);
		if (textSingle && textSingle->getName() == key) {
			return textSingle->getValue();
		}

		TextPrivateFormField::ref textPrivate = std::dynamic_pointer_cast<TextPrivateFormField>(*it);
		if (textPrivate && textPrivate->getName() == key) {
			return textPrivate->getValue();
		}

		ListSingleFormField::ref listSingle = std::dynamic_pointer_cast<ListSingleFormField>(*it);
		if (listSingle && listSingle->getName() == key) {
			return listSingle->getValue();
		}

		BooleanFormField::ref boolean = std::dynamic_pointer_cast<BooleanFormField>(*it);
		if (boolean && boolean->getName() == key) {
			return boolean->getValue() ? "1" : "0";
		}
#endif
	}

	return def;
}


}
}
