// Copyright (c) 2013 The Berkelium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERKELIUM_API_INSTANCE_HPP_
#define BERKELIUM_API_INSTANCE_HPP_
#pragma once

// =========================================
// WARNING:
//   THIS FILE IS AUTOMATICALLY GENERATED!
//   !! ANY CHANGES WILL BE OVERWRITTEN !!
//
// See berkelium/berkelium-api/update.sh
// =========================================

#include <Berkelium/API/Berkelium.hpp>

namespace Berkelium {

// Represents a running berkelium host instance.
class Instance {
protected:
	Instance();

public:
	virtual ~Instance() = 0;

	// Returns the associated Runtime.
	virtual RuntimeRef getRuntime() = 0;

	// Closes all open Windows/Tabs and the profile, terminates the host.
	virtual void close() = 0;

	// Returns the profile used to launch this instance.
	virtual ProfileRef getProfile() = 0;

	// Returns the executable used to launch this instance.
	virtual HostExecutableRef getExecutable() = 0;

	// Add the given HostDelegate to this Instance.
	virtual void addHostDelegate(HostDelegateRef delegate) = 0;

	// Remove the given HostDelegate from this Instance.
	virtual void removeHostDelegate(HostDelegateRef delegate) = 0;

	// Returns the number of active Windows.
	virtual int32_t getWindowCount() = 0;

	// Returns a list of all active windows.
	virtual WindowListRef getWindowList() = 0;

	// Open a new window.
	virtual WindowRef createWindow(bool incognito) = 0;
};

} // namespace Berkelium

#endif // BERKELIUM_INSTANCE_HPP_
