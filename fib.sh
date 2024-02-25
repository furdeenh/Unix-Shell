setenv ACC $last0
addacc $last1
setenv last0 $last1
setenv last1 $ACC
echo $counter entry is $ACC
setenv ACC $counter
addacc 
setenv counter $ACC
test $counter -gt $end
?exit $last1
$0 fib.sh
