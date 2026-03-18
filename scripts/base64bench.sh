#!/bin/bash

# Benchmark script to compare fastbase64 and base64 performance
# Measures encoding and decoding times for various file sizes

SIZE=""
REPETITIONS=""
FASTBASE64=""

while [ $# -gt 0 ]; do
    case $1 in
        --size)
            SIZE="$2"
            shift 2
            ;;
        --repetitions)
            REPETITIONS="$2"
            shift 2
            ;;
        *)
            FASTBASE64="$1"
            shift
            break
            ;;
    esac
done

if [ -z "$FASTBASE64" ]; then
    echo "Usage: $0 [--size SIZE] [--repetitions N] /path/to/fastbase64"
    exit 1
fi

num_runs=${REPETITIONS:-10}

# Check if fastbase64 exists
if [ ! -x "$FASTBASE64" ]; then
    echo "Error: $FASTBASE64 not found or not executable"
    exit 1
fi

# Detect OS for base64 command syntax
if base64 --help 2>/dev/null | grep -q GNU; then
    BASE64_ENCODE="base64"
    BASE64_DECODE="base64 -d"
else
    BASE64_ENCODE="base64 -i"
    BASE64_DECODE="base64 -d -i"
fi

echo "Starting fastbase64 vs base64 benchmark..."
if [ $num_runs -eq 1 ]; then
    echo "Using 1 repetition for timing."
else
    echo "Using $num_runs repetitions for timing."
fi

# Function to time a command in milliseconds with 0.1ms precision,
# running a specified number of times and returning the median.
time_cmd() {
    local cmd="$1"
    local num_runs="${2:-10}"
    local times=()
    for i in $(seq 1 $num_runs); do
        local start_ns=$(python3 -c "import time; print(time.perf_counter_ns())")
        eval "$cmd" >/dev/null 2>&1
        local end_ns=$(python3 -c "import time; print(time.perf_counter_ns())")
        # Convert ns to tenths of ms (0.1ms), rounded to nearest unit.
        local elapsed_tenths=$(( (end_ns - start_ns + 50000) / 100000 ))
        times+=("$elapsed_tenths")
    done
    # Sort times expressed in tenths of milliseconds.
    local sorted=($(printf '%s\n' "${times[@]}" | sort -n))
    # Median: average of 5th and 6th values (indices 4 and 5), or single if num_runs=1
    if [ $num_runs -eq 1 ]; then
        local median_tenths=${times[0]}
    else
        local median_tenths=$(( (sorted[4] + sorted[5]) / 2 ))
    fi
    printf "%d.%d" $((median_tenths / 10)) $((median_tenths % 10))
}

# Create test files
if [ -n "$SIZE" ]; then
    if [[ $SIZE =~ ^([0-9]+)([mMgG])$ ]]; then
        num=${BASH_REMATCH[1]}
        unit=${BASH_REMATCH[2]}
        case $unit in
            m|M) count=$((num * 1024)) ; bytes=$((num * 1024 * 1024)) ;;
            g|G) count=$((num * 1024 * 1024)) ; bytes=$((num * 1024 * 1024 * 1024)) ;;
        esac
    else
        echo "Invalid size: $SIZE. Use format like 10m or 1G"
        exit 1
    fi
    if [ $bytes -ge $((1024 * 1024 * 1024)) ]; then
        if [ -z "$REPETITIONS" ]; then
            num_runs=1
        fi
    fi
    sizes=("$SIZE")
    files=("test_${SIZE}.bin")
    if [ -f "test_${SIZE}.bin" ]; then
        echo "Test file test_${SIZE}.bin already exists, skipping creation."
    else
        echo "Creating test file for $SIZE..."
        dd if=/dev/urandom of="test_${SIZE}.bin" bs=1024 count=$count 2>/dev/null
    fi
else
    sizes=("1m" "10m" "100m")
    files=("test_1m.bin" "test_10m.bin" "test_100m.bin")
    echo "Creating test files..."
    for i in "${!sizes[@]}"; do
        file="${files[$i]}"
        if [ -f "$file" ]; then
            echo "Test file $file already exists, skipping."
        else
            size="${sizes[$i]}"
            case $size in
                1m) dd if=/dev/urandom of="$file" bs=1024 count=1024 2>/dev/null ;;
                10m) dd if=/dev/urandom of="$file" bs=1024 count=10240 2>/dev/null ;;
                100m) dd if=/dev/urandom of="$file" bs=1024 count=102400 2>/dev/null ;;
            esac
        fi
    done
fi

echo ""
echo "Benchmark Results (times in milliseconds):"
echo "Size     | Encode Base64 | Encode FastBase64 | Decode Base64 | Decode FastBase64"
echo "---------|---------------|-------------------|---------------|------------------"

for i in "${!sizes[@]}"; do
    size="${sizes[$i]}"
    file="${files[$i]}"
    
    # Encode with fastbase64 and measure time
    fastbase64_encode_time=$(time_cmd "$FASTBASE64 -e  \"$file\" > \"fastbase64_$size.b64\"" $num_runs)
    
    # Encode with base64 and measure time
    base64_encode_time=$(time_cmd "$BASE64_ENCODE \"$file\" > \"base64_$size.b64\"" $num_runs)
    
    # Decode base64 encoded file and measure time
    base64_decode_time=$(time_cmd "$BASE64_DECODE \"base64_$size.b64\" > \"recovered_base64_$size.bin\"" $num_runs)
    
    # Decode fastbase64 encoded file and measure time
    fastbase64_decode_time=$(time_cmd "$FASTBASE64 -d  \"fastbase64_$size.b64\" > \"recovered_fastbase64_$size.bin\"" $num_runs)
    
    # Print results
    printf "%-8s | %-13s | %-17s | %-13s | %-16s\n" \
        "$size" "$base64_encode_time" "$fastbase64_encode_time" "$base64_decode_time" "$fastbase64_decode_time"
done

# Clean up
# rm -f test_*.bin base64_*.b64 fastbase64_*.b64

echo ""
echo "Benchmark completed."
