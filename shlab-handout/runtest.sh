#! /bin/bash
 
for file in $(ls trace*)
do
    ./sdriver.pl -t $file -s ./tshref > tshref_$file
    ./sdriver.pl -t $file -s ./tsh > tsh_$file
    diff tsh_$file tshref_$file > diff_$file
    echo diff_$file " :"
    cat diff_$file
    echo -e "-------------------------------------\n"
done