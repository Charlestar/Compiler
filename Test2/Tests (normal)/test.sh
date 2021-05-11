path="tests/"
files=$(ls $path)
for filename in $files
do
   echo -e "\n" $filename
   ~/Compiler/Lab/Code/parser $path$filename
done
