// ============================================================================
//
// ztd.cuneicode
// Copyright © 2022-2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.cuneicode licenses may use this file
// in accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Shepherd's Oasis, LLC.
// For licensing terms and conditions see your agreement. For
// further information contact opensource@soasis.org.
//
// Apache License Version 2 Usage
// Alternatively, this file may be used under the terms of Apache License
// Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// 		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ========================================================================= //

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/conv.h>
#include <ztd/cuneicode/mcchar.h>

#include <ztd/cuneicode/detail/conversion.hpp>
#include <ztd/cuneicode/detail/buffer_size.h>

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_pivot(
     cnc_conversion* __conversion, size_t* __p_bytes_out_count, unsigned char** __p_bytes_out,
     size_t* __p_bytes_in_count, const unsigned char** __p_bytes_in,
     cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	void* __state = static_cast<void*>(__conversion + 1);
	return __conversion->__multi_conversion_function(__conversion, __p_bytes_out_count,
	     __p_bytes_out, __p_bytes_in_count, __p_bytes_in, __p_pivot_info, __state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_one_pivot(
     cnc_conversion* __conversion, size_t* __p_bytes_out_count, unsigned char** __p_bytes_out,
     size_t* __p_bytes_in_count, const unsigned char** __p_bytes_in,
     cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	void* __state = static_cast<void*>(__conversion + 1);
	return __conversion->__single_conversion_function(__conversion, __p_bytes_out_count,
	     __p_bytes_out, __p_bytes_in_count, __p_bytes_in, __p_pivot_info, __state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv(
     cnc_conversion* __conversion, size_t* __p_bytes_out_count, unsigned char** __p_bytes_out,
     size_t* __p_bytes_in_count, const unsigned char** __p_bytes_in) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_pivot(__conversion, __p_bytes_out_count, __p_bytes_out, __p_bytes_in_count,
	     __p_bytes_in, nullptr);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_conv_is_valid(
     cnc_conversion* __conversion, size_t* __p_bytes_in_count,
     const unsigned char** __p_bytes_in) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_pivot(
	            __conversion, nullptr, nullptr, __p_bytes_in_count, __p_bytes_in, nullptr)
	     == CNC_MCERROR_OK;
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_conv_is_valid_pivot(
     cnc_conversion* __conversion, size_t* __p_bytes_in_count, const unsigned char** __p_bytes_in,
     cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_pivot(
	            __conversion, nullptr, nullptr, __p_bytes_in_count, __p_bytes_in, __p_pivot_info)
	     == CNC_MCERROR_OK;
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_count(
     cnc_conversion* __conversion, size_t* __p_bytes_out_count, size_t* __p_bytes_in_count,
     const unsigned char** __p_bytes_in) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_pivot(
	     __conversion, __p_bytes_out_count, nullptr, __p_bytes_in_count, __p_bytes_in, nullptr);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_count_pivot(
     cnc_conversion* __conversion, size_t* __p_bytes_out_count, size_t* __p_bytes_in_count,
     const unsigned char** __p_bytes_in, cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_pivot(__conversion, __p_bytes_out_count, nullptr, __p_bytes_in_count,
	     __p_bytes_in, __p_pivot_info);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_unbounded(
     cnc_conversion* __conversion, unsigned char** __p_bytes_out, size_t* __p_bytes_in_count,
     const unsigned char** __p_bytes_in) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_pivot(
	     __conversion, nullptr, __p_bytes_out, __p_bytes_in_count, __p_bytes_in, nullptr);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_unbounded_pivot(
     cnc_conversion* __conversion, unsigned char** __p_bytes_out, size_t* __p_bytes_in_count,
     const unsigned char** __p_bytes_in, cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_pivot(
	     __conversion, nullptr, __p_bytes_out, __p_bytes_in_count, __p_bytes_in, __p_pivot_info);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_one(
     cnc_conversion* __conversion, size_t* __p_bytes_out_count, unsigned char** __p_bytes_out,
     size_t* __p_bytes_in_count, const unsigned char** __p_bytes_in) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_one_pivot(__conversion, __p_bytes_out_count, __p_bytes_out, __p_bytes_in_count,
	     __p_bytes_in, nullptr);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_conv_one_is_valid(
     cnc_conversion* __conversion, size_t* __p_bytes_in_count,
     const unsigned char** __p_bytes_in) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_one_pivot(
	            __conversion, nullptr, nullptr, __p_bytes_in_count, __p_bytes_in, nullptr)
	     == CNC_MCERROR_OK;
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_conv_one_is_valid_pivot(
     cnc_conversion* __conversion, size_t* __p_bytes_in_count, const unsigned char** __p_bytes_in,
     cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_one_pivot(
	            __conversion, nullptr, nullptr, __p_bytes_in_count, __p_bytes_in, __p_pivot_info)
	     == CNC_MCERROR_OK;
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_one_count(
     cnc_conversion* __conversion, size_t* __p_bytes_out_count, size_t* __p_bytes_in_count,
     const unsigned char** __p_bytes_in) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_one_pivot(
	     __conversion, __p_bytes_out_count, nullptr, __p_bytes_in_count, __p_bytes_in, nullptr);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_one_count_pivot(
     cnc_conversion* __conversion, size_t* __p_bytes_out_count, size_t* __p_bytes_in_count,
     const unsigned char** __p_bytes_in, cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_one_pivot(__conversion, __p_bytes_out_count, nullptr, __p_bytes_in_count,
	     __p_bytes_in, __p_pivot_info);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_one_unbounded(
     cnc_conversion* __conversion, unsigned char** __p_bytes_out, size_t* __p_bytes_in_count,
     const unsigned char** __p_bytes_in) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_one_pivot(
	     __conversion, nullptr, __p_bytes_out, __p_bytes_in_count, __p_bytes_in, nullptr);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_one_unbounded_pivot(
     cnc_conversion* __conversion, unsigned char** __p_bytes_out, size_t* __p_bytes_in_count,
     const unsigned char** __p_bytes_in, cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_one_pivot(
	     __conversion, nullptr, __p_bytes_out, __p_bytes_in_count, __p_bytes_in, __p_pivot_info);
}
