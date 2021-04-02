path="Test/"
files=$(ls $path)
for filename in $files
do
   echo -e "\n" $filename
   ./parser $path$filename
done
