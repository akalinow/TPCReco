FROM elitpc/get:20190315_patched_v2
COPY requirements_apt.txt /tmp
COPY requirements_pip3.txt /tmp
RUN apt-get update -qq \
    && apt-get -y install $(cat /tmp/requirements_apt.txt) gdb valgrind git \
    && rm -rf /var/lib/apt/lists/*
RUN yes | pip3 install --no-cache-dir -r /tmp/requirements_pip3.txt
RUN rm /tmp/requirements_*.txt
