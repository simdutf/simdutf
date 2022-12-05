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

#ifndef ZTD_IDK_TEXT_ENCODING_ID_HPP
#define ZTD_IDK_TEXT_ENCODING_ID_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/endian.hpp>

#include <cstddef>

#include <ztd/prologue.hpp>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	enum class text_encoding_id {
		unknown = 0,
		ascii,
		utf7imap,
		utf7,
		utfebcdic,
		utf8,
		mutf8,
		wtf8,
		cesu8,
		utf16,
		utf16le,
		utf16be,
		utf32,
		utf32le,
		utf32be,
		gb18030,
		utf1,
	};

	inline constexpr bool is_unicode_encoding_id(text_encoding_id __id) noexcept {
		switch (__id) {
		case text_encoding_id::utf7:
		case text_encoding_id::utf7imap:
		case text_encoding_id::utfebcdic:
		case text_encoding_id::utf8:
		case text_encoding_id::utf16:
		case text_encoding_id::utf16le:
		case text_encoding_id::utf16be:
		case text_encoding_id::utf32:
		case text_encoding_id::utf32le:
		case text_encoding_id::utf32be:
		case text_encoding_id::gb18030:
		case text_encoding_id::wtf8:
		case text_encoding_id::mutf8:
		case text_encoding_id::utf1:
		case text_encoding_id::cesu8:
			return true;
		case text_encoding_id::ascii:
		case text_encoding_id::unknown:
		default:
			return false;
		}
	}

	inline constexpr text_encoding_id to_byte_text_encoding_id(
	     text_encoding_id __id, endian __endianness, ::std::size_t __character_size) noexcept {
		if (__character_size == sizeof(unsigned char)) {
			switch (__id) {
			case text_encoding_id::utf7:
			case text_encoding_id::utf7imap:
			case text_encoding_id::utfebcdic:
			case text_encoding_id::utf8:
			case text_encoding_id::ascii:
			case text_encoding_id::utf1:
			case text_encoding_id::cesu8:
			case text_encoding_id::mutf8:
			case text_encoding_id::wtf8:
			case text_encoding_id::gb18030:
				return __id;
			default:
				break;
			}
		}
		switch (__id) {
		case text_encoding_id::utf16:
			return (__endianness == endian::big
			          ? text_encoding_id::utf16be
			          : (__endianness == endian::little ? text_encoding_id::utf16le : text_encoding_id::unknown));
		case text_encoding_id::utf32:
			return (__endianness == endian::big
			          ? text_encoding_id::utf32be
			          : (__endianness == endian::little ? text_encoding_id::utf32le : text_encoding_id::unknown));
		default:
			return text_encoding_id::unknown;
		}
	}

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#include <ztd/epilogue.hpp>

#endif
