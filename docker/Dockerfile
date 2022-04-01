FROM elitpc/get:20190315_patched_v2
ADD . /opt/soft/TPCReco-src
WORKDIR /opt/soft/TPCReco-src/build
RUN cmake -DCMAKE_INSTALL_PREFIX=/opt/soft/TPCReco ..
RUN make -j && make install
ENV PATH=${PATH}:/opt/soft/TPCReco/bin
WORKDIR /opt/soft/TPCReco/resources
