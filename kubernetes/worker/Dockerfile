FROM centos:7

RUN yum -y install epel-release && yum clean all

COPY worker.py /root/worker.py
CMD ["/bin/bash"]
