package main

import(
	"flag"
	"fmt"
	"bytes"
	"io/ioutil"
	"crypto/tls"
	"log"
	"net/url"
	"net/http"
	"net/http/httputil"
	"strconv"
)

func rewriteBody(resp *http.Response) (err error) {
	b, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return  err
	}

	fmt.Printf("\n==> response body:\n%s\n", string(b))

	err = resp.Body.Close()
	if err != nil {
		log.Printf("error body close: %s", err)
		return err
	}

	body := ioutil.NopCloser(bytes.NewReader(b))

	resp.Body = body
	resp.ContentLength = int64(len(b))
	resp.Header.Set("Content-Length", strconv.Itoa(len(b)))
	return nil
}

type DebugTransport struct{}

func (DebugTransport) RoundTrip(r *http.Request) (*http.Response, error) {
	b, err := httputil.DumpRequestOut(r, false)
	if err != nil {
		return nil, err
	}
	fmt.Println(string(b))
	return http.DefaultTransport.RoundTrip(r)
}

func main() {
	proxyTo := flag.String("proxy", "http://google.com", "server to proxy to")
	bindPort := flag.String("port", "3456", "bind port")
	bindHost := flag.String("host", "0.0.0.0", "bind host")
	flag.Parse()

	remote, err := url.Parse(*proxyTo)
	if err != nil {
		panic(err)
	}

	proxy := httputil.NewSingleHostReverseProxy(remote)
	proxy.ModifyResponse = rewriteBody
	proxy.Transport = DebugTransport{}

	handler := func(p *httputil.ReverseProxy) func(http.ResponseWriter, *http.Request) {
		return func(w http.ResponseWriter, r *http.Request) {
			// All headers already are copied

			// Often we need to pass original Host
			// r.Host = remote.Host

			p.ServeHTTP(w, r)
		}
	}

	http.DefaultTransport.(*http.Transport).TLSClientConfig = &tls.Config{InsecureSkipVerify: true}
	http.HandleFunc("/", handler(proxy))

	log.Printf("Start listen to: %s:%s --> %s", *bindHost, *bindPort, *proxyTo)

	hp := *bindHost + ":" + *bindPort
	err = http.ListenAndServe(hp, nil)
	if err != nil {
		panic(err)
	}
}
