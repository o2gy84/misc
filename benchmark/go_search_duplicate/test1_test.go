package main

import (
	"testing"
	"sync"
)

var result uint64


// 10 000 000
var slice10000000 []uint64
func prepareTest10000000() []uint64 {
	var once sync.Once
	once.Do(func() {
		slice10000000 = prepareSlice(10000000)
	})
	return slice10000000
}

func BenchmarkBenchParallelWithHash10000000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest10000000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledParallelWithHash(slice)
		result = elem
	}
}

func BenchmarkBenchWithHash10000000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest10000000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledWithHash(slice)
		result = elem
	}
}

func BenchmarkBenchParallelWithAtomic10000000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest10000000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledParallelWithAtomics(slice)
		result = elem
	}
}

func BenchmarkBenchWithSlice10000000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest10000000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledWithSlice(slice)
		result = elem
	}
}


// 1 000 000 
var slice1000000 []uint64
func prepareTest1000000() []uint64 {
	var once sync.Once
	once.Do(func() {
		slice1000000 = prepareSlice(1000000)
	})
	return slice1000000
}

func BenchmarkBenchParallelWithHash1000000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest1000000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledParallelWithHash(slice)
		result = elem
	}
}

func BenchmarkBenchWithHash1000000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest1000000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledWithHash(slice)
		result = elem
	}
}

func BenchmarkBenchParallelWithAtomic1000000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest1000000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledParallelWithAtomics(slice)
		result = elem
	}
}

func BenchmarkBenchWithSlice1000000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest1000000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledWithSlice(slice)
		result = elem
	}
}


// 100 000
var slice100000 []uint64
func prepareTest100000() []uint64 {
	var once sync.Once
	once.Do(func() {
		slice100000 = prepareSlice(100000)
	})
	return slice100000
}

func BenchmarkBenchParallelWithHash100000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest100000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledParallelWithHash(slice)
		result = elem
	}
}

func BenchmarkBenchWithHash100000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest100000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledWithHash(slice)
		result = elem
	}
}

func BenchmarkBenchParallelWithAtomic100000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest100000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledParallelWithAtomics(slice)
		result = elem
	}
}

func BenchmarkBenchWithSlice100000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest100000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledWithSlice(slice)
		result = elem
	}
}

// 10 000
var slice10000 []uint64
func prepareTest10000() []uint64 {
	var once sync.Once
	once.Do(func() {
		slice10000 = prepareSlice(10000)
	})
	return slice10000
}

func BenchmarkBenchParallelWithHash10000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest10000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledParallelWithHash(slice)
		result = elem
	}
}

func BenchmarkBenchWithHash10000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest10000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledWithHash(slice)
		result = elem
	}
}

func BenchmarkBenchParallelWithAtomic10000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest10000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledParallelWithAtomics(slice)
		result = elem
	}
}

func BenchmarkBenchWithSlice10000(b *testing.B) {
	b.StopTimer()
	slice := prepareTest10000()
	b.StartTimer()
    for i := 0; i < b.N; i++ {
		_, elem := findDoubledWithSlice(slice)
		result = elem
	}
}

