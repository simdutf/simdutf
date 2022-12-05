// =============================================================================
//
// ztd.idk
// Copyright © 2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.idk licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Shepherd's Oasis, LLC.
// For licensing terms and conditions see your agreement. For
// further information contact opensource@soasis.org.
//
// Apache License Version 2 Usage
// Alternatively, this file may be used under the terms of Apache License
// Version 2.0 (the "License") for non-commercial use; you may not use this
// file except in compliance with the License. You may obtain a copy of the
// License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ============================================================================ //

#pragma once

#ifndef ZTD_IDK_TESTS_KEEP_PROCESS_AWAKE_HPP
#define ZTD_IDK_TESTS_KEEP_PROCESS_AWAKE_HPP

#include <ztd/version.hpp>

#include <ztd/idk/detail/windows.hpp>

#include <memory>
#include <cstddef>
#include <cstring>

namespace ztd { namespace tests {

	class keep_process_awake {
	public:
		keep_process_awake() : _M_success(true) {
#if ZTD_IS_ON(ZTD_PLATFORM_WINDOWS)
			this->_M_power_request_handle             = INVALID_HANDLE_VALUE;
			this->_M_reason_string                    = L"ztd.idk Tests/Benchmarks keep-system-awake";
			this->_M_reason                           = {};
			this->_M_reason.Version                   = POWER_REQUEST_CONTEXT_VERSION;
			this->_M_reason.Flags                     = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
			this->_M_reason.Reason.SimpleReasonString = this->_M_reason_string.data();
			this->_M_power_request_handle             = ::PowerCreateRequest(::std::addressof(this->_M_reason));
			this->_M_success                          = this->_M_power_request_handle != INVALID_HANDLE_VALUE;
			if (this->_M_success) {
				BOOL __success   = ::PowerSetRequest(this->_M_power_request_handle, PowerRequestExecutionRequired);
				this->_M_success = __success != static_cast<BOOL>(0);
			}
#endif
		}

		bool awake_request_successful() {
			return _M_success;
		}

		~keep_process_awake() {
#if ZTD_IS_ON(ZTD_PLATFORM_WINDOWS)
			if (this->_M_success) {
				::PowerClearRequest(this->_M_power_request_handle, PowerRequestExecutionRequired);
			}
			if (this->_M_power_request_handle != INVALID_HANDLE_VALUE) {
				::CloseHandle(this->_M_power_request_handle);
			}
#endif
		}

	private:
		bool _M_success;
#if ZTD_IS_ON(ZTD_PLATFORM_WINDOWS)
		std::wstring _M_reason_string;
		REASON_CONTEXT _M_reason;
		HANDLE _M_power_request_handle;
#endif
	};

}} // namespace ztd::tests

#endif
