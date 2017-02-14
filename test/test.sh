ret=0
delim="_"

file=$1
bindir=$2

bpe_dir=bpe-models

filename=$(basename "$file")
dir=$(dirname "$file")
test="${filename%.*}"

# Extract test options.
name=$(echo $test | cut -f1 -d$delim)
mode=$(echo $test | cut -f2 -d$delim)
joiner_annotate=$(echo $test | cut -f3 -d$delim)
case=$(echo $test | cut -f4 -d$delim)
bpe=$(echo $test | cut -f5 -d$delim)

tokenize_opts="--mode $mode"
detokenize_opts=""

if [ $case = "true" ]; then
    tokenize_opts="$tokenize_opts --case_feature"
    detokenize_opts="--case_feature"
fi

if [ $joiner_annotate = "marker" ]; then
    tokenize_opts="$tokenize_opts --joiner_annotate"
fi

if [ $bpe ]; then
    tokenize_opts="$tokenize_opts --bpe_model $dir/$bpe_dir/$bpe"
fi

# Test tokenization.
$bindir/cli/tokenize $tokenize_opts < $dir/$test.raw >tmp # 2>/dev/null
diff tmp $dir/$name.tokenized > /dev/null
res=$?
if [ $res -ne 0 ]; then
    ret=1
fi

# Test detokenization.
$bindir/cli/detokenize $detokenize_opts < $dir/$name.tokenized >tmp # 2>/dev/null
if [ -f $dir/$name.detokenized ]; then
    diff tmp $dir/$name.detokenized > /dev/null
else
    diff tmp $dir/$test.raw > /dev/null
fi
res=$?
if [ $res -ne 0 ]; then
    ret=1
fi

rm -f tmp

exit $ret
