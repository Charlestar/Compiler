path="Tests/"
files=$(ls $path)
for filename in $files
do
   echo -e "\n" $filename
   ../Lab/Code/parser $path$filename
done
