#!/bin/sh

GRAPHS="latency roundtrip bandwidth bibw alltoall broadcast reduce allreduce"

usage()
{
  echo "Usage: $0 <datafile_prefix> [latency,roundtrip,bandwidth,bibw,alltoall,broadcast,reduce,allreduce]"
  exit 1
}

if [ $# -ne 1 -a $# -ne 2 ]; then
  usage;
fi

PREFIX=$1
DATAPREFIX=`basename $1`

if [ $# -eq 2 ]; then
  GRAPHS=$2
fi

for g in $GRAPHS
do
#  echo "Generating ${PREFIX}_${g}.gp..."
  rm -f ${PREFIX}_${g}.gp
  if [ $g = "broadcast" ]; then
    sed -e "s:TITLE:Performance of MPI Broadcast of ${DATAPREFIX}:g" < lib/${g}_template.gp | \
    sed -e "s:OUTPUT:${DATAPREFIX}_${g}.ps:g" > ${PREFIX}_${g}.gp
    echo "plot \"${DATAPREFIX}_${g}.dat\" with linespoints" >> ${PREFIX}_${g}.gp
  elif [ $g = "reduce" ]; then
    sed -e "s:TITLE:Performance of MPI Reduce of ${DATAPREFIX}:g" < lib/${g}_template.gp | \
    sed -e "s:OUTPUT:${DATAPREFIX}_${g}.ps:g" > ${PREFIX}_${g}.gp
    echo "plot \"${DATAPREFIX}_${g}.dat\" with linespoints" >> ${PREFIX}_${g}.gp
  elif [ $g = "allreduce" ]; then
    sed -e "s:TITLE:Performance of MPI Allreduce of ${DATAPREFIX}:g" < lib/${g}_template.gp | \
    sed -e "s:OUTPUT:${DATAPREFIX}_${g}.ps:g" > ${PREFIX}_${g}.gp
    echo "plot \"${DATAPREFIX}_${g}.dat\" with linespoints" >> ${PREFIX}_${g}.gp
  elif [ $g = "alltoall" ]; then
    sed -e "s:TITLE:Performance of MPI Alltoall of ${DATAPREFIX}:g" < lib/${g}_template.gp | \
    sed -e "s:OUTPUT:${DATAPREFIX}_${g}.ps:g" > ${PREFIX}_${g}.gp
    echo "plot \"${DATAPREFIX}_${g}.dat\" with linespoints" >> ${PREFIX}_${g}.gp
  elif [ $g = "bibw" ]; then  
    sed -e "s:TITLE:Bidirectional MPI Bandwidth of ${DATAPREFIX}:g" < lib/bandwidth_template.gp | \
    sed -e "s:OUTPUT:${DATAPREFIX}_${g}.ps:g" > ${PREFIX}_${g}.gp
    echo "plot \"${DATAPREFIX}_${g}.dat\" with linespoints" >> ${PREFIX}_${g}.gp
  elif [ $g = "bandwidth" ]; then  
    sed -e "s:TITLE:Unidirectional MPI Bandwidth of ${DATAPREFIX}:g" < lib/${g}_template.gp | \
    sed -e "s:OUTPUT:${DATAPREFIX}_${g}.ps:g" > ${PREFIX}_${g}.gp
    echo "plot \"${DATAPREFIX}_${g}.dat\" with linespoints" >> ${PREFIX}_${g}.gp
  elif [ $g = "latency" ]; then  
    sed -e "s:TITLE:Latency of MPI Send of ${DATAPREFIX}:g" < lib/${g}_template.gp | \
    sed -e "s:OUTPUT:${DATAPREFIX}_${g}.ps:g" > ${PREFIX}_${g}.gp
    echo "plot \"${DATAPREFIX}_${g}.dat\" with linespoints" >> ${PREFIX}_${g}.gp
  else  
    sed -e "s:TITLE:Roundtrip time of MPI Send of ${DATAPREFIX}:g" < lib/${g}_template.gp | \
    sed -e "s:OUTPUT:${DATAPREFIX}_${g}.ps:g" > ${PREFIX}_${g}.gp
    echo "plot \"${DATAPREFIX}_${g}.dat\" with linespoints" >> ${PREFIX}_${g}.gp
  fi
done

exit 0