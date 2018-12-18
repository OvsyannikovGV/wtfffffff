for i in `seq 1 1024` ; do for j in `seq 1 10240` ; do rm -rf x ; sh -c "./a.out b &" ; ./a.out >x ; diff b x ; done ; echo $i ; done
