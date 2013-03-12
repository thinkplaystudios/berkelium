// Copyright (c) 2012 The Berkelium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Berkelium/API/Profile.hpp>
#include <Berkelium/API/Instance.hpp>
#include <Berkelium/API/Util.hpp>
#include <Berkelium/API/LogDelegate.hpp>
#include <Berkelium/IPC/Channel.hpp>
#include <Berkelium/IPC/Message.hpp>
#include <Berkelium/Impl/Process.hpp>
#include <Berkelium/Impl/Impl.hpp>

#include <set>
#include <iostream>

namespace Berkelium {

Instance::Instance() {
}

Instance::~Instance() {
}

namespace impl {

template<typename T>
struct set {
	typedef std::set<T, std::owner_less<T>> type;
};

typedef set<WindowWRef>::type WindowWRefSet;
typedef set<LogDelegateWRef>::type LogDelegateWRefSet;
typedef set<HostDelegateWRef>::type HostDelegateWRefSet;
typedef set<Ipc::ChannelWRef>::type ChannelWRefSet;

class InstanceImpl : public Instance {
	InstanceWRef self;
	HostExecutableRef executable;
	ProfileRef profile;
	Ipc::ChannelRef ipc;
	ProcessRef process;
	WindowWRefSet windows;
	LogDelegateWRefSet logs;
	HostDelegateWRefSet hosts;
	ChannelWRefSet channels;
	std::vector<Ipc::ChannelRef> freeWindowChannels;

public:
	InstanceImpl(HostExecutableRef executable, ProfileRef profile, Ipc::ChannelRef ipc, ProcessRef process) :
		self(),
		executable(executable),
		profile(profile),
		ipc(ipc),
		process(process),
		windows(),
		logs(),
		hosts(),
		channels(),
		freeWindowChannels() {
		channels.insert(ipc);
	}

	~InstanceImpl() {
		// TODO only call close if ipc is open...
		close();
		if(profile->isInUse()) {
			std::cerr << "waiting for profile..." << std::endl;
			while(profile->isInUse()) {
				Util::sleep(100);
			}
			std::cerr << "profile closed!" << std::endl;
		}
	}

	void setSelf(InstanceRef instance) {
		self = instance;
	}

	virtual void close() {
		Ipc::MessageRef msg = Ipc::Message::create();
		msg->add_str("exit");
		ipc->send(msg);
		//ipc->recv(msg); //ACK
	}

	virtual void update() {
		for(std::set<Ipc::ChannelWRef>::iterator it = channels.begin(); it != channels.end(); it++) {
			Ipc::ChannelWRef ref = *it;
			if(ref.expired()) {
				it = channels.erase(it);
			}
		}
		ChannelWRefSet copy(channels);
		for(std::set<Ipc::ChannelWRef>::iterator it = copy.begin(); it != copy.end(); it++) {
			Ipc::ChannelWRef ref = *it;
			if(ref.expired()) {
				continue;
			}
			Ipc::ChannelRef ir = Ipc::ChannelRef(ref);
			if(!ir->isEmpty()) {
				Ipc::MessageRef msg = Ipc::Message::create();
				ir->recv(msg);
				std::string str = msg->get_str();
				if(str.compare("addWindow") == 0) {
					std::string id = msg->get_str();
					std::cerr << "new window: '" << id << "'!" << std::endl;
					Ipc::ChannelRef c = ipc->getSubChannel(id);
					channels.insert(c);
					freeWindowChannels.push_back(c);
				} else if(str.compare("OnReady") == 0) {
					msg->reset();
					msg->add_str("Navigate");
					msg->add_str("http://heise.de/");
					std::cerr << "sending navigate to heise.de!" << std::endl;
					ir->send(msg);
				} else {
					std::cerr << "recv from host: '" << str << "'!" << std::endl;
				}
			}
		}
	}

	virtual ProfileRef getProfile() {
		return profile;
	}

	virtual HostExecutableRef getExecutable() {
		return executable;
	}

	virtual void addLogDelegate(LogDelegateRef delegate) {
		logs.insert(delegate);
	}

	virtual void removeLogDelegate(LogDelegateRef delegate) {
		for(std::set<LogDelegateWRef>::iterator it = logs.begin(); it != logs.end(); it++) {
			LogDelegateRef ref = it->lock();
			if(!ref || ref.get() == delegate.get()) {
				it = logs.erase(it);
			}
		}
	}

	virtual void addHostDelegate(HostDelegateRef delegate) {
		hosts.insert(delegate);
	}

	virtual void removeHostDelegate(HostDelegateRef delegate) {
		for(std::set<HostDelegateWRef>::iterator it = hosts.begin(); it != hosts.end(); it++) {
			HostDelegateRef ref = it->lock();
			if(!ref || ref.get() == delegate.get()) {
				it = hosts.erase(it);
			}
		}
	}

	virtual void log(LogType type, const std::string& message) {
		InstanceRef instance(self);
		std::set<LogDelegateRef> copy;

		for(std::set<LogDelegateWRef>::iterator it = logs.begin(); it != logs.end(); it++) {
			LogDelegateRef ref = it->lock();
			if(ref) {
				copy.insert(ref);
			} else {
				it = logs.erase(it);
			}
		}

		for(std::set<LogDelegateRef>::iterator it = copy.begin(); it != copy.end(); it++) {
			LogDelegateRef ref = *it;
			ref->log(instance, type, message);
		}
	}

	virtual int32_t getWindowCount() {
		return windows.size();
	}

	virtual WindowList getWindowList() {
		WindowList ret;
		for(std::set<WindowWRef>::iterator it = windows.begin(); it != windows.end(); it++) {
			WindowRef ref = it->lock();
			if(ref) {
				ret.push_back(ref);
			} else {
				it = windows.erase(it);
			}
		}
		return ret;
	}

	virtual WindowRef createWindow(bool incognito) {
		std::cerr << "create Window start" << std::endl;

		while(freeWindowChannels.empty()) {
			update();
			Util::sleep(30);
		}

		std::cerr << "got free window channel" << std::endl;
		Ipc::ChannelRef channel = freeWindowChannels.back();
		freeWindowChannels.pop_back();

		WindowRef ret(newWindow(InstanceRef(self), channel, incognito));
		windows.insert(ret);

		std::cerr << "create Window done" << std::endl;
		return ret;
	}
};

InstanceRef newInstance(HostExecutableRef executable, ProfileRef profile, Ipc::ChannelRef ipc, ProcessRef process) {
	InstanceImpl* impl = new InstanceImpl(executable, profile, ipc, process);
	InstanceRef ret(impl);
	impl->setSelf(ret);
	//impl->createWindow(false);
	return ret;
}

} // namespace impl

} // namespace Berkelium