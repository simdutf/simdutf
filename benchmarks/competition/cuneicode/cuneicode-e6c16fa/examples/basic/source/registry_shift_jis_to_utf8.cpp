#include <ztd/cuneicode.h>

#include <ztd/idk/size.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

int main() {
	cnc_conversion_registry* registry = NULL;
	{
		cnc_open_error err
		     = cnc_registry_new(&registry, CNC_REGISTRY_OPTIONS_DEFAULT);
		if (err != CNC_OPEN_ERROR_OK) {
			fprintf(stderr, "[error] could not open a new registry.");
			return 2;
		}
	}

	cnc_conversion* conversion          = NULL;
	cnc_conversion_info conversion_info = {};
	{
		cnc_open_error err = cnc_conv_new(
		     registry, "shift-jis", "utf-8", &conversion, &conversion_info);
		if (err != CNC_OPEN_ERROR_OK) {
			fprintf(stderr, "[error] could not open a new registry.");
			cnc_registry_delete(registry);
			return 2;
		}
	}

	fprintf(stdout, "Opened a conversion from \"");
	fwrite(conversion_info.from_code_data,
	     sizeof(*conversion_info.from_code_data), conversion_info.from_code_size,
	     stdout);
	fprintf(stdout, "\" to \"");
	fwrite(conversion_info.to_code_data, sizeof(*conversion_info.to_code_data),
	     conversion_info.to_code_size, stdout);
	if (conversion_info.is_indirect) {
		fprintf(stdout, "\" (through \"");
		fwrite(conversion_info.indirect_code_data,
		     sizeof(*conversion_info.indirect_code_data),
		     conversion_info.indirect_code_size, stdout);
		fprintf(stdout, "\").");
	}
	else {
		fprintf(stdout, "\".");
	}
	fprintf(stdout, "\n");

	const char input_data[]
	     = "\x61\x6c\x6c\x20\x61\x63\x63\x6f\x72\x64\x69\x6e\x67\x20\x74\x6f\x20"
	       "\x82\xAF\x82\xA2\x82\xA9\x82\xAD\x2c\x20\x75\x66\x75\x66\x75\x66\x75"
	       "\x21";
	unsigned char output_data[ztd_c_array_size(input_data) * 2] = {};

	const size_t starting_input_size  = ztd_c_string_array_byte_size(input_data);
	size_t input_size                 = starting_input_size;
	const unsigned char* input        = (const unsigned char*)&input_data[0];
	const size_t starting_output_size = ztd_c_array_byte_size(output_data);
	size_t output_size                = starting_output_size;
	unsigned char* output             = (unsigned char*)&output_data[0];
	cnc_mcerror err
	     = cnc_conv(conversion, &output_size, &output, &input_size, &input);
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
	fprintf(stdout, "\n");

	// clean up resources
	cnc_conv_delete(conversion);
	cnc_registry_delete(registry);
	return has_err ? 1 : 0;
}
