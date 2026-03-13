#!/bin/bash

# Benchmark script to compare fastbase64 and base64 performance
# Measures encoding and decoding times for various file sizes

if [ $# -lt 1 ]; then
    echo "Usage: $0 /path/to/fastbase64"
    exit 1
fi

FASTBASE64="$1"

# Check if fastbase64 exists
if [ ! -x "$FASTBASE64" ]; then
    echo "Error: $FASTBASE64 not found or not executable"
    exit 1
fi

# Detect OS for base64 command syntax
if base64 --help | grep -q GNU; then
    BASE64_ENCODE="base64"
    BASE64_DECODE="base64 -d"
else
    BASE64_ENCODE="base64 -i"
    BASE64_DECODE="base64 -d -i"
fi

echo "Starting fastbase64 vs base64 benchmark..."

# Function to time a command in milliseconds with 0.1ms precision,
# running 10 times and returning the median.
time_cmd() {
    local cmd="$1"
    local times=()
    for i in {1..10}; do
        local start_ns=$(python3 -c "import time; print(time.perf_counter_ns())")
        eval "$cmd" >/dev/null 2>&1
        local end_ns=$(python3 -c "import time; print(time.perf_counter_ns())")
        # Convert ns to tenths of ms (0.1ms), rounded to nearest unit.
        local elapsed_tenths=$(( (end_ns - start_ns + 50000) / 100000 ))
        times+=("$elapsed_tenths")
    done
    # Sort times expressed in tenths of milliseconds.
    local sorted=($(printf '%s\n' "${times[@]}" | sort -n))
    # Median: average of 5th and 6th values (indices 4 and 5).
    local median_tenths=$(( (sorted[4] + sorted[5]) / 2 ))
    printf "%d.%d" $((median_tenths / 10)) $((median_tenths % 10))
}

# Create test files of different sizes
echo "Creating test files..."
dd if=/dev/urandom of=test_1m.bin bs=1024 count=1024 2>/dev/null
dd if=/dev/urandom of=test_10m.bin bs=1024 count=10240 2>/dev/null
dd if=/dev/urandom of=test_100m.bin bs=1024 count=102400 2>/dev/null

sizes=("1m" "10m" "100m")
files=("test_1m.bin" "test_10m.bin" "test_100m.bin")

echo ""
echo "Benchmark Results (times in milliseconds):"
echo "Size     | Encode Base64 | Encode FastBase64 | Decode Base64 | Decode FastBase64"
echo "---------|---------------|-------------------|---------------|------------------"

for i in "${!sizes[@]}"; do
    size="${sizes[$i]}"
    file="${files[$i]}"
    
    # Encode with fastbase64 and measure time
    fastbase64_encode_time=$(time_cmd "$FASTBASE64 -e  \"$file\" > \"fastbase64_$size.b64\"")
    
    # Encode with base64 and measure time
    base64_encode_time=$(time_cmd "$BASE64_ENCODE \"$file\" > \"base64_$size.b64\"")
    
    # Decode base64 encoded file and measure time
    base64_decode_time=$(time_cmd "$BASE64_DECODE \"base64_$size.b64\" > \"recovered_base64_$size.bin\"")
    
    # Decode fastbase64 encoded file and measure time
    fastbase64_decode_time=$(time_cmd "$FASTBASE64 -d  \"fastbase64_$size.b64\" > \"recovered_fastbase64_$size.bin\"")
    
    # Print results
    printf "%-8s | %-13s | %-17s | %-13s | %-16s\n" \
        "$size" "$base64_encode_time" "$fastbase64_encode_time" "$base64_decode_time" "$fastbase64_decode_time"
done

# Clean up
# rm -f test_*.bin base64_*.b64 fastbase64_*.b64

echo ""
echo "Benchmark completed."
