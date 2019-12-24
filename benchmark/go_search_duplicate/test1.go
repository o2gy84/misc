package main

import (
	"fmt"
	"math/rand"
	"time"
	"sync"
	"sync/atomic"
	"context"
)

func generateUniqSlice(n uint64) []uint64 {
	slice := make([]uint64, n)
	var i uint64 = 0
	for i = 1; i <= n; i++ {
		slice[i-1] = i
	}

	rand.Shuffle(len(slice), func(i, j int) { slice[i], slice[j] = slice[j], slice[i] })
	return slice
}

func addElementOnIndex(slice []uint64, index uint64, element uint64) []uint64 {
	slice = append(slice, 0)
	copy(slice[index + 1:], slice[index:])
	slice[index] = element
	return slice
}

func prepareSlice(n uint64) []uint64 {
	slice := generateUniqSlice(n)
	randIndex := rand.Uint64() % n
	randElement := slice[randIndex]
	return addElementOnIndex(slice, rand.Uint64() % n, randElement)
}

func findDoubledWithHash(slice []uint64) (found bool, elem uint64) {
	found = false
	m := make(map[uint64]struct{}, len(slice))

	for _, n := range(slice) {
		_, ok := m[n]
		if ok {
			found = true
			elem = n
			return found, elem
		}
		m[n] = struct{}{}
	}
	return found, 0
}

func findDoubledWithSlice(slice []uint64) (found bool, elem uint64) {
	found = false
	m := make([]uint32, len(slice))

	for _, n := range(slice) {
		newVal := m[n] + 1
		if newVal == 2 {
			found = true
			elem = n
			return found, elem
		}
		m[n] = newVal
	}
	return found, 0
}

type PartialFindResult struct {
	found bool
	elem uint64
}

func findPartial(ctx context.Context, slice []uint64, m map[uint64]struct{}, mutex *sync.Mutex, dst chan PartialFindResult) {
	for _, n := range(slice) {
		select {
			case <-ctx.Done():
				dst <-PartialFindResult{found: false, elem: 0}
				return
			default:
		}

		mutex.Lock()
		_, ok := m[n]
		if ok {
			mutex.Unlock()
			dst <-PartialFindResult{found: true, elem: n}
			return
		}
		m[n] = struct{}{}
		mutex.Unlock()
	}
	dst <-PartialFindResult{found: false, elem: 0}
}

type chunk struct {
	start, end uint
}

func splitToChunks(length uint, n uint) []chunk {
	chunkLen := length/n
	chunks := make([]chunk, n)
	if length < chunkLen {
		chunks[0] = chunk{start:0, end: length}
		return chunks
	}

	var start uint = 0
	var i uint
	for i = 0; i < (n - 1); i++ {
		chunks[i] = chunk{start: start, end: start + length/n}
		start = start + length/n
	}
	chunks[len(chunks) - 1] = chunk{start: start, end: length}
	return chunks
}

func findDoubledParallelWithHash(slice []uint64) (bool, uint64) {
	var goroutinesNum uint = 2
	dst := make(chan PartialFindResult)
	ctx, cancel := context.WithCancel(context.Background())

	m := make(map[uint64]struct{}, len(slice))
	var mutex sync.Mutex

	chunks := splitToChunks(uint(len(slice)), goroutinesNum)
	for _, chunk := range(chunks) {
		go findPartial(ctx, slice[chunk.start:chunk.end], m, &mutex, dst)
	}

	found := false
	var foundElement uint64 = 0
	for i := 0; i < len(chunks); i++ {
		result := <-dst
		if (result.found) {
			cancel()
			found = true
			foundElement = result.elem
		}
	}
	return found, foundElement
}

func findPartialWithAtomic(ctx context.Context, slice []uint64, checksSlice []uint32, dst chan PartialFindResult) {
	for _, n := range(slice) {
		select {
			case <-ctx.Done():
				dst <-PartialFindResult{found: false, elem: 0}
				return
			default:
		}

		ptr := &checksSlice[n]
		newVal := atomic.AddUint32(ptr, 1)
		if newVal == 2 {
			dst <-PartialFindResult{found: true, elem: n}
			return
		}
	}
	dst <-PartialFindResult{found: false, elem: 0}
}

func findDoubledParallelWithAtomics(slice []uint64) (bool, uint64) {
	var goroutinesNum uint = 4
	dst := make(chan PartialFindResult)
	ctx, cancel := context.WithCancel(context.Background())
	checksSlice := make([]uint32, len(slice))

	chunks := splitToChunks(uint(len(slice)), goroutinesNum)
	for _, chunk := range(chunks) {
		go findPartialWithAtomic(ctx, slice[chunk.start:chunk.end], checksSlice, dst)
	}

	found := false
	var foundElement uint64 = 0
	for i := 0; i < len(chunks); i++ {
		result := <-dst
		if (result.found) {
			cancel()
			found = true
			foundElement = result.elem
		}
	}
	return found, foundElement
}

func findPartialSyncMap(ctx context.Context, slice []uint64, m *sync.Map, dst chan PartialFindResult) {
	for _, n := range(slice) {
		select {
			case <-ctx.Done():
				dst <-PartialFindResult{found: false, elem: 0}
				return
			default:
		}

		_, ok := m.Load(n)
		if ok {
			dst <-PartialFindResult{found: true, elem: n}
			return
		}
		m.Store(n, struct{}{})
	}
	dst <-PartialFindResult{found: false, elem: 0}
}

func findDoubledParallelSyncMap(slice []uint64) (bool, uint64) {
	var goroutinesNum uint = 4
	dst := make(chan PartialFindResult)
	ctx, cancel := context.WithCancel(context.Background())

	m := sync.Map{}

	chunks := splitToChunks(uint(len(slice)), goroutinesNum)
	for _, chunk := range(chunks) {
		go findPartialSyncMap(ctx, slice[chunk.start:chunk.end], &m, dst)
	}

	found := false
	var foundElement uint64 = 0
	for i := 0; i < len(chunks); i++ {
		result := <-dst
		if (result.found) {
			cancel()
			found = true
			foundElement = result.elem
		}
	}
	return found, foundElement
}

func main() {
	rand.Seed(time.Now().UnixNano())
	var n uint64 = 100
	slice := prepareSlice(n)

	fmt.Printf("slice: %+v\n\n", slice)

	found, elem := findDoubledWithHash(slice)
	fmt.Printf("found doubled with hash: %v, elem: %d\n", found, elem)

	found, elem = findDoubledWithSlice(slice)
	fmt.Printf("found doubled with slice: %v, elem: %d\n", found, elem)

	found, elem = findDoubledParallelWithHash(slice)
	fmt.Printf("found doubled in parallel with hash: %v, elem: %d\n", found, elem)

	found, elem = findDoubledParallelWithAtomics(slice)
	fmt.Printf("found doubled in parallel with atomics: %v, elem: %d\n", found, elem)

	found, elem = findDoubledParallelSyncMap(slice)
	fmt.Printf("found doubled in parallel with sync.Map: %v, elem: %d\n", found, elem)
}


