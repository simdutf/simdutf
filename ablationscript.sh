
cmake -B build -DSIMDUTF_BASE64_DISABLE_SINGLE_OPT=OFF -DSIMDUTF_BENCHMARKS=ON -DSIMDUTF_TESTS=ON
cmake --build build --target benchmark_base64

cmake -B buildnosingle -DSIMDUTF_BASE64_DISABLE_SINGLE_OPT=ON -DSIMDUTF_BENCHMARKS=ON -DSIMDUTF_TESTS=ON
cmake --build buildnosingle --target benchmark_base64
./buildnosingle/benchmark_base64 -d ../base64data/email/*.txt

for i in {2..32}; do
    cmake -B build$i -DSIMDUTF_BASE64_BLOCK_SIZE=$i -DSIMDUTF_BASE64_DISABLE_SINGLE_OPT=OFF -DSIMDUTF_BENCHMARKS=ON -DSIMDUTF_TESTS=ON
    cmake --build build$i --target benchmark_base64
done

echo "standard"
./build/benchmarks/base64/benchmark_base64 -d ../base64data/email/*.txt
echo "single disabled"
./buildnosingle/benchmarks/base64/benchmark_base64 -d ../base64data/email/*.txt
for i in {2..32}; do
    echo "using $i stack blocks"
    ./build$i/benchmarks/base64/benchmark_base64 -d ../base64data/email/*.txt
done