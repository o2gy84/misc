FROM centos:7

RUN yum -y install epel-release && yum clean all
RUN yum -y install python-pip
RUN pip install requests

COPY worker.py /root/worker.py
CMD ["/bin/bash"]
