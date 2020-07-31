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
#include <Swiften/Elements/Form.h>

using namespace Swift;

namespace Transport {
namespace FormUtils {

static FormField::ref createHiddenField(const std::string &name, const std::string &value) {
	FormField::ref field = std::make_shared<FormField>(FormField::HiddenType, value);
	field->setName(name);
	return field;
}

static FormField::ref createTextSingleField(const std::string &name, const std::string &value, const std::string &label, bool required) {
	FormField::ref field = std::make_shared<FormField>(FormField::TextSingleType, value);
	field->setName(name);
	field->setLabel(label);
	field->setRequired(required);
	return field;
}

static FormField::ref createTextPrivateField(const std::string &name, const std::string &label, bool required) {
	FormField::ref field = std::make_shared<FormField>(FormField::TextPrivateType);
	field->setName(name);
	field->setLabel(label);
	field->setRequired(required);
	return field;
}

static FormField::ref createListSingleField(const std::string &name, Swift::FormField::Option value, const std::string &label, const std::string &def, bool required) {
	FormField::ref field = std::make_shared<FormField>(FormField::ListSingleType);
	field->setName(name);
	field->setLabel(label);
	field->addOption(value);
	field->addValue(def);
	return field;
}

static FormField::ref createBooleanField(const std::string &name, const std::string &value, const std::string &label, bool required) {
	FormField::ref field = std::make_shared<FormField>(FormField::BooleanType, value);
	field->setName(name);
	field->setLabel(label);
	field->setRequired(required);
	return field;
}

static FormField::ref createTextFixedField(const std::string &value) {
	FormField::ref field = std::make_shared<FormField>(FormField::FixedType, value);
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
	const std::vector<std::string> values = field->getValues();
	return values.empty() ? "" : values[0];
}

std::string fieldValue(Swift::Form::ref form, const std::string &key, const std::string &def) {
	const std::vector<FormField::ref> fields = form->getFields();
	for (std::vector<FormField::ref>::const_iterator it = fields.begin(); it != fields.end(); it++) {
		FormField::ref field = *it;
		const std::vector<std::string> values = field->getValues();
		if (field->getName() == key) {
			return values.empty() ? "" : values[0];
		}
	}

	return def;
}


}
}
