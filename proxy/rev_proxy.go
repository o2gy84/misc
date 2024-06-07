package main

import(
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

	fmt.Printf("\n==> response body: %s\n\n", string(b))

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
        remote, err := url.Parse("https://10.102.228.22:443")
        if err != nil {
                panic(err)
        }

		http.DefaultTransport.(*http.Transport).TLSClientConfig = &tls.Config{InsecureSkipVerify: true}

        handler := func(p *httputil.ReverseProxy) func(http.ResponseWriter, *http.Request) {
                return func(w http.ResponseWriter, r *http.Request) {
						// this breaks signature
                        // r.Host = remote.Host

                        p.ServeHTTP(w, r)
                }
        }

        proxy := httputil.NewSingleHostReverseProxy(remote)

		proxy.ModifyResponse = rewriteBody
		proxy.Transport = DebugTransport{}

        http.HandleFunc("/", handler(proxy))

		port := "3356"
		host := "0.0.0.0"
		ph := host + ":" + port
		log.Printf("Start listen to: %s", ph)

        err = http.ListenAndServe(ph, nil)
        if err != nil {
                panic(err)
        }
}
