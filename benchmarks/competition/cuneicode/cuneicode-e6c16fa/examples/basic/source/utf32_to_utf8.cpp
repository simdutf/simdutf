#include <ztd/cuneicode.h>

#include <ztd/idk/size.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

int main() {
	const char32_t input_data[] = U"Bark Bark Bark 🐕‍🦺!";
	char output_data[ztd_c_array_size(input_data) * 4] = {};
	cnc_mcstate_t state                                = {};
	// set the "do UB shit if invalid" bit to true
	cnc_mcstate_set_assume_valid(&state, true);
	const size_t starting_input_size  = ztd_c_string_array_size(input_data);
	size_t input_size                 = starting_input_size;
	const char32_t* input             = input_data;
	const size_t starting_output_size = ztd_c_array_size(output_data);
	size_t output_size                = starting_output_size;
	char* output                      = output_data;
	cnc_mcerror err                   = cnc_c32snrtomcsn_utf8(
          &output_size, &output, &input_size, &input, &state);
	const bool has_err          = err != CNC_MCERROR_OK;
	const size_t input_read     = starting_input_size - input_size;
	const size_t output_written = starting_output_size - output_size;
	const char* const conversion_result_title_str
	     = (const char*)(has_err ? u8"Conversion failed... 😭"
	                             : u8"Conversion succeeded 🎉");
	const size_t conversion_result_title_str_size
	     = strlen(conversion_result_title_str);
	// Use fwrite to prevent conversions / locale-sensitive-probing from
	// fprintf family of functions
	fwrite(conversion_result_title_str, sizeof(*conversion_result_title_str),
	     conversion_result_title_str_size, has_err ? stderr : stdout);
	fprintf(has_err ? stderr : stdout,
	     "\n\tRead: %zu %zu-bit elements"
	     "\n\tWrote: %zu %zu-bit elements\n",
	     (size_t)(input_read), (size_t)(sizeof(*input) * CHAR_BIT),
	     (size_t)(output_written), (size_t)(sizeof(*output) * CHAR_BIT));
	fprintf(stdout, "%s Conversion Result:\n", has_err ? "Partial" : "Complete");
	fwrite(output_data, sizeof(*output_data), output_written, stdout);
	// the stream (may be) line-buffered, so make sure an extra "\n" is written
	// out this is actually critical for some forms of stdout/stderr mirrors; they
	// won't show the last line even if you manually call fflush(…) !
	fwrite("\n", sizeof(char), 1, stdout);
	return has_err ? 1 : 0;
}
