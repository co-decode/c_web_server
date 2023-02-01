#!/bin/bash

for N in {1..50}
do
	curl -D - http://localhost:8001/ &
done
wait
