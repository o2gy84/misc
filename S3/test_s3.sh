#!/bin/sh

s3Key=cloud.mail.ru
s3Secret='secret'
bucket=cloud-mail-ru

key=aabbcc
address='https://1.2.3.4:443'

resource="/${bucket}/${key}"
contentType="application/octet-stream"
dateValue="`date +'%a, %d %b %Y %H:%M:%S %z'`"
                                                                                                                                                                                                                                         echo "key: ${key}"
                                                                                                                                                                                                                                         stringToSignPUT="PUT\n\n${contentType}\n${dateValue}\n${resource}"
signature=`/bin/echo -en "$stringToSignPUT" | openssl sha1 -hmac ${s3Secret} -binary | base64`
echo "PUT: curl -k -X PUT -H 'Date: ${dateValue}' -H 'Content-Type: ${contentType}' -H 'Authorization: AWS ${s3Key}:${signature}' ${address}/${bucket}/${key} -d 'data'"

stringToSignGET="GET\n\n${contentType}\n${dateValue}\n${resource}"
signature=`/bin/echo -en "$stringToSignGET" | openssl sha1 -hmac ${s3Secret} -binary | base64`
echo "GET: curl -k -H 'Date: ${dateValue}' -H 'Content-Type: ${contentType}' -H 'Authorization: AWS ${s3Key}:${signature}' ${address}/${bucket}/${key}"

