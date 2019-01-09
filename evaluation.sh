dataset_train=quickranktestdata/msn1/msn1.fold1.train.5k.txt
dataset_vali=quickranktestdata/msn1/msn1.fold1.vali.5k.txt
dataset_test=quickranktestdata/msn1/msn1.fold1.test.5k.txt

base_name=output/dataset

# LAMBDA MART
model=${base_name}-lambdamart-model
./bin/quicklearn \
  --algo LAMBDAMART \
  --train ${dataset_train} \
  --valid ${dataset_vali} \
  --train-metric NDCG \
  --train-cutoff 10 \
  --model-out ${model}.xml

./bin/quicklearn \
    --model-in ${model}.xml \
    --test ${dataset_test} \
    --test-metric NDCG \
    --test-cutoff 10 \
    --scores ${model}-scores.txt

# MART
model=${base_name}-mart-model
./bin/quicklearn \
  --algo MART \
  --train ${dataset_train} \
  --valid ${dataset_vali} \
  --train-metric NDCG \
  --train-cutoff 10 \
  --model-out ${model}.xml

./bin/quicklearn \
    --model-in ${model}.xml \
    --test ${dataset_test} \
    --test-metric NDCG \
    --test-cutoff 10 \
    --scores ${model}-scores.txt


# LAMBDAMART-SELECTIVE
model=${base_name}-lambdamartselective-model
./bin/quicklearn \
  --algo LAMBDAMART-SELECTIVE \
  --train ${dataset_train} \
  --valid ${dataset_vali} \
  --train-metric NDCG \
  --train-cutoff 10 \
  --model-out ${model}.xml

./bin/quicklearn \
    --model-in ${model}.xml \
    --test ${dataset_test} \
    --test-metric NDCG \
    --test-cutoff 10 \
    --scores ${model}-scores.txt

# STOCHASTIC-NEGATIVE
model=${base_name}-stochasticnegative-model
./bin/quicklearn \
  --algo STOCHASTIC-NEGATIVE \
  --train ${dataset_train} \
  --valid ${dataset_vali} \
  --train-metric NDCG \
  --train-cutoff 10 \
  --model-out ${model}.xml

./bin/quicklearn \
    --model-in ${model}.xml \
    --test ${dataset_test} \
    --test-metric NDCG \
    --test-cutoff 10 \
    --scores ${model}-scores.txt


# RANKBOOST
model=${base_name}-rankboost-model
./bin/quicklearn \
  --algo RANKBOOST \
  --train ${dataset_train} \
  --valid ${dataset_vali} \
  --train-metric NDCG \
  --train-cutoff 10 \
  --model-out ${model}.xml

./bin/quicklearn \
    --model-in ${model}.xml \
    --test ${dataset_test} \
    --test-metric NDCG \
    --test-cutoff 10 \
    --scores ${model}-scores.txt