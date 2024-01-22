filesdir=$1
searchstr=$2

if [ -z "$filesdir" ]
then
    echo "filesdir is empty."
    exit 1
fi

if [ -z "$searchstr" ]
then
    echo "searchstr is empty."
    exit 1
fi

X=$(ls -1 $filesdir | wc -l)
Y=$(grep -R $searchstr $filesdir | tee /tmp/test_file | wc -l)
echo The number of files are $X and the number of matching lines are $Y.
