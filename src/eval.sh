# GShare hyperparameters
ghistoryBits_gshare=13

# Tournament hyperparameters
ghistoryBits_tournament=9
lhistoryBits_tournament=10
pcIndexBits_tournament=10

# Perceptron hyperparameters
ghistoryBits=16
pcIndexBits=9
perceptronTheta=20

make

for file in ../traces/*.bz2
do
    echo "Processing $file"
    filename=$(basename $file)
    filename="${filename%.*}"
    echo "----------GShare-----------"
    bunzip2 -kc $file | ./predictor --gshare:$ghistoryBits_gshare
    echo "----------Tournament-------"
    bunzip2 -kc $file | ./predictor --tournament:$ghistoryBits_tournament:$lhistoryBits_tournament:$pcIndexBits_tournament
    echo "----------Custom-----------"
    bunzip2 -kc $file | ./predictor --custom:$ghistoryBits:$pcIndexBits:$perceptronTheta
    echo "==========================="
done


