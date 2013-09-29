// Copyright (c) 2013 The Berkelium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Berkelium/API/Util.hpp>
#include <Berkelium/API/Window.hpp>
#include <Berkelium/API/Tab.hpp>
#include <Berkelium/API/Runtime.hpp>
#include <Berkelium/API/Logger.hpp>
#include <Berkelium/IPC/Message.hpp>
#include <Berkelium/Impl/Impl.hpp>
#include <Berkelium/Impl/Manager.hpp>

namespace Berkelium {

Tab::Tab() {
}

Tab::~Tab() {
}

namespace impl {

class TabImpl : public Tab {
BERKELIUM_IMPL_CLASS(Tab)

private:
	TabWRef self;
	Ipc::ChannelRef send;
	Ipc::ChannelRef recv;
	Ipc::MessageRef message;
	WindowRef parent;

public:
	TabImpl(WindowRef parent, Ipc::ChannelRef ipc) :
		BERKELIUM_IMPL_CTOR3(Tab, ipc->getName(), parent),
		self(),
		send(ipc),
		recv(ipc->getReverseChannel()),
		message(ipc->getMessage()),
		parent(parent) {
	}

	virtual ~TabImpl() {
		manager->unregisterTab();
	}

	virtual void internalUpdate() {
		if(!recv->isEmpty()) {
			recv->recv(message);
			if(message->length() == 0) {
				// only an ack..
			} else {
				switch(Ipc::CommandId cmd = message->get_cmd()) {
					default: {
						logger->error() << "Tab: received unknown command '" << cmd << "'" << std::endl;
						break;
					}
					case Ipc::CommandId::onReady: {
						message->reset();
						message->add_cmd(Ipc::CommandId::navigate);
						message->add_str("http://heise.de/");
						logger->debug() << "sending navigate to heise.de!" << std::endl;
						send->send(message);
						break;
					}
				}
			}
		}
	}

	virtual void close() {
	}

	virtual void sync() {
	}

	virtual TabDelegateRef getTabDelegate() {
		return TabDelegateRef();
	}

	virtual void addTabDelegate(TabDelegateRef delegate) {
	}

	virtual void removeTabDelegate(TabDelegateRef delegate) {
	}

	virtual WindowRef getWindow() {
		return parent;
	}

	virtual void resize(int32_t width, int32_t height) {
	}

	virtual void navigateTo(const std::string& url) {
	}

	TabRef getSelf() 	{
		return self.lock();
	}

	static TabRef newTab(WindowRef window, Ipc::ChannelRef ipc) {
		TabImpl* impl = new TabImpl(window, ipc);
		TabRef ret(impl);
		impl->self = ret;
		impl->manager->registerTab(ret);
		return ret;
	}
};

ManagerRef getManager(Tab* tab)
{
	if(!tab) {
		bk_error("getManager(Tab* = null)");
		return ManagerRef();
	}
	TabImpl* impl = (TabImpl*)tab;
	return impl->getManager();
}

TabRef newTab(WindowRef window, Ipc::ChannelRef ipc) {
	return TabImpl::newTab(window, ipc);
}

} // namespace impl

} // namespace Berkelium
