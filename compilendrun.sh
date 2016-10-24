if [ -z "$1" ]; then
	echo "Get serious mate, that's not a target";
	exit;
fi

scp latestserversim.c $1:~/imagica/programs/serverProgram/serversimulation.c
#scp salimgharvalues.txt $1:~/imagica/programs/serverProgram/
#scp goldrushvalues.txt $1:~/imagica/programs/serverProgram/

output=$(ssh $1 'gcc imagica/programs/serverProgram/serversimulation.c -o imagica/programs/serverProgram/server -lwiringPi');
echo $output;

if [ -z "$output" ]; then
	ssh $1 'cd ~/imagica/programs/serverProgram/;sudo ./install.sh;';
fi
