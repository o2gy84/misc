package s3aws

import (
	"context"
	"crypto/tls"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"strconv"
	"strings"
	"time"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/credentials"
	"github.com/aws/aws-sdk-go-v2/service/s3"
	"github.com/aws/aws-sdk-go-v2/service/s3/types"
)

func (w *Worker) moveToS3(ctx context.Context, log *logrus.Entry, reqID string, taskID uint64, fdbHash fileDBHash, body io.ReadCloser, size int64, src, dst uint32) (int, error) {
	myBucket := "cloud-mail-ru"
	publicKey := "cloud.mail.ru"
	secretKey := "secret"

	r2Resolver := aws.EndpointResolverWithOptionsFunc(func(service, region string, options ...interface{}) (aws.Endpoint, error) {
		return aws.Endpoint{
			//URL: "https://1.2.3.4:443",
			URL: "http://127.0.0.1:3456",
			HostnameImmutable: true,
			Source: aws.EndpointSourceCustom,
		}, nil
	})

	cfg, err := config.LoadDefaultConfig(context.TODO(),
		config.WithEndpointResolverWithOptions(r2Resolver),
		config.WithCredentialsProvider(credentials.NewStaticCredentialsProvider(publicKey, secretKey, "")),
		config.WithRegion("auto"),
		config.WithClientLogMode(aws.LogRetries | aws.LogRequest),
	)
	if err != nil {
		return 0, err
	}

	noRetry := func(options *s3.Options) {
		options.RetryMaxAttempts = 1
	}

	tr := &http.Transport{
        TLSClientConfig: &tls.Config{InsecureSkipVerify: true},
    }
	cfg.HTTPClient = &http.Client{Transport: tr}

	size := int64(fdbHash.size)

	client := s3.NewFromConfig(cfg)
	output, err := client.PutObject(context.TODO(), &s3.PutObjectInput{
		Bucket: aws.String(myBucket),
	    Key:    aws.String(fdbHash.hash),
    	Body:   body,
		ContentLength: &size,
		//ChecksumAlgorithm: types.ChecksumAlgorithmSha256,
	}, noRetry)

	/*
	output, err := client.PutObject(context.TODO(), &s3.PutObjectInput{
		Bucket: aws.String(myBucket),
	    Key:    aws.String(fdbHash.hash),
		Body: file,
	},
	noRetry,
	s3.WithAPIOptions(
    	//v4.AddUnsignedPayloadMiddleware,
	    //v4.RemoveComputePayloadSHA256Middleware,
		//v4.AddStreamingEventsPayload,
		//v4.AddComputePayloadSHA256Middleware,
		//v4.AddContentSHA256HeaderMiddleware,
	))
	*/

	/*
	output, err := client.CreateMultipartUpload(context.TODO(), &s3.CreateMultipartUploadInput{
		Bucket: aws.String(myBucket),
	    Key:    aws.String(fdbHash.hash),
		//ContentLength: &size,
	})
	if err != nil {
		log.Infof("create multipart failed: %v", err)
		return 0, err
	}

	fakeID := "xxxxxxxxxxxxxxxxxxxxxx"

	parts := types.CompletedMultipartUpload{
		//Parts: []types.CompletedPart{part1},
	}

	{
		var partNumber int32 = 1
		upl, err := client.UploadPart(context.TODO(), &s3.UploadPartInput{
			Bucket: aws.String(myBucket),
			Key:    aws.String(fdbHash.hash),
			PartNumber: &partNumber,
			UploadId: output.UploadId,
			//UploadId: &fakeID,
			Body:   body,
		})
		if err != nil {
			log.Infof("failed upload part: %v", err)
			return 0, err
		}
		log.Info("part 1 is uploaded: %+v", upl)
		part := types.CompletedPart{
			PartNumber: &partNumber,
			ETag:       upl.ETag,
		}
		parts.Parts = append(parts.Parts, part)
	}
	{
		var partNumber int32 = 2
		upl, err := client.UploadPart(context.TODO(), &s3.UploadPartInput{
			Bucket: aws.String(myBucket),
			Key:    aws.String(fdbHash.hash),
			PartNumber: &partNumber,
			UploadId: output.UploadId,
			//UploadId: &fakeID,
			Body:   body,
		})
		if err != nil {
			log.Infof("failed upload part: %v", err)
			return 0, err
		}
		log.Info("part 2 is uploaded: %+v", upl)
		part := types.CompletedPart{
			PartNumber: &partNumber,
			ETag:       upl.ETag,
		}
		parts.Parts = append(parts.Parts, part)
	}

	res, err := client.CompleteMultipartUpload(context.TODO(), &s3.CompleteMultipartUploadInput{
		Bucket: aws.String(myBucket),
	    Key:    aws.String(fdbHash.hash),
		UploadId: output.UploadId,
		MultipartUpload: &parts,
	})

	if err != nil {
		log.Infof("failed complete multopart: %v", err)
		return 0, err
	}
}



