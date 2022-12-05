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

#ifndef ZTD_CUNEICODE_CONV_H
#define ZTD_CUNEICODE_CONV_H

#pragma once

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/mcerror.h>
#include <ztd/cuneicode/open_error.h>
#include <ztd/cuneicode/registry.h>
#include <ztd/cuneicode/heap.h>
#include <ztd/cuneicode/pivot_info.h>
#include <ztd/cuneicode/inout_result.h>

#include <ztd/idk/charN_t.h>
#include <ztd/idk/extent.h>

#if ZTD_IS_ON(ZTD_CXX)
#include <cstddef>
#else
#include <stddef.h>
#include <stdbool.h>
#endif

//////
/// @addtogroup ztd_cuneicode_conversion Conversion Functions
///
/// @{

//////
/// @brief Creates a new encoding using the heap provided to the `__registry`.
///
/// @param[in] __registry The registry to use for opening the cnc_conversion handle.
/// @param[in] __from A pointer to data encoded as UTF-8 representing the encoding to transcode
/// from.
/// @param[in] __to A pointer to data encoded as UTF-8 representing the encoding to transcode
/// to.
/// @param[in, out] __out_p_conversion A pointer to the cnc_conversion handle to open.
/// @param[in, out] __p_info A pointer to an already-created cnc_conversion_info that will be
/// filled out with information regarding how the transcoding operation was opened, if it was
/// successful.
///
/// @remarks This call defers to calling cnc_conv_new_n after computing the length of the
/// `__from` and
/// `__to` parameters, if they are not `nullptr`. If either is `nullptr`, their size is assumed
/// to be 0.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_new_c8(
     cnc_conversion_registry* __registry, const ztd_char8_t* __from, const ztd_char8_t* __to,
     cnc_conversion** __out_p_conversion, cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Creates a new encoding using the heap provided to the `__registry`.
///
/// @param[in] __registry The registry to use for opening the cnc_conversion handle.
/// @param[in] __from_size The size of the `__from` string.
/// @param[in] __from A pointer to data encoded as UTF-8 representing the encoding to transcode
/// from.
/// @param[in] __to_size The size of the `__to` string.
/// @param[in] __to A pointer to data encoded as UTF-8 representing the encoding to transcode to.
/// @param[in, out] __out_p_conversion A pointer to the cnc_conversion handle to open.
/// @param[in, out] __p_info A pointer to an already-created cnc_conversion_info that will be filled
/// out with information regarding how the transcoding operation was opened, if it was successful.
///
/// @remarks This call defers to calling cnc_conv_open_n after computing the necessary size from
/// cnc_conv_open_n.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_new_c8n(
     cnc_conversion_registry* __registry, size_t __from_size,
     const ztd_char8_t __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const ztd_char8_t __to[ZTD_PTR_EXTENT(__to_size)], cnc_conversion** __out_p_conversion,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;


//////
/// @brief Creates a new encoding using the heap provided to the `__registry`.
///
/// @param[in] __registry The registry to use for opening the cnc_conversion handle.
/// @param[in] __from_size The size of the `__from` string.
/// @param[in] __from A pointer to data encoded as UTF-8 representing the encoding to transcode
/// from.
/// @param[in] __to_size The size of the `__to` string.
/// @param[in] __to A pointer to data encoded as UTF-8 representing the encoding to transcode to.
/// @param[in] __selection A function pointer to a indirect selection function.
/// @param[in, out] __out_p_conversion A pointer to the cnc_conversion handle to open.
/// @param[in, out] __p_info A pointer to an already-created cnc_conversion_info that will be filled
/// out with information regarding how the transcoding operation was opened, if it was successful.
///
/// @remarks This call defers to calling cnc_conv_open_n after computing the necessary size from
/// cnc_conv_open_n.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_new_c8_select(
     cnc_conversion_registry* __registry, const ztd_char8_t* __from, const ztd_char8_t* __to,
     cnc_indirect_selection_c8_function* __selection, cnc_conversion** __out_p_conversion,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;


//////
/// @brief Creates a new encoding using the heap provided to the `__registry`.
///
/// @param[in] __registry The registry to use for opening the cnc_conversion handle.
/// @param[in] __from_size The size of the `__from` string.
/// @param[in] __from A pointer to data encoded as UTF-8 representing the encoding to transcode
/// from.
/// @param[in] __to_size The size of the `__to` string.
/// @param[in] __to A pointer to data encoded as UTF-8 representing the encoding to transcode to.
/// @param[in] __selection A function pointer to a indirect selection function.
/// @param[in, out] __out_p_conversion A pointer to the cnc_conversion handle to open.
/// @param[in, out] __p_info A pointer to an already-created cnc_conversion_info that will be filled
/// out with information regarding how the transcoding operation was opened, if it was successful.
///
/// @remarks This call defers to calling cnc_conv_open_n after computing the necessary size from
/// cnc_conv_open_n.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_new_c8n_select(
     cnc_conversion_registry* __registry, size_t __from_size,
     const ztd_char8_t __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const ztd_char8_t __to[ZTD_PTR_EXTENT(__to_size)],
     cnc_indirect_selection_c8_function* __selection, cnc_conversion** __out_p_conversion,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Opens a new encoding in the provided space.
///
/// @param[in] __registry The registry to use for opening the cnc_conversion handle.
/// @param[in] __from A pointer to data encoded as UTF-8 representing the encoding to transcode
/// from.
/// @param[in] __to A pointer to data encoded as UTF-8 representing the encoding to transcode to.
/// @param[in, out] __out_p_conversion A pointer to the cnc_conversion handle to open.
/// @param[in, out] __p_available_space The amount of space available in the `__space` parameter, in
/// the number of bytes, that can be used to allocate any necessary data for the cnc_conversion
/// handle.
/// @param[in, out] __space A pointer to space which can be used for opening up the conversion
/// handle, which has the size indicated by `__p_available_space`.
/// @param[in, out] __p_info A pointer to an already-created cnc_conversion_info that will be filled
/// out with information regarding how the transcoding operation was opened, if it was successful.
///
/// @remarks This call defers to calling cnc_conv_open_n after computing the length of the `__from`
/// and
/// `__to` parameters, if they are not `nullptr`. If either is `nullptr`, their size is assumed to
/// be 0.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open_c8(
     cnc_conversion_registry* __registry, const ztd_char8_t* __from, const ztd_char8_t* __to,
     cnc_conversion** __out_p_conversion, size_t* __p_available_space, unsigned char* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Opens a new encoding in the provided space an returns it through `__out_p_conversion`.
///
/// @param[in] __registry The registry to use for opening the cnc_conversion handle.
/// @param[in] __from_size The size of the `__from` string.
/// @param[in] __from A pointer to data encoded as UTF-8 representing the encoding to transcode
/// from.
/// @param[in] __to_size The size of the `__to` string.
/// @param[in] __to A pointer to data encoded as UTF-8 representing the encoding to transcode to.
/// @param[in, out] __out_p_conversion A pointer to the cnc_conversion handle to open.
/// @param[in, out] __p_available_space The amount of space available in the `__space` parameter, in
/// the number of bytes, that can be used to allocate any necessary data for the cnc_conversion
/// handle.
/// @param[in, out] __space A pointer to space which can be used for opening up the conversion
/// handle, which has the size indicated by `__p_available_space`.
/// @param[in, out] __p_info A pointer to an already-created cnc_conversion_info that will be filled
/// out with information regarding how the transcoding operation was opened, if it was successful.
///
/// @remarks If there is a conversion that can be achieved by using an intermediate encoding (e.g.,
/// converting from the desired `__from` encoding to UTF-32, then from UTF-32 to the desired `__to`
/// encoding), then an indirect transcode will be opened if a direct encoding cannot be opened. If
/// there is not enough space to write out, then this function will provide the amount needed in
/// `__p_available_space` directly. Otherwise, it will decrement the value pointed to be
/// `__p_available_space` by the amount of space used.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open_c8n(
     cnc_conversion_registry* __registry, size_t __from_size,
     const ztd_char8_t __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const ztd_char8_t __to[ZTD_PTR_EXTENT(__to_size)], cnc_conversion** __out_p_conversion,
     size_t* __p_available_space, void* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;


//////
/// @brief Opens a new encoding in the provided space an returns it through `__out_p_conversion`.
///
/// @param[in] __registry The registry to use for opening the cnc_conversion handle.
/// @param[in] __from A pointer to data encoded as UTF-8 representing the encoding to transcode
/// from.
/// @param[in] __to A pointer to data encoded as UTF-8 representing the encoding to transcode to.
/// @param[in] __selection A function pointer to an indirect selection function.
/// @param[in, out] __out_p_conversion A pointer to the cnc_conversion handle to open.
/// @param[in, out] __p_available_space The amount of space available in the `__space` parameter, in
/// the number of bytes, that can be used to allocate any necessary data for the cnc_conversion
/// handle.
/// @param[in, out] __space A pointer to space which can be used for opening up the conversion
/// handle, which has the size indicated by `__p_available_space`.
/// @param[in, out] __p_info A pointer to an already-created cnc_conversion_info that will be filled
/// out with information regarding how the transcoding operation was opened, if it was successful.
///
/// @remarks If there is a conversion that can be achieved by using an intermediate encoding (e.g.,
/// converting from the desired `__from` encoding to UTF-32, then from UTF-32 to the desired `__to`
/// encoding), then an indirect transcode will be opened if a direct encoding cannot be opened. If
/// there is not enough space to write out, then this function will provide the amount needed in
/// `__p_available_space` directly. Otherwise, it will decrement the value pointed to be
/// `__p_available_space` by the amount of space used.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open_c8_select(
     cnc_conversion_registry* __registry, const ztd_char8_t* __from, const ztd_char8_t* __to,
     cnc_indirect_selection_c8_function* __selection, cnc_conversion** __out_p_conversion,
     size_t* __p_available_space, void* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;


//////
/// @brief Opens a new encoding in the provided space an returns it through `__out_p_conversion`.
///
/// @param[in] __registry The registry to use for opening the cnc_conversion handle.
/// @param[in] __from_size The size of the `__from` string.
/// @param[in] __from A pointer to data encoded as UTF-8 representing the encoding to transcode
/// from.
/// @param[in] __to_size The size of the `__to` string.
/// @param[in] __to A pointer to data encoded as UTF-8 representing the encoding to transcode to.
/// @param[in] __selection A function pointer to an indirect selection function.
/// @param[in, out] __out_p_conversion A pointer to the cnc_conversion handle to open.
/// @param[in, out] __p_available_space The amount of space available in the `__space` parameter, in
/// the number of bytes, that can be used to allocate any necessary data for the cnc_conversion
/// handle.
/// @param[in, out] __space A pointer to space which can be used for opening up the conversion
/// handle, which has the size indicated by `__p_available_space`.
/// @param[in, out] __p_info A pointer to an already-created cnc_conversion_info that will be filled
/// out with information regarding how the transcoding operation was opened, if it was successful.
///
/// @remarks If there is a conversion that can be achieved by using an intermediate encoding (e.g.,
/// converting from the desired `__from` encoding to UTF-32, then from UTF-32 to the desired `__to`
/// encoding), then an indirect transcode will be opened if a direct encoding cannot be opened. If
/// there is not enough space to write out, then this function will provide the amount needed in
/// `__p_available_space` directly. Otherwise, it will decrement the value pointed to be
/// `__p_available_space` by the amount of space used.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open_c8n_select(
     cnc_conversion_registry* __registry, size_t __from_size,
     const ztd_char8_t __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const ztd_char8_t __to[ZTD_PTR_EXTENT(__to_size)],
     cnc_indirect_selection_c8_function* __selection, cnc_conversion** __out_p_conversion,
     size_t* __p_available_space, void* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_conv_new_c8
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_new(
     cnc_conversion_registry* __registry, const char* __from, const char* __to,
     cnc_conversion** __out_p_conversion, cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_conv_new_c8n
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_new_n(
     cnc_conversion_registry* __registry, size_t __from_size,
     const char __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const char __to[ZTD_PTR_EXTENT(__to_size)], cnc_conversion** __out_p_conversion,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;


//////
/// @copydoc cnc_conv_new_c8_select
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_new_select(
     cnc_conversion_registry* __registry, const char* __from, const char* __to,
     cnc_indirect_selection_function* __selection, cnc_conversion** __out_p_conversion,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_conv_new_c8n_select
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_new_n_select(
     cnc_conversion_registry* __registry, size_t __from_size,
     const char __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const char __to[ZTD_PTR_EXTENT(__to_size)], cnc_indirect_selection_function* __selection,
     cnc_conversion** __out_p_conversion, cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_conv_open_c8
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open(
     cnc_conversion_registry* __registry, const char* __from, const char* __to,
     cnc_conversion** __out_p_conversion, size_t* __p_available_space, unsigned char* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_conv_open_c8n
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open_n(
     cnc_conversion_registry* __registry, size_t __from_size,
     const char __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const char __to[ZTD_PTR_EXTENT(__to_size)], cnc_conversion** __out_p_conversion,
     size_t* __p_available_space, void* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;


//////
/// @copydoc cnc_conv_open_c8_select
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open_select(
     cnc_conversion_registry* __registry, const char* __from, const char* __to,
     cnc_indirect_selection_function* __selection, cnc_conversion** __out_p_conversion,
     size_t* __p_available_space, void* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;


//////
/// @copydoc cnc_conv_open_c8n_select
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open_n_select(
     cnc_conversion_registry* __registry, size_t __from_size,
     const char __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const char __to[ZTD_PTR_EXTENT(__to_size)], cnc_indirect_selection_function* __selection,
     cnc_conversion** __out_p_conversion, size_t* __p_available_space, void* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Closes (destroys) the data used by the cnc_conversion handle pointed to by
/// `__conversion`.
///
/// @param[in] __conversion The cnc_conversion handle to destroy. Can be `nullptr`.
///
/// @remarks This function, to use a C++ analogy, behaves much like a destructor. It does not free
/// any memory: it simply destroys anything created or used by the cnc_open_function supplied when
/// registrying the "from" and "to" conversion pair. If `__conversion` is `nullptr`, this function
/// does nothing.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ void cnc_conv_close(
     cnc_conversion* __conversion) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Deletes and data used by the cnc_conversion handle pointed to by `__conversion`.
///
/// @param[in] __conversion The cnc_conversion handle to destroy. Can be `nullptr`.
///
/// @remarks This function will call `cnc_conv_close` on the `__conversion` function, and then
/// delete the memory. It must not be used if cnc_conv_new or cnc_conv_new_n was not used. If
/// `__conversion` is `nullptr`, this function does nothing.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ void cnc_conv_delete(
     cnc_conversion* __conversion) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Converts a series of bytes in one encoding scheme to the other encoding scheme using the
/// specified `__conversion` format.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __out_pput_bytes_size A pointer to the size of the output buffer. If this is
/// `nullptr`, then it will not update the count (and the output stream will automatically be
/// considered large enough to handle all data, if
/// `__out_pput_bytes` is not `nullptr`).
/// @param[in, out] __out_pput_bytes A pointer to the pointer of the output buffer. If this or the
/// pointer within are `nullptr`, than this function will not write output data (it may still
/// decrement the value pointed to by
/// `__out_pput_bytes_size`).
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and
/// CNC_MCERROR_OK is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and CNC_MCERROR_OK is
/// returned.
///
/// @remarks The conversion functions take parameters as output parameters (pointers) so that they
/// can provide information about how much of the input and output is used. Providing a `nullptr`
/// for both `__p_ouput_bytes_size` and `__out_pput_bytes` serves as a way to validate the input.
/// Providing only `__out_pput_bytes` but not
/// `__out_pput_bytes_size` is a way to indicate that the output space is sufficiently large for the
/// input space. Providing `__out_pput_bytes_size` but not `__out_pput_bytes` is a way to determine
/// how much data will be written out for a given input without actually performing such a write.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv(
     cnc_conversion* __conversion, size_t* __out_pput_bytes_size, unsigned char** __out_pput_bytes,
     size_t* __p_input_bytes_size, const unsigned char** __p_input_bytes) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Converts a series of bytes in one encoding scheme to the other encoding scheme using the
/// specified `__conversion` format.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __out_pput_bytes_size A pointer to the size of the output buffer. If this is
/// `nullptr`, then it will not update the count (and the output stream will automatically be
/// considered large enough to handle all data, if
/// `__out_pput_bytes` is not `nullptr`).
/// @param[in, out] __out_pput_bytes A pointer to the pointer of the output buffer. If this or the
/// pointer within are `nullptr`, than this function will not write output data (it may still
/// decrement the value pointed to by
/// `__out_pput_bytes_size`).
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and
/// CNC_MCERROR_OK is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and CNC_MCERROR_OK is
/// returned.
/// @param[in, out] __p_pivot_info A pointer to a pivot buffer and return error code. If the return
/// value of this function is not CNC_MCERROR_OK, the pivot information is not NULL, and the error
/// was caused by the intermediate conversion step failing, then the `error` member of
/// cnc_pivot_info will be set to the error value that took place.
///
/// @remarks The conversion functions take parameters as output parameters (pointers) so that they
/// can provide information about how much of the input and output is used. Providing a `nullptr`
/// for both `__p_ouput_bytes_size` and `__out_pput_bytes` serves as a way to validate the input.
/// Providing only `__out_pput_bytes` but not
/// `__out_pput_bytes_size` is a way to indicate that the output space is sufficiently large for the
/// input space. Providing `__out_pput_bytes_size` but not `__out_pput_bytes` is a way to determine
/// how much data will be written out for a given input without actually performing such a write.
/// The pivot buffer is used when the conversion cannot be done directly (which is specified through
/// the cnc_conversion_info structure returned from opening a conversion routine). If the pivot
/// buffer does not point to a null / empty buffer, and it ends up being too small for the given
/// conversion, it may produce spurious CNC_MCERROR_INSUFFICIENT_OUTPUT errors unrelated to the
/// actual `__out_pput_bytes` buffer passed into the function.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_pivot(
     cnc_conversion* __conversion, size_t* __out_pput_bytes_size, unsigned char** __out_pput_bytes,
     size_t* __p_input_bytes_size, const unsigned char** __p_input_bytes,
     cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Counts the total number of bytes that can be successfully converted until an error occurs
/// for the specified `__conversion` format.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __out_pput_bytes_size A pointer to the size of the output buffer. If this is
/// `nullptr`, then it will not update the count.
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and
/// CNC_MCERROR_OK is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and CNC_MCERROR_OK is
/// returned.
///
/// @remarks This function is an ease-of-use shortcut for calling cnc_conv with the
/// `__out_pput_bytes` argument sent to `nullptr`.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_count(
     cnc_conversion* __conversion, size_t* __out_pput_bytes_size, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Counts the total number of bytes that can be successfully converted until an error occurs
/// for the specified `__conversion` format.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __out_pput_bytes_size A pointer to the size of the output buffer. If this is
/// `nullptr`, then it will not update the count.
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and
/// CNC_MCERROR_OK is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and CNC_MCERROR_OK is
/// returned.
/// @param[in, out] __p_pivot_info A pointer to a pivot buffer and return error code. If the return
/// value of this function is not CNC_MCERROR_OK, the pivot information is not NULL, and the error
/// was caused by the intermediate conversion step failing, then the `error` member of
/// cnc_pivot_info will be set to the error value that took place.
///
/// @remarks This function is an ease-of-use shortcut for calling cnc_conv with the
/// `__out_pput_bytes` argument sent to `nullptr`.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_count_pivot(
     cnc_conversion* __conversion, size_t* __out_pput_bytes_size, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes, cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Checks whether or not the input can be successfully converted according to the format of
/// `__conversion`.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and `true`
/// is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and `true` is returned.
///
/// @remarks This function is an ease-of-use shortcut for calling cnc_conv with the
/// `__out_pput_bytes` argument sent to `nullptr`.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_conv_is_valid(
     cnc_conversion* __conversion, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Checks whether or not the input can be successfully converted according to the format of
/// `__conversion`.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and `true`
/// is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and `true` is returned.
/// @param[in, out] __p_pivot_info A pointer to a pivot buffer and return error code. If the return
/// value of this function is not CNC_MCERROR_OK, the pivot information is not NULL, and the error
/// was caused by the intermediate conversion step failing, then the `error` member of
/// cnc_pivot_info will be set to the error value that took place.
///
/// @remarks This function is an ease-of-use shortcut for calling cnc_conv with the
/// `__out_pput_bytes` argument sent to `nullptr`.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_conv_is_valid_pivot(
     cnc_conversion* __conversion, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes, cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Converts a series of bytes in one encoding scheme to the other encoding scheme using the
/// specified
/// `__conversion` format.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __out_pput_bytes A pointer to the pointer of the output buffer. If this or the
/// pointer within are `nullptr`, than this function will not write output data (it may still
/// decrement the value pointed to by
/// `__out_pput_bytes_size`).
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and
/// CNC_MCERROR_OK is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and CNC_MCERROR_OK is
/// returned.
///
/// @remarks The conversion functions take parameters as output parameters (pointers) so that they
/// can provide information about how much of the input and output is used. Providing a `nullptr`
/// for both `__p_ouput_bytes_size` and `__out_pput_bytes` serves as a way to validate the input.
/// Providing only `__out_pput_bytes` but not
/// `__out_pput_bytes_size` is a way to indicate that the output space is sufficiently large for the
/// input space. Providing `__out_pput_bytes_size` but not `__out_pput_bytes` is a way to determine
/// how much data will be written out for a given input without actually performing such a write.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_unbounded(
     cnc_conversion* __conversion, unsigned char** __out_pput_bytes, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Converts a series of bytes in one encoding scheme to the other encoding scheme using the
/// specified
/// `__conversion` format.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __out_pput_bytes A pointer to the pointer of the output buffer. If this or the
/// pointer within are `nullptr`, than this function will not write output data (it may still
/// decrement the value pointed to by
/// `__out_pput_bytes_size`).
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and
/// CNC_MCERROR_OK is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and CNC_MCERROR_OK is
/// returned.
/// @param[in, out] __p_pivot_info A pointer to a pivot buffer and return error code. If the return
/// value of this function is not CNC_MCERROR_OK, the pivot information is not NULL, and the error
/// was caused by the intermediate conversion step failing, then the `error` member of
/// cnc_pivot_info will be set to the error value that took place.
///
/// @remarks The conversion functions take parameters as output parameters (pointers) so that they
/// can provide information about how much of the input and output is used. Providing a `nullptr`
/// for both `__p_ouput_bytes_size` and `__out_pput_bytes` serves as a way to validate the input.
/// Providing only `__out_pput_bytes` but not
/// `__out_pput_bytes_size` is a way to indicate that the output space is sufficiently large for the
/// input space. Providing `__out_pput_bytes_size` but not `__out_pput_bytes` is a way to determine
/// how much data will be written out for a given input without actually performing such a write.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_unbounded_pivot(
     cnc_conversion* __conversion, unsigned char** __out_pput_bytes, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes, cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Peforms at least one complete unit of work on the input and produces one complete unit of
/// work into the output according to the format of `__conversion`.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __out_pput_bytes_size A pointer to the size of the output buffer. If this is
/// `nullptr`, then it will not update the count (and the output stream will automatically be
/// considered large enough to handle all data, if
/// `__out_pput_bytes` is not `nullptr`).
/// @param[in, out] __out_pput_bytes A pointer to the pointer of the output buffer. If this or the
/// pointer within are `nullptr`, than this function will not write output data (it may still
/// decrement the value pointed to by `__out_pput_bytes_size`).
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and
/// CNC_MCERROR_OK is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and CNC_MCERROR_OK is
/// returned.
///
/// @remarks This function only performs exactly one complete unit of work for the input and the
/// output: nothing more. The conversion functions take parameters as output parameters (pointers)
/// so that they can provide information about how much of the input and output is used. Providing a
/// `nullptr` for both `__p_ouput_bytes_size` and
/// `__out_pput_bytes` serves as a way to validate the input for one completely unit of work.
/// Providing only
/// `__out_pput_bytes` but not `__out_pput_bytes_size` is a way to indicate that the output space is
/// sufficiently large for the input space. Providing `__out_pput_bytes_size` but not
/// `__out_pput_bytes` is a way to determine how much data will be written out for a given input
/// without actually performing such a write.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_one(
     cnc_conversion* __conversion, size_t* __out_pput_bytes_size, unsigned char** __out_pput_bytes,
     size_t* __p_input_bytes_size, const unsigned char** __p_input_bytes) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Peforms at least one complete unit of work on the input and produces one complete unit of
/// work into the output according to the format of `__conversion`.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __out_pput_bytes_size A pointer to the size of the output buffer. If this is
/// `nullptr`, then it will not update the count (and the output stream will automatically be
/// considered large enough to handle all data, if
/// `__out_pput_bytes` is not `nullptr`).
/// @param[in, out] __out_pput_bytes A pointer to the pointer of the output buffer. If this or the
/// pointer within are `nullptr`, than this function will not write output data (it may still
/// decrement the value pointed to by `__out_pput_bytes_size`).
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and
/// CNC_MCERROR_OK is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and CNC_MCERROR_OK is
/// returned.
/// @param[in, out] __p_pivot_info A pointer to a pivot buffer and return error code. If the return
/// value of this function is not CNC_MCERROR_OK, the pivot information is not NULL, and the error
/// was caused by the intermediate conversion step failing, then the `error` member of
/// cnc_pivot_info will be set to the error value that took place.
///
/// @remarks This function only performs exactly one complete unit of work for the input and the
/// output: nothing more. The conversion functions take parameters as output parameters (pointers)
/// so that they can provide information about how much of the input and output is used. Providing a
/// `nullptr` for both `__p_ouput_bytes_size` and
/// `__out_pput_bytes` serves as a way to validate the input for one completely unit of work.
/// Providing only
/// `__out_pput_bytes` but not `__out_pput_bytes_size` is a way to indicate that the output space is
/// sufficiently large for the input space. Providing `__out_pput_bytes_size` but not
/// `__out_pput_bytes` is a way to determine how much data will be written out for a given input
/// without actually performing such a write.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_one_pivot(
     cnc_conversion* __conversion, size_t* __out_pput_bytes_size, unsigned char** __out_pput_bytes,
     size_t* __p_input_bytes_size, const unsigned char** __p_input_bytes,
     cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Counts the total number of bytes that can be successfully converted for one complete unit
/// of input, or if an error occurs, for the specified `__conversion` format.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __out_pput_bytes_size A pointer to the size of the output buffer. If this is
/// `nullptr`, then it will not update the count.
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and
/// CNC_MCERROR_OK is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and CNC_MCERROR_OK is
/// returned.
///
/// @remarks This function is an ease-of-use shortcut for calling cnc_conv_one with the
/// `__out_pput_bytes` argument sent to `nullptr`.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_one_count(
     cnc_conversion* __conversion, size_t* __out_pput_bytes_size, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Counts the total number of bytes that can be successfully converted for one complete unit
/// of input, or if an error occurs, for the specified `__conversion` format.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __out_pput_bytes_size A pointer to the size of the output buffer. If this is
/// `nullptr`, then it will not update the count.
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and
/// CNC_MCERROR_OK is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and CNC_MCERROR_OK is
/// returned.
/// @param[in, out] __p_pivot_info A pointer to a pivot buffer and return error code. If the return
/// value of this function is not CNC_MCERROR_OK, the pivot information is not NULL, and the error
/// was caused by the intermediate conversion step failing, then the `error` member of
/// cnc_pivot_info will be set to the error value that took place.
///
/// @remarks This function is an ease-of-use shortcut for calling cnc_conv_one with the
/// `__out_pput_bytes` argument sent to `nullptr`.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_one_count_pivot(
     cnc_conversion* __conversion, size_t* __out_pput_bytes_size, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes, cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Checks whether or not one complete unit of input can be successfully converted according
/// to the format of
/// `__conversion` to one complete unit of output.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and `true`
/// is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and `true` is returned.
///
/// @remarks This function is an ease-of-use shortcut for calling cnc_conv_one with the
/// `__out_pput_bytes` argument and the `__out_pput_byte_size` argument set to `nullptr` sent to
/// `nullptr`.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_conv_one_is_valid(
     cnc_conversion* __conversion, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Checks whether or not one complete unit of input can be successfully converted according
/// to the format of
/// `__conversion` to one complete unit of output.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and `true`
/// is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and `true` is returned.
/// @param[in, out] __p_pivot_info A pointer to a pivot buffer and return error code. If the return
/// value of this function is not CNC_MCERROR_OK, the pivot information is not NULL, and the error
/// was caused by the intermediate conversion step failing, then the `error` member of
/// cnc_pivot_info will be set to the error value that took place.
///
/// @remarks This function is an ease-of-use shortcut for calling cnc_conv_one with the
/// `__out_pput_bytes` argument and the `__out_pput_byte_size` argument set to `nullptr` sent to
/// `nullptr`.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_conv_one_is_valid_pivot(
     cnc_conversion* __conversion, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes, cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Performs a conversion to the specified sequences, assuming that there is an appropriately
/// sized buffer that will fit all of the output, no matter what.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __out_pput_bytes A pointer to the pointer of the output buffer. If this or the
/// pointer within are `nullptr`, than this function will not write output data (it may still
/// decrement the value pointed to by `__out_pput_bytes_size`).
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and `true`
/// is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and `true` is returned.
///
/// @remarks This function is an ease-of-use shortcut for calling cnc_conv_one with the
/// `__out_pput_bytes` argument and the `__out_pput_byte_size` argument set to `nullptr` sent to
/// `nullptr`.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_one_unbounded(
     cnc_conversion* __conversion, unsigned char** __out_pput_bytes, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Performs a conversion to the specified sequences, assuming that there is an appropriately
/// sized buffer that will fit all of the output, no matter what.
///
/// @param[in] __conversion The cnc_conversion handle indicating the format to be used. Shall not be
/// `nullptr`.
/// @param[in, out] __out_pput_bytes A pointer to the pointer of the output buffer. If this or the
/// pointer within are `nullptr`, than this function will not write output data (it may still
/// decrement the value pointed to by `__out_pput_bytes_size`).
/// @param[in, out] __p_input_bytes_size A pointer to the size of the input buffer. If this is
/// `nullptr` or points to a value equivalent to `0`, then the input is considered empty and `true`
/// is returned.
/// @param[in, out] __p_input_bytes A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and `true` is returned.
/// @param[in, out] __p_pivot_info A pointer to a pivot buffer and return error code. If the return
/// value of this function is not CNC_MCERROR_OK, the pivot information is not NULL, and the error
/// was caused by the intermediate conversion step failing, then the `error` member of
/// cnc_pivot_info will be set to the error value that took place.
///
/// @remarks This function is an ease-of-use shortcut for calling cnc_conv_one with the
/// `__out_pput_bytes` argument and the `__out_pput_byte_size` argument set to `nullptr` sent to
/// `nullptr`.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_conv_one_unbounded_pivot(
     cnc_conversion* __conversion, unsigned char** __out_pput_bytes, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes, cnc_pivot_info* __p_pivot_info) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @}

#endif
