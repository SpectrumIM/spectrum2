/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <Swiften/FileTransfer/MyOutgoingSIFileTransfer.h>

#include <boost/bind.hpp>

#include <Swiften/FileTransfer/StreamInitiationRequest.h>
#include <Swiften/FileTransfer/BytestreamsRequest.h>
#include <Swiften/FileTransfer/SOCKS5BytestreamServer.h>
#include <Swiften/FileTransfer/SOCKS5BytestreamRegistry.h>
#include <Swiften/FileTransfer/IBBSendSession.h>

namespace Swift {

MyOutgoingSIFileTransfer::MyOutgoingSIFileTransfer(const std::string& id, const JID& from, const JID& to, const std::string& name, int size, const std::string& description, std::shared_ptr<ReadBytestream> bytestream, IQRouter* iqRouter, SOCKS5BytestreamServer* socksServer, SOCKS5BytestreamRegistry* registry) : id(id), from(from), to(to), name(name), size(size), description(description), bytestream(bytestream), iqRouter(iqRouter), socksServer(socksServer), registry(registry) {
}

void MyOutgoingSIFileTransfer::start() {
	StreamInitiation::ref streamInitiation(new StreamInitiation());
	streamInitiation->setID(id);
	streamInitiation->setFileInfo(StreamInitiationFileInfo(name, description, size));
	streamInitiation->addProvidedMethod("http://jabber.org/protocol/bytestreams");
	streamInitiation->addProvidedMethod("http://jabber.org/protocol/ibb");
	StreamInitiationRequest::ref request = StreamInitiationRequest::create(from, to, streamInitiation, iqRouter);
	request->onResponse.connect(boost::bind(&MyOutgoingSIFileTransfer::handleStreamInitiationRequestResponse, this, _1, _2));
	request->send();
}

void MyOutgoingSIFileTransfer::stop() {
}

void MyOutgoingSIFileTransfer::cancel() {
	// TODO
// 	session->sendTerminate(JinglePayload::Reason::Cancel);

	if (ibbSession) {
		ibbSession->stop();
	}
}

void MyOutgoingSIFileTransfer::handleStreamInitiationRequestResponse(StreamInitiation::ref response, ErrorPayload::ref error) {
	if (error) {
		finish(FileTransferError());
	}
	else {
		if (response->getRequestedMethod() == "http://jabber.org/protocol/bytestreams") {
			Bytestreams::ref bytestreams(new Bytestreams());
			bytestreams->setStreamID(id);
			HostAddressPort addressPort = socksServer->getAddressPort();
			bytestreams->addStreamHost(Bytestreams::StreamHost(addressPort.getAddress().toString(), from, addressPort.getPort()));
			BytestreamsRequest::ref request = BytestreamsRequest::create(from, to, bytestreams, iqRouter);
			request->onResponse.connect(boost::bind(&MyOutgoingSIFileTransfer::handleBytestreamsRequestResponse, this, _1, _2));
			request->send();
		}
		else if (response->getRequestedMethod() == "http://jabber.org/protocol/ibb") {
			ibbSession = std::shared_ptr<IBBSendSession>(new IBBSendSession(id, from, to, bytestream, iqRouter));
			ibbSession->onFinished.connect(boost::bind(&MyOutgoingSIFileTransfer::handleIBBSessionFinished, this, _1));
			ibbSession->start();
		}
	}
}

void MyOutgoingSIFileTransfer::handleBytestreamsRequestResponse(Bytestreams::ref, ErrorPayload::ref error) {
	if (error) {
		finish(FileTransferError());
		return;
	}
	//socksServer->onTransferFinished.connect();
}

void MyOutgoingSIFileTransfer::finish(boost::optional<FileTransferError> error) {
	if (ibbSession) {
		ibbSession->onFinished.disconnect(boost::bind(&MyOutgoingSIFileTransfer::handleIBBSessionFinished, this, _1));
		ibbSession.reset();
	}
	onFinished(error);
}

void MyOutgoingSIFileTransfer::handleIBBSessionFinished(boost::optional<FileTransferError> error) {
	finish(error);
}

}
