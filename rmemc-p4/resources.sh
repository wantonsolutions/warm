pushd ~/bf-sde-9.7.0/build/pkgsrc
resources=`find . | grep mau.resources.log`
for res in $resources; do
    avg=`cat $res | grep "Average"`
    name=`echo $res | cut -d / -f 3`
    #echo $name
    #echo $res
    echo $avg | sed "s/Average/${name}/g"
done
popd